#include "hitmarker.hpp"

#include "hitboxes.hpp"
#include "lagcomp.hpp"

ATLAS_NO_DESTROY hitmarker g_hitmarker{ };

ATLAS_FORCE_INLINE color_t hitmarker_color( usize event )
{
    const config::hitmarker_t &settings{ g_cfg.m_hitmarker };
    return settings.m_override_color[ event ] ? settings.m_event_color[ event ] : settings.m_color;
}

void hitmarker::on_bullet_impact( IGameEvent *event )
{
    if ( !g_cfg.m_hitmarker.m_enabled || this->m_impact_count == MAX_HIT_IMPACTS ||
         g_csgo.m_engine->GetPlayerForUserID( event->GetInt( ATLAS_XORSTR( "userid" ).get( ) ) ) != g_csgo.m_engine->GetLocalPlayer( ) )
    {
        return;
    }

    this->m_impacts[ this->m_impact_count++ ] = vec3{ event->GetFloat( ATLAS_XORSTR( "x" ).get( ) ),
                                                      event->GetFloat( ATLAS_XORSTR( "y" ).get( ) ),
                                                      event->GetFloat( ATLAS_XORSTR( "z" ).get( ) ) };
}

void hitmarker::on_player_hurt( IGameEvent *event )
{
    const i32 local{ g_csgo.m_engine->GetLocalPlayer( ) };
    const i32 attacker{ g_csgo.m_engine->GetPlayerForUserID( event->GetInt( ATLAS_XORSTR( "attacker" ).get( ) ) ) };
    const i32 victim{ g_csgo.m_engine->GetPlayerForUserID( event->GetInt( ATLAS_XORSTR( "userid" ).get( ) ) ) };
    if ( attacker != local || victim <= 0 || victim == local )
    {
        return;
    }

    if ( g_cfg.m_hitmarker.m_hitsound )
    {
        g_csgo.m_surface->PlaySound( ATLAS_XORSTR( "buttons/arena_switch_press_02.wav" ).get( ) );
    }

    IClientEntity *client{ g_csgo.m_entity_list->GetClientEntity( victim ) };
    C_CSPlayer    *player{ client ? client->As< C_CSPlayer * >( ) : nullptr };
    if ( !g_cfg.m_hitmarker.m_enabled || !player )
    {
        return;
    }

    const i32 hitgroup{ event->GetInt( ATLAS_XORSTR( "hitgroup" ).get( ) ) };
    hit_t    &hit{ this->m_hits[ this->m_next ] };

    if ( g_cfg.m_hitmarker.m_render_mode != config::HITMARKER_IMPACT || hitgroup == HITGROUP_GENERIC ||
         !this->take_impact( victim, player, hit.m_position ) )
    {
        hit.m_position = player->WorldSpaceCenter( );
    }

    hit.m_time   = g_csgo.m_globals->realtime;
    hit.m_damage = event->GetInt( ATLAS_XORSTR( "dmg_health" ).get( ) );
    hit.m_event  = hitgroup == HITGROUP_HEAD ? config::HITMARKER_HEADSHOT : config::HITMARKER_NORMAL;
    if ( event->GetInt( ATLAS_XORSTR( "health" ).get( ) ) <= 0 )
    {
        hit.m_event = config::HITMARKER_KILL;
    }

    this->m_next = ( this->m_next + 1 ) % CAPACITY;
}

void hitmarker::on_net_update_end( )
{
    this->m_impact_count = 0;
}

void hitmarker::paint( ) const
{
    if ( !g_cfg.m_hitmarker.m_enabled || !g_csgo.m_engine->IsInGame( ) )
    {
        return;
    }

    const float  now{ g_csgo.m_globals->realtime };
    const bool   screen_mode{ g_cfg.m_hitmarker.m_render_mode == config::HITMARKER_2D };
    const hit_t *latest{ };
    if ( screen_mode )
    {
        for ( const hit_t &hit : this->m_hits )
        {
            const float age{ now - hit.m_time };
            const float duration{ hit.m_event == config::HITMARKER_KILL ? 0.85f : 0.65f };
            if ( age >= 0.0f && age < duration && ( !latest || hit.m_time > latest->m_time ) )
            {
                latest = &hit;
            }
        }
    }

    for ( const hit_t &hit : this->m_hits )
    {
        if ( screen_mode && &hit != latest )
        {
            continue;
        }

        const float age{ now - hit.m_time };
        const float duration{ hit.m_event == config::HITMARKER_KILL ? 0.85f : 0.65f };
        if ( age < 0.0f || age >= duration )
        {
            continue;
        }

        vec2 screen{ };
        if ( screen_mode )
        {
            screen = vec2{ static_cast< f32 >( g_cl.m_screen_width / 2 ), static_cast< f32 >( g_cl.m_screen_height / 2 ) };
        } else if ( !g_cl.world_to_screen( hit.m_position, screen ) )
        {
            continue;
        }

        const float progress{ age / duration };
        const float inner{ 3.0f + progress * 4.0f };
        const float outer{ 9.0f + progress * ( hit.m_event == config::HITMARKER_KILL ? 10.0f : 7.0f ) };
        color_t     color{ hitmarker_color( hit.m_event ) };
        color.a = static_cast< u8 >( static_cast< float >( color.a ) * ( 1.0f - progress ) );

        for ( i32 x{ -1 }; x <= 1; x += 2 )
        {
            for ( i32 y{ -1 }; y <= 1; y += 2 )
            {
                const vec2 direction{ static_cast< float >( x ) * 0.7071f, static_cast< float >( y ) * 0.7071f };
                g_draw.line( screen + direction * inner, screen + direction * outer, color, 2.0f );
            }
        }

        if ( g_cfg.m_hitmarker.m_show_damage && hit.m_damage > 0 )
        {
            char damage[ 16 ]{ };
            str::format_to( damage, sizeof( damage ), ATLAS_XORSTR( "-{}" ).get( ), hit.m_damage );
            const point_t at{ static_cast< i32 >( screen.x + 13.0f ), static_cast< i32 >( screen.y - 19.0f - progress * 23.0f ) };
            g_draw.text( ATLAS_XORSTR( "ESP Small" ).get( ), at, color, damage, true );
        }
    }
}

bool hitmarker::take_impact( i32 victim, C_CSPlayer *player, vec3 &position )
{
    const lag_record_t *entry{ g_lagcomp.record( victim, 0 ) };
    if ( !this->m_impact_count || !entry || !entry->m_header )
    {
        return false;
    }

    const mstudiohitboxset_t *set{ entry->m_header->pHitboxSet( player->m_nHitboxSet( ) ) };
    if ( !set )
    {
        return false;
    }

    pose_hitboxes_t pose{ };
    pose.build( set, entry->m_bones );

    i32   nearest{ };
    float nearest_distance{ FLT_MAX };
    for ( i32 index{ }; index < this->m_impact_count; ++index )
    {
        const float distance{ pose.distance( this->m_impacts[ index ] ) };
        if ( distance < nearest_distance )
        {
            nearest          = index;
            nearest_distance = distance;
        }
    }

    position                   = this->m_impacts[ nearest ];
    this->m_impacts[ nearest ] = this->m_impacts[ --this->m_impact_count ];
    return true;
}
