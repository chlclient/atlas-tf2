#include "shot_log.hpp"

#include "gui/notifications.hpp"

#include "hitmarker.hpp"
#include "player_data.hpp"
#include "ragebot.hpp"
#include "resolver.hpp"

ATLAS_NO_DESTROY shot_log      g_shot_log{ };
ATLAS_NO_DESTROY shot_aim_t    shot_log::s_staged{ };
ATLAS_NO_DESTROY shot_record_t shot_log::s_shots[ MAX_SHOT_RECORDS ]{ };
ATLAS_NO_DESTROY shot_answer_t shot_log::s_answers[ MAX_SHOT_RECORDS ]{ };

constexpr color_t SHOT_LINE_COLORS[ SHOT_LINE_COUNT ]{ { 196, 199, 208 }, { 126, 196, 107 }, { 239, 75, 72 } };

ATLAS_FORCE_INLINE i32 event_player( IGameEvent *event, const char *key )
{
    return g_csgo.m_engine->GetPlayerForUserID( event->GetInt( key ) );
}

ATLAS_FORCE_INLINE const char *hitgroup_label( i32 hitgroup )
{
    switch ( hitgroup )
    {
        case HITGROUP_HEAD: return ATLAS_XORSTR_STATIC( "head" );
        case HITGROUP_CHEST: return ATLAS_XORSTR_STATIC( "chest" );
        case HITGROUP_STOMACH: return ATLAS_XORSTR_STATIC( "stomach" );
        case HITGROUP_LEFTARM: return ATLAS_XORSTR_STATIC( "left arm" );
        case HITGROUP_RIGHTARM: return ATLAS_XORSTR_STATIC( "right arm" );
        case HITGROUP_LEFTLEG: return ATLAS_XORSTR_STATIC( "left leg" );
        case HITGROUP_RIGHTLEG: return ATLAS_XORSTR_STATIC( "right leg" );
        case HITGROUP_GEAR: return ATLAS_XORSTR_STATIC( "gear" );
        default: return ATLAS_XORSTR_STATIC( "generic" );
    }
}

ATLAS_FORCE_INLINE const char *yaw_class_label( yaw_class_t yaw_class )
{
    switch ( yaw_class )
    {
        case YAW_NETWORKED: return ATLAS_XORSTR_STATIC( "networked" );
        case YAW_LOWER_BODY: return ATLAS_XORSTR_STATIC( "lby" );
        case YAW_MOVING: return ATLAS_XORSTR_STATIC( "moving" );
        default: return ATLAS_XORSTR_STATIC( "unknown" );
    }
}

ATLAS_FORCE_INLINE shot_miss_t miss_reason( const shot_record_t &shot )
{
    if ( !shot.m_answer.m_impacted )
    {
        return SHOT_MISS_UNKNOWN;
    }

    vec3        direction{ shot.m_answer.m_impact - shot.m_aim.m_shot.m_start };
    const float length{ math::vector_normalize( direction ) };

    pose_crossing_t crossing{ };
    if ( !shot.m_aim.m_pose.cross( shot.m_aim.m_shot.m_start, direction, FLT_MAX, crossing ) )
    {
        return SHOT_MISS_SPREAD;
    }

    if ( crossing.m_entry > length )
    {
        return shot.m_answer.m_impacts == shot.m_aim.m_walls ? SHOT_MISS_OCCLUSION_OR_RESOLVER : SHOT_MISS_OCCLUSION;
    }

    return SHOT_MISS_RESOLVER;
}

ATLAS_FORCE_INLINE const char *miss_label( shot_miss_t miss )
{
    switch ( miss )
    {
        case SHOT_MISS_SPREAD: return ATLAS_XORSTR_STATIC( "spread" );
        case SHOT_MISS_OCCLUSION: return ATLAS_XORSTR_STATIC( "occlusion" );
        case SHOT_MISS_OCCLUSION_OR_RESOLVER: return ATLAS_XORSTR_STATIC( "occlusion or resolver" );
        case SHOT_MISS_RESOLVER: return ATLAS_XORSTR_STATIC( "resolver" );
        default: return ATLAS_XORSTR_STATIC( "unknown" );
    }
}

ATLAS_FORCE_INLINE const shot_hurt_t *target_hurt( const shot_record_t &shot )
{
    for ( i32 index{ }; index < shot.m_answer.m_hurt_count; ++index )
    {
        if ( shot.m_answer.m_hurts[ index ].m_victim == shot.m_aim.m_target )
        {
            return &shot.m_answer.m_hurts[ index ];
        }
    }

    return nullptr;
}

