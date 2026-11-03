#include "global.hpp"

ATLAS_NO_DESTROY csgo    g_csgo{ };
ATLAS_NO_DESTROY client  g_cl{ };
ATLAS_NO_DESTROY api     g_api{ };
ATLAS_NO_DESTROY netvars g_netvars{ };
ATLAS_NO_DESTROY hooks   g_hooks{ };

void client::on_directx_frame( void *device )
{
    render::d3d9::D3DVIEWPORT9 vp{ };
    render::d3d9::device9{ device }.GetViewport( &vp );

    this->m_screen_width  = static_cast< i32 >( vp.width );
    this->m_screen_height = static_cast< i32 >( vp.height );
}

void client::update_local_data( )
{
    IClientEntity *local_entity{ g_csgo.m_entity_list->GetClientEntity( g_csgo.m_engine->GetLocalPlayer( ) ) };

    this->m_local = local_entity ? local_entity->As< C_CSPlayer * >( ) : nullptr;
}

void client::update_w2s( )
{
    atl::memcpy( this->m_w2s, g_csgo.m_engine->WorldToScreenMatrix( ), sizeof( VMatrix ) );
}

bool client::world_to_screen( const vec3 &world, vec2 &screen ) const
{
    const auto &m = this->m_w2s;

    const float w{ m[ 3 ][ 0 ] * world.x + m[ 3 ][ 1 ] * world.y + m[ 3 ][ 2 ] * world.z + m[ 3 ][ 3 ] };
    if ( w <= 0.001f )
    {
        return false;
    }

    const float cx{ m[ 0 ][ 0 ] * world.x + m[ 0 ][ 1 ] * world.y + m[ 0 ][ 2 ] * world.z + m[ 0 ][ 3 ] };
    const float cy{ m[ 1 ][ 0 ] * world.x + m[ 1 ][ 1 ] * world.y + m[ 1 ][ 2 ] * world.z + m[ 1 ][ 3 ] };

    screen.x = 0.5f * ( 1.f + cx / w ) * static_cast< float >( this->m_screen_width );
    screen.y = 0.5f * ( 1.f - cy / w ) * static_cast< float >( this->m_screen_height );

    return true;
}

bool client::is_on_screen( const rect_t &bounds ) const
{
    constexpr i32 margin{ 64 };

    const i32 width{ this->m_screen_width };
    const i32 height{ this->m_screen_height };

    return bounds.right >= -margin && bounds.left <= width + margin && bounds.bottom >= -margin && bounds.top <= height + margin;
}

void client::fix_movement( CUserCmd *cmd, const vec3 &original_angles ) const
{
    const f32 turn{ DEG2RAD( cmd->viewangles.y - original_angles.y ) };
    const f32 sine{ math::sin( turn ) };
    const f32 cosine{ math::cos( turn ) };

    const f32 forward{ cmd->forwardmove };
    const f32 side{ cmd->sidemove };

    cmd->forwardmove = forward * cosine - side * sine;
    cmd->sidemove    = forward * sine + side * cosine;
}

void client::trace_line( const vec3 &start, const vec3 &end, u32 mask, ITraceFilter *filter, trace_t &trace ) const
{
    Ray_t ray{ };
    ray.Init( start, end );

    g_csgo.m_trace->TraceRay( ray, mask, filter, &trace );
}

void client::clip_trace_to_players( const vec3 &start, const vec3 &end, u32 mask, ITraceFilter *filter, trace_t &trace,
                                    float min_range ) const
{
    constexpr float MAX_RANGE{ 60.0f };

    Ray_t ray{ };
    ray.Init( start, end );

    float smallest_fraction{ trace.fraction };

    for ( i32 index{ 1 }; index <= g_csgo.m_globals->maxClients; ++index )
    {
        IClientEntity *entity{ g_csgo.m_entity_list->GetClientEntity( index ) };
        if ( !entity )
        {
            continue;
        }

        auto *player{ entity->As< C_CSPlayer * >( ) };
        if ( !player->IsPlayer( ) ||
             !player->IsAlive( ) ||
             player->IsDormant( ) ||
             ( filter && !filter->ShouldHitEntity( player, static_cast< i32 >( mask ) ) ) )
        {
            continue;
        }

        const float range{ math::distance_to_ray( player->WorldSpaceCenter( ), start, end ) };
        if ( range < min_range || range > MAX_RANGE )
        {
            continue;
        }

        trace_t player_trace{ };
        g_csgo.m_trace->ClipRayToEntity( ray, mask | CONTENTS_HITBOX, player, &player_trace );

        if ( player_trace.fraction < smallest_fraction )
        {
            trace             = player_trace;
            smallest_fraction = player_trace.fraction;
        }
    }
}
