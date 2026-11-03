#include "anti_aim.hpp"

#include "anims.hpp"
#include "bone_setup.hpp"

#include "math/math.hpp"

ATLAS_NO_DESTROY anti_aim g_anti_aim{ };

constexpr float LOWER_BODY_REALIGN_ANGLE{ 35.0f };

ATLAS_FORCE_INLINE float bin_yaw( i32 bin )
{
    return static_cast< float >( bin ) * ( 360.0f / HIDE_HEAD_BINS ) - 180.0f;
}

ATLAS_FORCE_INLINE vec3 spin( const vec3 &point, const vec3 &centre, float degrees )
{
    const float s{ math::sin( DEG2RAD( degrees ) ) };
    const float c{ math::cos( DEG2RAD( degrees ) ) };
    const float x{ point.x - centre.x };
    const float y{ point.y - centre.y };

    return vec3{ centre.x + x * c - y * s, centre.y + x * s + y * c, point.z };
}

ATLAS_FORCE_INLINE bool is_guessed( float yaw, float toward )
{
    for ( i32 quarter{ }; quarter < 4; ++quarter )
    {
        if ( math::abs( math::normalize_yaw( yaw - toward - 90.0f * static_cast< float >( quarter ) ) ) <= LOWER_BODY_REALIGN_ANGLE )
        {
            return true;
        }
    }

    return false;
}

ATLAS_FORCE_INLINE bool is_world_clear( const vec3 &from, const vec3 &to )
{
    trace_t                       trace{ };
    Ray_t                         ray{ };
    CTraceFilterWorldAndPropsOnly filter{ };

    ray.Init( from, to );
    g_csgo.m_trace->TraceRay( ray, CS_MASK_SHOOT, &filter, &trace );

    return trace.fraction == 1.0f;
}

void anti_aim::think( )
{
    if ( !this->should_run( ) )
    {
        return;
    }

    const config::anti_hack_t        &config{ g_cfg.m_anti_hack };
    const config::anti_aim_profile_t &profile{ config.m_anti_aim_profiles[ this->movement_state( ) ] };
    CUserCmd                         *cmd{ g_cl.m_cmd };

    const bool batch_start{ g_csgo.m_client_states[ 0 ]->chokedcommands == 0 };
    const bool faking{ profile.m_fake_yaw != config::FAKE_YAW_NONE };
    if ( faking && batch_start )
    {
        g_cl.m_send_packet = false;
    }

    this->m_view = cmd->viewangles;
    this->fetch_threats( );
    const float base{ this->base_yaw( profile.m_yaw_base ) };

    switch ( profile.m_pitch )
    {
        case config::PITCH_DOWN: cmd->viewangles.x = 89.0f; break;
        case config::PITCH_UP: cmd->viewangles.x = -89.0f; break;
        case config::PITCH_ZERO: cmd->viewangles.x = 0.0f; break;
        default: break;
    }

    bool breaker{ };
    if ( batch_start || !faking )
    {
        this->m_real      = this->plan_real( profile, base );
        breaker           = this->breaks_lower_body( );
        cmd->viewangles.y = breaker ? math::normalize_yaw( this->m_real + static_cast< float >( config.m_lower_body_offset ) ) : this->m_real;
    } else
    {
        cmd->viewangles.y = this->plan_fake( profile, base );
    }

#if defined( ATLAS_DEV )
    if ( g_cfg.m_debug.m_anti_aim )
    {
        DBGOUT( "AA cmd=%d tick=%d start=%d send=%d state=%d real=%.4f yaw=%.4f pitch=%.4f breaker=%d timer=%.6f curtime=%.6f exposure=%d threats=%d",
                cmd->command_number, g_cl.m_local->m_nTickBase( ), batch_start ? 1 : 0, g_cl.m_send_packet ? 1 : 0,
                static_cast< i32 >( this->movement_state( ) ), this->m_real, cmd->viewangles.y, cmd->viewangles.x, breaker ? 1 : 0,
                g_anims.local_state( ).m_lower_body_realign_time, g_csgo.m_globals->curtime, this->m_exposure, this->m_threat_count );
    }
#endif // ATLAS_DEV
}