ATLAS_FORCE_INLINE void draw_tracer( const shot_record_t &shot )
{
    const config::bullet_tracer_t &settings{ g_cfg.m_bullet_tracer };
    if ( !settings.m_enabled || shot.m_state != SHOT_FIRED || !shot.m_answer.m_impacted )
    {
        return;
    }

    const color_t &color{ shot.m_answer.m_hurt_count > 0 ? settings.m_hit_color : settings.m_color };

    BeamInfo_t info{ };
    info.m_vecStart     = shot.m_shoot_position;
    info.m_vecEnd       = shot.m_answer.m_impact;
    info.m_pszModelName = ATLAS_XORSTR_STATIC( "sprites/laserbeam.vmt" );
    info.m_flLife       = settings.m_duration;
    info.m_flWidth      = 2.0f;
    info.m_flEndWidth   = 2.0f;
    info.m_flBrightness = static_cast< float >( color.a );
    info.m_flRed        = static_cast< float >( color.r );
    info.m_flGreen      = static_cast< float >( color.g );
    info.m_flBlue       = static_cast< float >( color.b );

    g_csgo.m_view_render_beams->CreateBeamPoints( info );
}

bool shot_log::install( )
{
    return g_csgo.m_game_events->AddListener( this, ATLAS_XORSTR( "weapon_fire" ).get( ), false ) &&
           g_csgo.m_game_events->AddListener( this, ATLAS_XORSTR( "bullet_impact" ).get( ), false ) &&
           g_csgo.m_game_events->AddListener( this, ATLAS_XORSTR( "player_hurt" ).get( ), false ) &&
           g_csgo.m_game_events->AddListener( this, ATLAS_XORSTR( "round_start" ).get( ), false );
}

void shot_log::stage( const shot_aim_t &aim )
{
    s_staged = aim;
}

void shot_log::on_command_simulated( i32 command, float curtime, C_CSPlayer *shooter, C_BaseCombatWeapon *weapon, float last_shot_before,
                                     i32 clip_before )
{
    this->m_predicted = command;

    if ( weapon->m_fLastShotTime( ) == last_shot_before || clip_before == 0 )
    {
        return;
    }

    const CCSWeaponInfo *data{ weapon->GetCSWpnData( ) };
    if ( !data || data->m_WeaponType == WEAPONTYPE_GRENADE )
    {
        return;
    }

    shot_record_t &shot{ this->push( ) };
    shot.m_weapon       = weapon;
    shot.m_weapon_index   = weapon->entindex( );
    shot.m_command        = command;
    shot.m_curtime        = curtime;
    shot.m_shoot_position = shooter->Weapon_ShootPosition( );
    shot.m_choked         = g_csgo.m_client_states[ 0 ]->chokedcommands;

    if ( s_staged.m_command != command )
    {
        return;
    }

    shot.m_aim = s_staged;
    shot.m_id  = ++this->m_last_id;

    player_info_t target{ };
    if ( g_csgo.m_engine->GetPlayerInfo( shot.m_aim.m_target, &target ) )
    {
        atl::memcpy( shot.m_name, target.name, sizeof( shot.m_name ) );
    }

    this->print_fire( shot );
}

void shot_log::on_post_data_update( )
{
    if ( !g_csgo.m_engine->IsInGame( ) )
    {
        this->clear( );
        g_nospread.clear( );
        return;
    }

    const i32 acknowledged{ g_csgo.m_client_states[ 0 ]->command_ack };

    for ( i32 offset{ }; offset < this->m_count; ++offset )
    {
        shot_record_t &shot{ this->at( offset ) };

        if ( shot.m_state != SHOT_PENDING ||
             shot.m_command > acknowledged ||
             g_csgo.m_entity_list->GetClientEntity( shot.m_weapon_index ) != shot.m_weapon )
        {
            continue;
        }

        if ( shot.m_weapon->m_fLastShotTime( ) < shot.m_curtime )
        {
            shot.m_state = SHOT_UNFIRED;
            shot.m_dead  = !g_cl.m_local || !g_cl.m_local->IsAlive( );
        }
    }
}