bool anti_aim::should_run( ) const
{
    config::anti_hack_t &config{ g_cfg.m_anti_hack };
    if ( !config.m_anti_aim_enabled || !gui::bind_active( config.m_anti_aim_key ) )
    {
        return false;
    }

    C_CSPlayer *local{ g_cl.m_local };
    if ( local->m_MoveType( ) == MOVETYPE_NOCLIP || local->m_MoveType( ) == MOVETYPE_LADDER )
    {
        return false;
    }

    const i32 buttons{ g_cl.m_cmd->buttons };
    if ( buttons & ( IN_ATTACK | IN_USE ) )
    {
        return false;
    }

    C_BaseCombatWeapon  *weapon{ local->m_hActiveWeapon( ).Get( ) };
    const CCSWeaponInfo *info{ weapon ? weapon->GetCSWpnData( ) : nullptr };
    if ( !info )
    {
        return true;
    }

    if ( info->m_WeaponType == WEAPONTYPE_KNIFE && ( buttons & IN_ATTACK2 ) )
    {
        return false;
    }

    return info->m_WeaponType != WEAPONTYPE_GRENADE || ( !( buttons & IN_ATTACK2 ) && weapon->m_fThrowTime( ) == 0.0f );
}

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L2353-L2377
usize anti_aim::movement_state( ) const
{
    if ( !g_cl.m_local->IsOnGround( ) )
    {
        return config::ANTI_AIM_STATE_AIR;
    }

    return g_cl.m_local->m_vecVelocity( ).length2d( ) > 0.1f ? config::ANTI_AIM_STATE_MOVE : config::ANTI_AIM_STATE_STAND;
}

void anti_aim::fetch_threats( )
{
    this->m_threat_count = 0;

    const vec3 eye{ g_cl.m_local->Weapon_ShootPosition( ) };
    for ( i32 index{ 1 }; index <= g_csgo.m_globals->maxClients; ++index )
    {
        IClientEntity *entity{ g_csgo.m_entity_list->GetClientEntity( index ) };
        if ( !entity || index == g_csgo.m_engine->GetLocalPlayer( ) )
        {
            continue;
        }

        C_CSPlayer *player{ entity->As< C_CSPlayer * >( ) };
        if ( player->IsDormant( ) || !player->IsAlive( ) || !g_cl.m_local->IsOtherEnemy( player ) )
        {
            continue;
        }

        anti_aim_threat_t threat{ player->Weapon_ShootPosition( ), 0.0f };
        vec3              angles{ };
        math::vector_angles( threat.m_eye - eye, angles );
        threat.m_fov = math::calc_fov( this->m_view, angles );

        i32 slot{ this->m_threat_count };
        while ( slot > 0 && this->m_threats[ slot - 1 ].m_fov > threat.m_fov )
        {
            if ( slot < HIDE_HEAD_THREATS )
            {
                this->m_threats[ slot ] = this->m_threats[ slot - 1 ];
            }
            --slot;
        }

        if ( slot < HIDE_HEAD_THREATS )
        {
            this->m_threats[ slot ] = threat;
            this->m_threat_count    = math::smaller( this->m_threat_count + 1, HIDE_HEAD_THREATS );
        }
    }
}

float anti_aim::base_yaw( usize base ) const
{
    if ( base != config::YAW_BASE_AT_TARGET || !this->m_threat_count )
    {
        return this->m_view.y;
    }

    vec3 angles{ };
    math::vector_angles( this->m_threats[ 0 ].m_eye - g_cl.m_local->Weapon_ShootPosition( ), angles );
    return angles.y;
}

float anti_aim::plan_real( const config::anti_aim_profile_t &profile, float base )
{
    const float wanted{ math::normalize_yaw( base + static_cast< float >( profile.m_real_offset ) ) };
    switch ( profile.m_real_yaw )
    {
        case config::REAL_YAW_HIDE_HEAD: return this->hide_head( wanted );

        case config::REAL_YAW_JITTER:
        {
            this->m_real_flip = !this->m_real_flip;
            const float half{ static_cast< float >( profile.m_jitter_range ) * 0.5f };
            return math::normalize_yaw( wanted + ( this->m_real_flip ? half : -half ) );
        }

        case config::REAL_YAW_SPIN:
        {
            this->m_spin = math::normalize_yaw( this->m_spin + static_cast< float >( profile.m_spin_speed ) );
            return math::normalize_yaw( wanted + this->m_spin );
        }

        default: return wanted;
    }
}