void shot_log::on_net_update_end( )
{
    this->pair( );

    while ( this->m_count > 0 && this->at( 0 ).m_state != SHOT_PENDING )
    {
        const shot_record_t &shot{ this->at( 0 ) };
        if ( shot.m_id )
        {
            this->print_result( shot );
        }

        if ( shot.m_id && shot.m_answer.m_impacted )
        {
            g_nospread.observe( shot.m_aim.m_shot, shot.m_answer.m_impact );
        }

        draw_tracer( shot );

        const bool pose_missed{ shot.m_id && shot.m_state == SHOT_FIRED && !target_hurt( shot ) && miss_reason( shot ) == SHOT_MISS_RESOLVER };
        if ( pose_missed )
        {
            g_ragebot.on_pose_miss( shot.m_aim.m_target );
        }

        if ( pose_missed && shot.m_aim.m_yaw_class == YAW_UNKNOWN )
        {
            g_resolver.on_miss( shot.m_aim.m_target, shot.m_aim.m_misses, shot.m_aim.m_standing );
        }

        this->m_head = ( this->m_head + 1 ) % MAX_SHOT_RECORDS;
        --this->m_count;
    }

    this->m_open = nullptr;
}

void shot_log::FireGameEvent( IGameEvent *event )
{
    if ( !g_cl.m_is_running )
    {
        return;
    }

    switch ( HASH_RT( event->GetName( ) ) )
    {
        case HASH_CT( "weapon_fire" ): this->on_weapon_fire( event ); break;
        case HASH_CT( "bullet_impact" ):
            this->on_bullet_impact( event );
            g_hitmarker.on_bullet_impact( event );
            break;
        case HASH_CT( "player_hurt" ):
            this->on_player_hurt( event );
            g_hitmarker.on_player_hurt( event );
            break;
        case HASH_CT( "round_start" ):
            this->clear( );
            g_player_data.on_round_start( );
            g_ragebot.on_round_start( );
            break;
        default: break;
    }
}

int shot_log::GetEventDebugID( )
{
    return 42;
}

shot_record_t &shot_log::at( i32 offset )
{
    return s_shots[ ( this->m_head + offset ) % MAX_SHOT_RECORDS ];
}

shot_answer_t &shot_log::answer_at( i32 offset )
{
    return s_answers[ ( this->m_answer_head + offset ) % MAX_SHOT_RECORDS ];
}

shot_record_t &shot_log::push( )
{
    if ( this->m_count == MAX_SHOT_RECORDS )
    {
        this->clear( );
    }

    shot_record_t &shot{ this->at( this->m_count ) };
    ++this->m_count;

    shot = shot_record_t{ };
    return shot;
}

void shot_log::pair( )
{
    while ( this->m_answer_count > 0 )
    {
        const shot_answer_t &answer{ this->answer_at( 0 ) };

        shot_record_t *waiting{ };
        for ( i32 offset{ }; offset < this->m_count && !waiting; ++offset )
        {
            shot_record_t &shot{ this->at( offset ) };
            if ( shot.m_state == SHOT_PENDING )
            {
                waiting = &shot;
            }
        }

        if ( !waiting && this->m_predicted < answer.m_ack )
        {
            return;
        }

        if ( waiting && waiting->m_command <= answer.m_ack )
        {
            waiting->m_answer = answer;
            waiting->m_state  = SHOT_FIRED;
        }

        this->m_answer_head = ( this->m_answer_head + 1 ) % MAX_SHOT_RECORDS;
        --this->m_answer_count;
    }
}

void shot_log::clear( )
{
    s_staged.m_command = 0;

    this->m_open         = nullptr;
    this->m_head         = 0;
    this->m_count        = 0;
    this->m_answer_head  = 0;
    this->m_answer_count = 0;
    this->m_predicted    = 0;
}

void shot_log::on_weapon_fire( IGameEvent *event )
{
    if ( event_player( event, ATLAS_XORSTR( "userid" ).get( ) ) != g_csgo.m_engine->GetLocalPlayer( ) )
    {
        return;
    }

    if ( this->m_answer_count == MAX_SHOT_RECORDS )
    {
        this->clear( );
    }

    shot_answer_t &answer{ this->answer_at( this->m_answer_count ) };
    ++this->m_answer_count;

    answer       = shot_answer_t{ };
    answer.m_ack = g_csgo.m_client_states[ 0 ]->command_ack;
    this->m_open = &answer;
}

void shot_log::on_bullet_impact( IGameEvent *event )
{
    if ( !this->m_open || event_player( event, ATLAS_XORSTR( "userid" ).get( ) ) != g_csgo.m_engine->GetLocalPlayer( ) )
    {
        return;
    }

    this->m_open->m_impacted = true;
    this->m_open->m_impact   = vec3{ event->GetFloat( ATLAS_XORSTR( "x" ).get( ) ),
                                     event->GetFloat( ATLAS_XORSTR( "y" ).get( ) ),
                                     event->GetFloat( ATLAS_XORSTR( "z" ).get( ) ) };
    ++this->m_open->m_impacts;
}

void shot_log::on_player_hurt( IGameEvent *event )
{
    if ( !this->m_open ||
         event_player( event, ATLAS_XORSTR( "attacker" ).get( ) ) != g_csgo.m_engine->GetLocalPlayer( ) ||
         this->m_open->m_hurt_count == MAX_SHOT_HURTS )
    {
        return;
    }

    shot_hurt_t &hurt{ this->m_open->m_hurts[ this->m_open->m_hurt_count++ ] };
    hurt.m_victim   = event_player( event, ATLAS_XORSTR( "userid" ).get( ) );
    hurt.m_hitgroup = event->GetInt( ATLAS_XORSTR( "hitgroup" ).get( ) );
    hurt.m_damage   = event->GetInt( ATLAS_XORSTR( "dmg_health" ).get( ) );
    hurt.m_health   = event->GetInt( ATLAS_XORSTR( "health" ).get( ) );
}

void shot_log::print( shot_line_kind_t kind, const char ( &text )[ SHOT_LINE_LENGTH ] ) const
{
    if ( g_cfg.m_ragebot.m_shot_log[ config::SHOT_LOG_CONSOLE ] )
    {
        g_csgo.m_Tier0_ConColorMsg_fn.get_as< void ( * )( const color_t &, const char *, ... ) >( )(
            SHOT_LINE_COLORS[ kind ], ATLAS_XORSTR( "[atlas] %s\n" ).get( ), text );
    }

    if ( kind == SHOT_LINE_FIRE || !g_cfg.m_ragebot.m_shot_log[ config::SHOT_LOG_NOTIFICATIONS ] )
    {
        return;
    }

    const bool hit{ kind == SHOT_LINE_HIT };
    push_notification( hit ? notify_kind_t::success : notify_kind_t::error, ICON_CROSSHAIR,
                       hit ? ATLAS_XORSTR_STATIC( "Hit" ) : ATLAS_XORSTR_STATIC( "Miss" ), text );
}

void shot_log::print_fire( const shot_record_t &shot )
{
    const shot_aim_t &aim{ shot.m_aim };

    char text[ SHOT_LINE_LENGTH ]{ };
    str::format_to( text, sizeof( text ), ATLAS_XORSTR( "#{} fired at {} ({}), hc {}%, dmg {}{}, lc {}t, yaw {}{}, walls {}, choked {}" ).get( ),
                    shot.m_id, shot.m_name, hitgroup_label( aim.m_hitgroup ), aim.m_hitchance, aim.m_damage,
                    aim.m_lethal ? ATLAS_XORSTR_STATIC( " lethal" ) : "", aim.m_backtrack, yaw_class_label( aim.m_yaw_class ),
                    aim.m_safe ? ATLAS_XORSTR_STATIC( ", safe" ) : "", aim.m_walls, shot.m_choked );

    this->print( SHOT_LINE_FIRE, text );
}

void shot_log::print_result( const shot_record_t &shot )
{
    char text[ SHOT_LINE_LENGTH ]{ };

    if ( shot.m_state == SHOT_UNFIRED )
    {
        str::format_to( text, sizeof( text ), ATLAS_XORSTR( "#{} was not fired by the server{}" ).get( ), shot.m_id,
                        shot.m_dead ? ATLAS_XORSTR_STATIC( ", we were already dead" ) : "" );
        this->print( SHOT_LINE_MISS, text );
        return;
    }

    const shot_hurt_t *hurt{ target_hurt( shot ) };
    if ( !hurt )
    {
        str::format_to( text, sizeof( text ), ATLAS_XORSTR( "#{} missed {} due to {}, impacts {}" ).get( ), shot.m_id, shot.m_name,
                        miss_label( miss_reason( shot ) ), shot.m_answer.m_impacts );
        this->print( SHOT_LINE_MISS, text );
        return;
    }

    const usize length{ str::format_to( text, sizeof( text ), ATLAS_XORSTR( "#{} hit {} ({}) for {}, {} left" ).get( ), shot.m_id,
                                        shot.m_name, hitgroup_label( hurt->m_hitgroup ), hurt->m_damage, hurt->m_health ) };

    if ( hurt->m_hitgroup != shot.m_aim.m_hitgroup )
    {
        str::format_to( text + length, sizeof( text ) - length, ATLAS_XORSTR( ", aimed {}" ).get( ),
                        hitgroup_label( shot.m_aim.m_hitgroup ) );
    }

    this->print( SHOT_LINE_HIT, text );
}