float anti_aim::plan_fake( const config::anti_aim_profile_t &profile, float base )
{
    const float wanted{ math::normalize_yaw( base + static_cast< float >( profile.m_fake_offset ) ) };
    switch ( profile.m_fake_yaw )
    {
        case config::FAKE_YAW_OPPOSITE: return math::normalize_yaw( this->m_real + 180.0f + static_cast< float >( profile.m_fake_offset ) );

        case config::FAKE_YAW_JITTER:
        {
            if ( g_cl.m_send_packet )
            {
                this->m_fake_flip = !this->m_fake_flip;
            }

            const float half{ static_cast< float >( profile.m_fake_range ) * 0.5f };
            return math::normalize_yaw( wanted + ( this->m_fake_flip ? half : -half ) );
        }

        case config::FAKE_YAW_RANDOM:
        {
            CUniformRandomStream random{ };
            random.Construct( );
            random.SetSeed( g_cl.m_cmd->random_seed );
            const float range{ static_cast< float >( profile.m_fake_range ) };
            return math::normalize_yaw( wanted + random.RandomFloat( -range, range ) );
        }

        case config::FAKE_YAW_STATIC: return wanted;

        default: return this->m_real;
    }
}

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L2353-L2377
bool anti_aim::breaks_lower_body( ) const
{
    if ( !g_cfg.m_anti_hack.m_break_lower_body )
    {
        return false;
    }

    return g_cl.m_local->m_PlayerAnimState( )->m_bOnGround( ) && g_cl.m_local->m_vecVelocity( ).length2d( ) <= 0.1f &&
           g_csgo.m_globals->curtime > g_anims.local_state( ).m_lower_body_realign_time;
}

float anti_aim::hide_head( float preferred )
{
    this->m_exposure = 0;
    if ( !this->m_threat_count )
    {
        return preferred;
    }

    C_CSPlayer *local{ g_cl.m_local };
    if ( !g_bone_mgr.player_setup( local, this->m_bones, MAXSTUDIOBONES ) )
    {
        return preferred;
    }

    const mstudiohitboxset_t *set{ local->m_pStudioHdr( )->m_pStudioHdr->pHitboxSet( local->m_nHitboxSet( ) ) };
    if ( !set )
    {
        return preferred;
    }

    pose_hitboxes_t hitboxes{ };
    hitboxes.build( set, this->m_bones );

    const pose_hitbox_t *head{ };
    for ( i32 index{ }; index < hitboxes.m_count && !head; ++index )
    {
        if ( hitboxes.m_boxes[ index ].m_group == HITGROUP_HEAD )
        {
            head = &hitboxes.m_boxes[ index ];
        }
    }

    if ( !head )
    {
        return preferred;
    }

    const vec3  points[ HEAD_SCAN_POINTS ]{ head->m_start, ( head->m_start + head->m_end ) * 0.5f, head->m_end };
    const float built{ local->m_PlayerAnimState( )->m_flEyeYaw( ) };

    vec3 toward{ };
    math::vector_angles( this->m_threats[ 0 ].m_eye - local->GetAbsOrigin( ), toward );
    const bool avoid{ g_cfg.m_anti_hack.m_avoid_guesses };

    float best_yaw{ preferred };
    i32   best{ this->head_exposure( hitboxes, points, math::normalize_yaw( preferred - built ) ) };
    bool  best_guessed{ avoid && is_guessed( preferred, toward.y ) };
    float best_distance{ };
    for ( i32 bin{ }; bin < HIDE_HEAD_BINS; ++bin )
    {
        const float yaw{ bin_yaw( bin ) };
        const i32   exposure{ this->head_exposure( hitboxes, points, math::normalize_yaw( yaw - built ) ) };
        const bool  guessed{ avoid && is_guessed( yaw, toward.y ) };
        const float distance{ math::abs( math::normalize_yaw( yaw - preferred ) ) };
        if ( exposure < best || ( exposure == best && ( guessed < best_guessed || ( guessed == best_guessed && distance < best_distance ) ) ) )
        {
            best          = exposure;
            best_yaw      = yaw;
            best_guessed  = guessed;
            best_distance = distance;
        }
    }

    this->m_exposure = best;
    return best_yaw;
}

i32 anti_aim::head_exposure( const pose_hitboxes_t &hitboxes, const vec3 *points, float turn ) const
{
    const vec3 origin{ g_cl.m_local->GetAbsOrigin( ) };

    i32 exposed{ };
    for ( i32 index{ }; index < this->m_threat_count; ++index )
    {
        const vec3 &eye{ this->m_threats[ index ].m_eye };
        const vec3  turned_eye{ spin( eye, origin, -turn ) };
        for ( i32 point{ }; point < HEAD_SCAN_POINTS; ++point )
        {
            if ( !is_world_clear( eye, spin( points[ point ], origin, turn ) ) )
            {
                continue;
            }

            const vec3  delta{ points[ point ] - turned_eye };
            const float length{ delta.length( ) };
            if ( hitboxes.first_group( turned_eye, delta / length, length ) == HITGROUP_HEAD )
            {
                ++exposed;
            }
        }
    }

    return exposed;
}
