#include "ragebot.hpp"

#include "shot_log.hpp"

ATLAS_NO_DESTROY ragebot g_ragebot{ };

constexpr float ACCURATE_SPEED_SCALE{ 0.34f };

constexpr float FULL_STOP_WISH{ 0.5f };

constexpr float  REVOLVER_EVENT_TIME{ 1.0f / 30.0f };
constexpr double REVOLVER_READY_DELAY{ 0.2 };

ATLAS_FORCE_INLINE usize weapon_group( i16 definition )
{
    switch ( definition )
    {
        case WEAPON_GLOCK:
        case WEAPON_HKP2000:
        case WEAPON_USP_SILENCER:
        case WEAPON_P250:
        case WEAPON_FIVESEVEN:
        case WEAPON_TEC9:
        case WEAPON_ELITE:
        case WEAPON_CZ75A: return config::RAGE_GROUP_PISTOL;
        case WEAPON_DEAGLE:
        case WEAPON_REVOLVER: return config::RAGE_GROUP_HEAVY_PISTOL;
        case WEAPON_MAC10:
        case WEAPON_MP9:
        case WEAPON_MP7:
        case WEAPON_UMP45:
        case WEAPON_P90:
        case WEAPON_BIZON: return config::RAGE_GROUP_SMG;
        case WEAPON_AK47:
        case WEAPON_M4A1:
        case WEAPON_M4A1_SILENCER:
        case WEAPON_GALILAR:
        case WEAPON_FAMAS:
        case WEAPON_AUG:
        case WEAPON_SG556: return config::RAGE_GROUP_RIFLE;
        case WEAPON_M249:
        case WEAPON_NEGEV: return config::RAGE_GROUP_MACHINE_GUN;
        case WEAPON_AWP: return config::RAGE_GROUP_AWP;
        case WEAPON_SSG08: return config::RAGE_GROUP_SCOUT;
        case WEAPON_G3SG1:
        case WEAPON_SCAR20: return config::RAGE_GROUP_AUTO;
        case WEAPON_TASER: return config::RAGE_GROUP_TASER;
        default: return config::RAGE_GROUP_COUNT;
    }
}

ATLAS_FORCE_INLINE const config::ragebot_profile_t *weapon_profile( C_BaseCombatWeapon *weapon )
{
    const i16   definition{ weapon->m_iItemDefinitionIndex( ) };
    const usize group{ weapon_group( definition ) };
    if ( group == config::RAGE_GROUP_COUNT )
    {
        return nullptr;
    }

    const config::ragebot_override_t &custom{ g_cfg.m_ragebot.m_overrides[ definition ] };
    return custom.m_enabled ? &custom.m_profile : &g_cfg.m_ragebot.m_groups[ group ];
}

ATLAS_FORCE_INLINE usize hitscan_index( i32 group )
{
    switch ( group )
    {
        case HITGROUP_HEAD: return config::HITSCAN_HEAD;
        case HITGROUP_CHEST: return config::HITSCAN_CHEST;
        case HITGROUP_STOMACH: return config::HITSCAN_STOMACH;
        case HITGROUP_LEFTARM:
        case HITGROUP_RIGHTARM: return config::HITSCAN_ARMS;
        case HITGROUP_LEFTLEG:
        case HITGROUP_RIGHTLEG: return config::HITSCAN_LEGS;
        default: return config::HITSCAN_COUNT;
    }
}

ATLAS_FORCE_INLINE bool weapon_ready( C_BaseCombatWeapon *weapon )
{
    const float curtime{ g_csgo.m_globals->curtime };

    if ( g_cl.m_local->m_flNextAttack( ) > curtime ||
         weapon->m_flNextPrimaryAttack( ) > curtime ||
         weapon->m_iClip1( ) <= 0 ||
         g_cl.m_local->m_bIsDefusing( ) ||
         g_cl.m_local->m_bWaitForNoAttack( ) )
    {
        return false;
    }

    return g_cl.m_local->m_iShotsFired( ) <= 0 || weapon->IsFullAuto( );
}

ATLAS_FORCE_INLINE bool can_fire( C_BaseCombatWeapon *weapon )
{
    if ( !weapon_ready( weapon ) )
    {
        return false;
    }

    if ( weapon->m_iItemDefinitionIndex( ) != WEAPON_REVOLVER )
    {
        return true;
    }

    return weapon->m_Activity( ) == ACT_VM_HAULBACK && g_csgo.m_globals->curtime > weapon->m_flPostponeFireReadyTime( );
}

ATLAS_FORCE_INLINE bool is_valid_target( C_CSPlayer *player )
{
    if ( player->IsDormant( ) ||
         !player->IsAlive( ) ||
         player->m_bGunGameImmunity( ) ||
         !g_cl.m_local->IsOtherEnemy( player ) )
    {
        return false;
    }

    return true;
}

ATLAS_FORCE_INLINE bool within_fov( const vec3 &shoot_pos, const vec3 &point )
{
    if ( g_cfg.m_ragebot.m_fov >= 180 )
    {
        return true;
    }

    vec3 angles{ };
    math::vector_angles( point - shoot_pos, angles );

    return math::calc_fov( g_cl.m_cmd->viewangles, angles ) <= static_cast< float >( g_cfg.m_ragebot.m_fov );
}

ATLAS_FORCE_INLINE i32 hitbox_points( const pose_hitbox_t &box, const vec3 &eye, float scale, vec3 *points )
{
    const vec3 middle{ ( box.m_start + box.m_end ) * 0.5f };

    if ( box.m_radius <= 0.0f )
    {
        points[ 0 ] = math::transform_point( middle, box.m_transform );
        return 1;
    }

    points[ 0 ] = middle;

    if ( scale <= 0.0f )
    {
        return 1;
    }

    vec3 axis{ box.m_end - box.m_start };
    math::vector_normalize( axis );

    vec3 side{ ( middle - eye ).cross( axis ) };
    math::vector_normalize( side );

    const float reach{ box.m_radius * scale };

    points[ 1 ] = middle + side * reach;
    points[ 2 ] = middle - side * reach;

    if ( box.m_group != HITGROUP_HEAD )
    {
        return 3;
    }

    points[ 3 ] = box.m_end.z > box.m_start.z ? box.m_end + axis * reach : box.m_start - axis * reach;
    return 4;
}

ATLAS_FORCE_INLINE float falloff_damage( const bullet_t &bullet, float distance )
{
    if ( distance > bullet.m_range * 0.75f )
    {
        return bullet.m_damage;
    }

    const float counted{ distance * bullet.m_range / ( bullet.m_range + PLAYER_RAY_EXTENSION ) };
    return bullet.m_damage * math::pow( bullet.m_range_modifier, counted * 0.002f );
}

ATLAS_FORCE_INLINE i32 hitgroup_bound( const armor_t &armor, const bullet_t &bullet, float damage, u32 groups )
{
    i32 bound{ };

    for ( ; groups != 0; groups &= groups - 1 )
    {
        const i32 group{ __builtin_ctz( groups ) };
        bound = math::bigger( bound, g_autowall.scale_damage( armor, damage, bullet.m_armor_ratio, group, bullet.m_shock ) );
    }

    return bound;
}

ATLAS_FORCE_INLINE i32 needed_damage( const ragebot_target_t &best, bool safe, i32 health, i32 required )
{
    if ( safe && !best.m_safe )
    {
        return required;
    }

    return math::bigger( required, math::smaller( health, best.m_damage + 1 ) );
}

ATLAS_FORCE_INLINE bool outranks( bool safe, i32 bound, const scan_point_t &other )
{
    return safe != other.m_safe ? safe : bound > other.m_bound;
}

ATLAS_FORCE_INLINE spread_t weapon_spread( C_BaseCombatWeapon *weapon )
{
    spread_t spread{ };
    spread.m_inaccuracy   = weapon->GetInaccuracy( );
    spread.m_spread       = weapon->GetSpread( );
    spread.m_recoil_index = weapon->m_flRecoilIndex( );
    spread.m_negev        = weapon->m_iItemDefinitionIndex( ) == WEAPON_NEGEV;
    spread.m_patterns     = g_csgo.weapon_accuracy_shotgun_spread_patterns->GetInt( ) != 0;

    return spread;
}

ATLAS_FORCE_INLINE bool compensate_spread( const vec3 &point, fired_shot_t &shot )
{
    if ( !g_cfg.m_ragebot.m_nospread || !g_nospread.synced( ) )
    {
        return false;
    }

    const vec2 offset{ g_nospread.offset( shot.m_seed & ( SPREAD_SEEDS - 1 ), shot.m_spread ) };

    vec3 angles{ };
    if ( !g_nospread.compensate( point - shot.m_start, offset, angles ) )
    {
        return false;
    }

    if ( math::abs( angles.x - g_cl.m_local->GetAimPunchAngleScaled( ).x ) > 89.0f )
    {
        return false;
    }

    shot.m_angles = angles;
    return true;
}

ATLAS_FORCE_INLINE void shift_bones( const scan_record_t &record, matrix3x4 *bones )
{
    const lag_record_t *entry{ record.m_entry };
    const vec3          shift{ record.m_origin - entry->m_origin };

    atl::memcpy( bones, entry->m_bones, sizeof( matrix3x4 ) * entry->m_bone_count );

    for ( i32 bone{ }; bone < entry->m_bone_count; ++bone )
    {
        bones[ bone ][ 0 ][ 3 ] += shift.x;
        bones[ bone ][ 1 ][ 3 ] += shift.y;
        bones[ bone ][ 2 ][ 3 ] += shift.z;
    }
}

ATLAS_FORCE_INLINE bool seed_hits( const scan_record_t &record, bool safe, const vec3 &start, const vec3 &direction, float length )
{
    if ( !record.m_hitboxes.hits( start, direction, length ) )
    {
        return false;
    }

    for ( i32 edge{ }; safe && edge < record.m_edge_count; ++edge )
    {
        if ( !record.m_edges[ edge ].hits( start, direction, length ) )
        {
            return false;
        }
    }

    return true;
}

ATLAS_FORCE_INLINE i32 spread_hits( const scan_record_t &record, bool safe, const bullet_t &bullet, const vec3 &angles, const spread_t &spread,
                                    i32 needed )
{
    vec3 forward{ };
    vec3 right{ };
    vec3 up{ };
    math::angle_vectors( angles, &forward, &right, &up );

    i32 hits{ };
    for ( i32 seed{ }; seed < SPREAD_SEEDS && hits + SPREAD_SEEDS - seed >= needed; ++seed )
    {
        const vec2 offset{ g_nospread.offset( seed, spread ) };

        vec3 direction{ forward + right * offset.x + up * offset.y };
        math::vector_normalize( direction );

        hits += seed_hits( record, safe, bullet.m_start, direction, bullet.m_range );
    }

    return hits;
}

ATLAS_FORCE_INLINE void capture_pose( C_CSPlayer *player, trace_pose_t &pose )
{
    pose.m_origin       = player->GetAbsOrigin( );
    pose.m_mins         = player->m_vecMins( );
    pose.m_maxs         = player->m_vecMaxs( );
    pose.m_bone_count   = player->m_CachedBoneCount( );
    pose.m_bone_counter = player->m_iMostRecentModelBoneCounter( );
    pose.m_bone_time    = player->m_flLastBoneSetupTime( );

    atl::memcpy( pose.m_bones, player->m_CachedBoneData( ), sizeof( matrix3x4 ) * pose.m_bone_count );
}

ATLAS_FORCE_INLINE void apply_pose( C_CSPlayer *player, const trace_pose_t &pose )
{
    player->SetAbsOrigin( pose.m_origin );
    player->m_vecMins( ) = pose.m_mins;
    player->m_vecMaxs( ) = pose.m_maxs;

    atl::memcpy( player->m_CachedBoneData( ), pose.m_bones, sizeof( matrix3x4 ) * pose.m_bone_count );
    player->m_iMostRecentModelBoneCounter( ) = pose.m_bone_counter;
    player->m_flLastBoneSetupTime( )         = pose.m_bone_time;
}

ATLAS_FORCE_INLINE float accurate_speed( C_BaseCombatWeapon *weapon )
{
    return ACCURATE_SPEED_SCALE * g_cl.m_local->GetMaxSpeed( weapon );
}

ATLAS_FORCE_INLINE float movement_frametime( )
{
    return TICK_INTERVAL * g_cl.m_local->m_flLaggedMovementValue( );
}

ATLAS_FORCE_INLINE float friction_step( float speed )
{
    const float control{ math::bigger( speed, g_csgo.sv_stopspeed->GetFloat( ) ) };
    const float drop{ control * ( g_cl.m_local->m_surfaceFriction( ) * g_csgo.sv_friction->GetFloat( ) ) * movement_frametime( ) };

    return math::bigger( speed - drop, 0.0f );
}

ATLAS_FORCE_INLINE float stop_deceleration( C_BaseCombatWeapon *weapon )
{
    const CCSWeaponInfo *info{ weapon->GetCSWpnData( ) };
    const float          max_speed{ info->m_flMaxSpeed[ weapon->m_weaponMode( ) != 0 ? 1 : 0 ] };
    const bool           zoomed_sniper{ weapon->m_zoomLevel( ) > 0 && info->m_iZoomLevels > 1 && max_speed * 0.52f < 110.0f };
    const bool           ducking{ ( g_cl.m_cmd->buttons & IN_DUCK ) || ( g_cl.m_local->m_fFlags( ) & FL_DUCKING ) };

    float scale{ 250.0f };
    if ( ducking && !zoomed_sniper )
    {
        scale *= 0.34f;
    } else if ( g_csgo.sv_accelerate_use_weapon_speed->GetInt( ) )
    {
        scale *= math::smaller( max_speed * 0.004f, 1.0f );
    }

    return movement_frametime( ) * g_csgo.sv_accelerate->GetFloat( ) * scale * g_cl.m_local->m_surfaceFriction( );
}

ATLAS_FORCE_INLINE float horizontal_speed( )
{
    const vec3 &velocity{ g_cl.m_local->m_vecVelocity( ) };
    return math::sqrt( velocity.x * velocity.x + velocity.y * velocity.y );
}

ATLAS_FORCE_INLINE bool stops_within( C_BaseCombatWeapon *weapon, i32 commands )
{
    const float target{ accurate_speed( weapon ) };
    const float deceleration{ stop_deceleration( weapon ) };
    float       speed{ horizontal_speed( ) };

    for ( i32 command{ }; command < commands; ++command )
    {
        const float slowed{ friction_step( speed ) };
        if ( slowed <= target )
        {
            return true;
        }

        speed = slowed - deceleration;
        if ( speed <= target )
        {
            return true;
        }
    }

    return false;
}

ATLAS_FORCE_INLINE i32 ticks_until_fire( C_BaseCombatWeapon *weapon )
{
    const float next{ math::bigger( g_cl.m_local->m_flNextAttack( ), weapon->m_flNextPrimaryAttack( ) ) };
    i32         ticks{ math::bigger( static_cast< i32 >( math::ceil( next / TICK_INTERVAL ) ) - g_cl.m_local->m_nTickBase( ), 0 ) };

    if ( !weapon->IsFullAuto( ) && g_cl.m_local->m_iShotsFired( ) > 0 )
    {
        ticks = math::bigger( ticks, 1 );
    }

    return ticks;
}

ATLAS_FORCE_INLINE bool can_stop( )
{
    if ( !g_cl.m_local->IsOnGround( ) ||
         g_cl.m_local->m_MoveType( ) != MOVETYPE_WALK ||
         g_cl.m_local->m_nWaterLevel( ) >= 2 ||
         ( g_cl.m_cmd->buttons & IN_JUMP ) ||
         g_cl.m_cmd->weaponselect != 0 )
    {
        return false;
    }

    return true;
}

ATLAS_FORCE_INLINE void write_stop( C_BaseCombatWeapon *weapon, usize mode )
{
    CUserCmd   *cmd{ g_cl.m_cmd };
    const vec3 &velocity{ g_cl.m_local->m_vecVelocity( ) };
    const float speed{ horizontal_speed( ) };
    const float target{ mode == config::AUTOSTOP_FULL ? 0.0f : accurate_speed( weapon ) };

    cmd->buttons &= ~IN_SPEED;

    if ( friction_step( speed ) <= target )
    {
        cmd->forwardmove = 0.0f;
        cmd->sidemove    = 0.0f;
        return;
    }

    const float wish{ mode == config::AUTOSTOP_FULL ? FULL_STOP_WISH : target };
    const float yaw{ DEG2RAD( cmd->viewangles.y ) };
    const float x{ -velocity.x / speed };
    const float y{ -velocity.y / speed };

    cmd->forwardmove = wish * ( x * math::cos( yaw ) + y * math::sin( yaw ) );
    cmd->sidemove    = wish * ( x * math::sin( yaw ) - y * math::cos( yaw ) );
}

void ragebot::think( )
{
    C_BaseCombatWeapon *weapon{ g_cl.m_local->m_hActiveWeapon( ).Get( ) };
    if ( !this->should_run( weapon ) )
    {
        return;
    }

    if ( weapon->m_iItemDefinitionIndex( ) == WEAPON_REVOLVER && g_cfg.m_ragebot.m_auto_shoot )
    {
        const bool cocked{ this->cock_revolver( weapon ) };
        if ( !cocked )
        {
            return;
        }
    }

    const bool ready{ can_fire( weapon ) };
    if ( !ready && !this->m_stopping )
    {
        return;
    }

    bullet_t bullet{ };
    if ( !g_autowall.setup( g_cl.m_local, bullet ) )
    {
        return;
    }

    const rewind_window_t window{ g_lagcomp.rewind_window( g_cl.m_local->m_nTickBase( ) ) };

    ragebot_target_t target{ };
    if ( !this->find_target( bullet, window, target ) )
    {
        return;
    }

    if ( !ready )
    {
        this->m_keep_stop = true;
        return;
    }

    weapon->UpdateAccuracyPenalty( );

    fired_shot_t shot{ weapon_spread( weapon ), bullet.m_start, { }, g_cl.m_cmd->random_seed };
    math::vector_angles( target.m_point - bullet.m_start, shot.m_angles );

    const bool compensated{ compensate_spread( target.m_point, shot ) };
    if ( compensated )
    {
        this->aim_at_target( target, shot, SPREAD_SEEDS, window );
        this->m_keep_stop = true;
        return;
    }

    const i32 needed{ static_cast< i32 >( math::ceil( static_cast< float >( this->m_profile->m_hitchance * SPREAD_SEEDS ) * 0.01f ) ) };
    const i32 hits{ spread_hits( *target.m_record, target.m_safe, bullet, shot.m_angles, shot.m_spread, needed ) };
    if ( hits >= needed )
    {
        this->aim_at_target( target, shot, hits, window );
        this->m_keep_stop = true;
        return;
    }

    const bool scoping{ this->scope_in( weapon ) };
    if ( scoping )
    {
        this->m_keep_stop = true;
        return;
    }

    spread_t standing{ shot.m_spread };
    standing.m_inaccuracy = math::smaller( weapon->m_fAccuracyPenalty( ), 1.0f );

    this->m_keep_stop = spread_hits( *target.m_record, target.m_safe, bullet, shot.m_angles, standing, needed ) >= needed;
}

void ragebot::begin_command( )
{
    this->m_stopping  = false;
    this->m_keep_stop = false;

    C_BaseCombatWeapon *weapon{ g_cl.m_local->m_hActiveWeapon( ).Get( ) };
    if ( !this->should_run( weapon ) || this->m_profile->m_autostop == config::AUTOSTOP_OFF )
    {
        return;
    }

    if ( !can_stop( ) || ( weapon->m_iClip1( ) <= 0 && weapon->m_iPrimaryReserveAmmoCount( ) <= 0 ) )
    {
        return;
    }

    const i32 wait{ ticks_until_fire( weapon ) };
    if ( wait > 0 && ( !this->m_profile->m_autostop_early || stops_within( weapon, wait ) ) )
    {
        return;
    }

    this->m_forwardmove = g_cl.m_cmd->forwardmove;
    this->m_sidemove    = g_cl.m_cmd->sidemove;
    this->m_buttons     = g_cl.m_cmd->buttons;

    write_stop( weapon, this->m_profile->m_autostop );
    this->m_stopping = true;
}

void ragebot::end_command( )
{
    if ( !this->m_stopping || this->m_keep_stop )
    {
        return;
    }

    g_cl.m_cmd->forwardmove = this->m_forwardmove;
    g_cl.m_cmd->sidemove    = this->m_sidemove;
    g_cl.m_cmd->buttons     = ( g_cl.m_cmd->buttons & ~IN_SPEED ) | ( this->m_buttons & IN_SPEED );
}

void ragebot::on_simulated( C_BaseCombatWeapon *weapon, i32 activity, float curtime ) const
{
    if ( weapon->m_iItemDefinitionIndex( ) != WEAPON_REVOLVER ||
         activity == ACT_VM_HAULBACK ||
         weapon->m_Activity( ) != ACT_VM_HAULBACK )
    {
        return;
    }

    const i32   advances{ static_cast< i32 >( REVOLVER_EVENT_TIME / TICK_INTERVAL ) + 1 };
    const float event{ TICKS_TO_TIME( TIME_TO_TICKS( curtime ) + advances - 1 ) };

    weapon->m_flPostponeFireReadyTime( ) = static_cast< float >( static_cast< double >( event ) + REVOLVER_READY_DELAY );
}

bool ragebot::uses_safe_point( ) const
{
    if ( !g_cfg.m_ragebot.m_enabled )
    {
        return false;
    }

    C_BaseCombatWeapon              *weapon{ g_cl.m_local->m_hActiveWeapon( ).Get( ) };
    const config::ragebot_profile_t *profile{ weapon ? weapon_profile( weapon ) : nullptr };

    return profile && profile->m_safe_point != config::SAFE_POINT_OFF;
}

void ragebot::on_pose_miss( i32 index )
{
    this->m_pose_missed[ index ] = true;
}

void ragebot::on_round_start( )
{
    atl::memset( this->m_pose_missed, 0, sizeof( this->m_pose_missed ) );
}

bool ragebot::should_run( C_BaseCombatWeapon *weapon )
{
    if ( !g_cfg.m_ragebot.m_enabled || !gui::bind_active( g_cfg.m_ragebot.m_activate_key ) )
    {
        return false;
    }

    if ( g_gui.open( ) || !weapon )
    {
        return false;
    }

    this->m_profile = weapon_profile( weapon );
    if ( !this->m_profile )
    {
        return false;
    }

    if ( !g_cfg.m_ragebot.m_auto_shoot && !( g_cl.m_cmd->buttons & IN_ATTACK ) )
    {
        return false;
    }

    return true;
}

bool ragebot::cock_revolver( C_BaseCombatWeapon *weapon ) const
{
    i32        &buttons{ g_cl.m_cmd->buttons };
    const float curtime{ g_csgo.m_globals->curtime };

    if ( !weapon_ready( weapon ) )
    {
        buttons &= ~( IN_ATTACK | IN_ATTACK2 );
        return false;
    }

    if ( weapon->m_Activity( ) == ACT_VM_HAULBACK && weapon->m_flPostponeFireReadyTime( ) == FLT_MAX )
    {
        buttons &= ~( IN_ATTACK | IN_ATTACK2 );
        return false;
    }

    if ( weapon->m_Activity( ) != ACT_VM_HAULBACK || curtime <= weapon->m_flPostponeFireReadyTime( ) )
    {
        buttons |= IN_ATTACK;
        buttons &= ~IN_ATTACK2;
        return false;
    }

    buttons &= ~IN_ATTACK;

    if ( curtime < weapon->m_flNextSecondaryAttack( ) )
    {
        buttons |= IN_ATTACK2;
    } else
    {
        buttons &= ~IN_ATTACK2;
    }

    return true;
}

struct scan_job_t
{
    ragebot        *m_ragebot{ };
    const bullet_t *m_bullet{ };
};

bool ragebot::find_target( const bullet_t &bullet, const rewind_window_t &window, ragebot_target_t &best )
{
    this->collect_records( window );

    scan_job_t job{ this, &bullet };
    if ( !g_thread_pool.run( &ragebot::build_record_job, &job, this->m_record_count ) )
    {
        return false;
    }

    for ( i32 index{ }; index < this->m_record_count && !( best.m_lethal && best.m_safe ); ++index )
    {
        this->fire_record( this->m_records[ index ], bullet, best );
    }

    return best.m_record != nullptr;
}

void ragebot::collect_records( const rewind_window_t &window )
{
    this->m_record_count = 0;

    const i32 highest{ math::smaller( g_csgo.m_globals->maxClients, MAX_TRACKED_PLAYERS - 1 ) };

    for ( i32 index{ 1 }; index <= highest; ++index )
    {
        IClientEntity *entity{ g_csgo.m_entity_list->GetClientEntity( index ) };
        if ( !entity )
        {
            continue;
        }

        auto *player{ entity->As< C_CSPlayer * >( ) };
        if ( !player->IsAlive( ) )
        {
            this->m_pose_missed[ index ] = false;
        }

        if ( !is_valid_target( player ) )
        {
            continue;
        }

        i32 newest{ -1 };
        i32 oldest{ -1 };
        i32 exact{ -1 };
        i32 moving{ -1 };

        for ( i32 age{ }; age < g_lagcomp.count( index ); ++age )
        {
            const lag_record_t *entry{ g_lagcomp.record( index, age ) };
            if ( entry->m_bone_count <= 0 || !g_lagcomp.reachable( index, *entry, window ) )
            {
                continue;
            }

            if ( newest < 0 )
            {
                newest = age;
            }

            if ( exact < 0 && ( entry->m_yaw_class == YAW_NETWORKED || entry->m_yaw_class == YAW_LOWER_BODY ) )
            {
                exact = age;
            }

            if ( moving < 0 && entry->m_yaw_class == YAW_MOVING )
            {
                moving = age;
            }

            oldest = age;
        }

        if ( newest < 0 )
        {
            continue;
        }

        if ( exact < 0 )
        {
            exact = moving;
        }

        if ( exact >= 0 )
        {
            this->add_record( player, index, exact, window );
        }

        if ( newest != exact )
        {
            this->add_record( player, index, newest, window );
        }

        if ( oldest != newest && oldest != exact )
        {
            this->add_record( player, index, oldest, window );
        }
    }
}

void ragebot::add_record( C_CSPlayer *player, i32 index, i32 age, const rewind_window_t &window )
{
    const lag_record_t       *entry{ g_lagcomp.record( index, age ) };
    const mstudiohitboxset_t *set{ entry->m_header->pHitboxSet( player->m_nHitboxSet( ) ) };
    if ( !set || player->m_CachedBoneCount( ) != entry->m_bone_count )
    {
        return;
    }

    scan_record_t &record{ this->m_records[ this->m_record_count++ ] };
    record.m_player   = player;
    record.m_entry    = entry;
    record.m_set      = set;
    record.m_origin   = g_lagcomp.rewound_origin( index, age, window );
    record.m_armor    = g_autowall.armor_of( player );
    record.m_health   = player->m_iHealth( );
    record.m_required = math::smaller( this->m_profile->m_min_damage, record.m_health );

    const usize mode{ this->m_profile->m_safe_point };
    record.m_safe_point = mode != config::SAFE_POINT_OFF;
    record.m_safe_only  = mode == config::SAFE_POINT_FORCE || ( record.m_safe_point && this->m_pose_missed[ index ] );
    record.m_edge_count = record.m_safe_point && entry->m_has_edges ? DESYNC_EDGE_COUNT : 0;
}

void ragebot::build_record( scan_record_t &record, matrix3x4 *bones, const bullet_t &bullet ) const
{
    shift_bones( record, bones );
    record.m_hitboxes.build( record.m_set, bones );

    for ( i32 edge{ }; edge < record.m_edge_count; ++edge )
    {
        record.m_edges[ edge ] = record.m_entry->m_edges[ edge ];
        record.m_edges[ edge ].shift( record.m_origin - record.m_entry->m_origin );
    }

    const float nearest{ falloff_damage( bullet, record.m_hitboxes.distance( bullet.m_start ) ) };
    if ( hitgroup_bound( record.m_armor, bullet, nearest, 1u << HITGROUP_HEAD ) < record.m_required )
    {
        record.m_count = 0;
        return;
    }

    record.m_count = this->gather_points( record, bullet, record.m_points );
}

void ragebot::build_record_job( void *context, i32 index, i32 worker )
{
    const scan_job_t &job{ *static_cast< const scan_job_t * >( context ) };
    ragebot          &self{ *job.m_ragebot };

    self.build_record( self.m_records[ index ], self.m_scratch[ worker ], *job.m_bullet );
}

i32 ragebot::gather_points( const scan_record_t &record, const bullet_t &bullet, scan_point_t *points ) const
{
    const pose_hitboxes_t &hitboxes{ record.m_hitboxes };

    const bool guessed{ record.m_entry->m_yaw_class == YAW_UNKNOWN && !record.m_entry->m_yaw_bounded };

    i32 count{ };

    for ( i32 index{ }; index < hitboxes.m_count; ++index )
    {
        const pose_hitbox_t &box{ hitboxes.m_boxes[ index ] };
        const usize          scan{ hitscan_index( box.m_group ) };

        if ( scan == config::HITSCAN_COUNT ||
             !this->m_profile->m_hitscan_boxes[ scan ] ||
             ( box.m_group == HITGROUP_HEAD && box.m_radius <= 0.0f ) )
        {
            continue;
        }

        const i32   percent{ box.m_group == HITGROUP_HEAD ? this->m_profile->m_head_scale : this->m_profile->m_body_scale };
        const float scale{ this->m_profile->m_multipoint[ scan ] ? static_cast< float >( percent ) * 0.01f : 0.0f };

        vec3      candidates[ MAX_HITBOX_POINTS ]{ };
        const i32 made{ hitbox_points( box, bullet.m_start, scale, candidates ) };

        for ( i32 at{ }; at < made; ++at )
        {
            vec3 direction{ candidates[ at ] - bullet.m_start };
            math::vector_normalize( direction );

            pose_crossing_t crossing{ };
            if ( !hitboxes.cross( bullet.m_start, direction, bullet.m_range, crossing ) )
            {
                continue;
            }

            i32 bound{ hitgroup_bound( record.m_armor, bullet, falloff_damage( bullet, crossing.m_entry ), crossing.m_groups ) };

            bool safe{ !record.m_safe_point };
            if ( record.m_edge_count > 0 && !( guessed && box.m_group == HITGROUP_HEAD ) )
            {
                i32  least{ bound };
                bool crossed{ true };

                for ( i32 edge{ }; edge < record.m_edge_count && crossed; ++edge )
                {
                    pose_crossing_t edged{ };
                    crossed = record.m_edges[ edge ].cross( bullet.m_start, direction, bullet.m_range, edged );
                    if ( crossed )
                    {
                        least = math::smaller( least, hitgroup_bound( record.m_armor, bullet, falloff_damage( bullet, edged.m_entry ), edged.m_groups ) );
                    }
                }

                safe = crossed && least >= record.m_required;
                if ( safe )
                {
                    bound = least;
                }
            }

            if ( ( record.m_safe_only && !safe ) || bound < record.m_required || !within_fov( bullet.m_start, candidates[ at ] ) )
            {
                continue;
            }

            i32 slot{ count++ };
            for ( ; slot > 0 && outranks( safe, bound, points[ slot - 1 ] ); --slot )
            {
                points[ slot ] = points[ slot - 1 ];
            }

            points[ slot ] = scan_point_t{ candidates[ at ], bound, safe };
        }
    }

    return count;
}

void ragebot::fire_record( const scan_record_t &record, const bullet_t &bullet, ragebot_target_t &best )
{
    if ( record.m_count == 0 ||
         ( best.m_safe && !record.m_points[ 0 ].m_safe ) ||
         record.m_points[ 0 ].m_bound < needed_damage( best, record.m_points[ 0 ].m_safe, record.m_health, record.m_required ) )
    {
        return;
    }

    this->pose_record( record );
    this->pin_record( record.m_player );

    for ( i32 index{ }; index < record.m_count && !( best.m_lethal && best.m_safe ); ++index )
    {
        const scan_point_t &point{ record.m_points[ index ] };
        if ( best.m_safe && !point.m_safe )
        {
            break;
        }

        const i32 needed{ needed_damage( best, point.m_safe, record.m_health, record.m_required ) };
        if ( point.m_bound < needed )
        {
            continue;
        }

        bullet_t aimed{ bullet };
        aimed.m_direction = point.m_position - bullet.m_start;

        bullet_hit_t hit{ };
        if ( !g_autowall.fire( g_cl.m_local, aimed, record.m_player, hit ) )
        {
            continue;
        }

        const i32 damage{ record.m_edge_count > 0 && point.m_safe ? math::smaller( hit.m_damage, point.m_bound ) : hit.m_damage };
        if ( damage < needed )
        {
            continue;
        }

        best = ragebot_target_t{ &record, point.m_position, damage, hit.m_hitgroup, hit.m_wall_count, damage >= record.m_health, point.m_safe };
    }

    this->unpin_record( record.m_player );
}

bool ragebot::scope_in( C_BaseCombatWeapon *weapon ) const
{
    if ( !this->m_profile->m_autoscope ||
         weapon->GetCSWpnData( )->m_iZoomLevels <= 0 ||
         weapon->m_zoomLevel( ) > 0 ||
         weapon->m_flNextSecondaryAttack( ) > g_csgo.m_globals->curtime )
    {
        return false;
    }

    g_cl.m_cmd->buttons |= IN_ATTACK2;
    g_cl.m_cmd->buttons &= ~IN_ATTACK;
    return true;
}

void ragebot::aim_at_target( const ragebot_target_t &target, const fired_shot_t &shot, i32 hits, const rewind_window_t &window ) const
{
    vec3 view{ shot.m_angles - g_cl.m_local->GetAimPunchAngleScaled( ) };
    math::clamp_angles( view );

    const scan_record_t &record{ *target.m_record };
    const i32            index{ record.m_player->entindex( ) };

    g_cl.m_cmd->viewangles = view;
    g_cl.m_cmd->tick_count = window.command_tick( record.m_entry->m_tick );

    if ( g_cfg.m_ragebot.m_auto_shoot )
    {
        g_cl.m_cmd->buttons |= IN_ATTACK;
    }

    shot_aim_t aim{ };
    aim.m_command   = g_cl.m_cmd->command_number;
    aim.m_target    = index;
    aim.m_hitgroup  = target.m_hitgroup;
    aim.m_damage    = target.m_damage;
    aim.m_hitchance = hits * 100 / SPREAD_SEEDS;
    aim.m_backtrack = g_lagcomp.record( index, 0 )->m_tick - record.m_entry->m_tick;
    aim.m_yaw_class = record.m_entry->m_yaw_class;
    aim.m_misses    = record.m_entry->m_misses;
    aim.m_standing  = record.m_entry->m_standing;
    aim.m_walls     = target.m_walls;
    aim.m_lethal    = target.m_lethal;
    aim.m_safe      = target.m_safe && record.m_edge_count > 0;
    aim.m_shot      = shot;
    aim.m_pose      = record.m_hitboxes;

    g_shot_log.stage( aim );
}

void ragebot::pose_record( const scan_record_t &record )
{
    shift_bones( record, this->m_pose.m_bones );

    this->m_pose.m_origin     = record.m_origin;
    this->m_pose.m_mins       = record.m_entry->m_mins;
    this->m_pose.m_maxs       = record.m_entry->m_maxs;
    this->m_pose.m_bone_count = record.m_entry->m_bone_count;
}

void ragebot::pin_record( C_CSPlayer *player )
{
    capture_pose( player, this->m_held );

    this->m_pose.m_bone_counter = *g_csgo.m_model_bone_counter;
    this->m_pose.m_bone_time    = this->m_held.m_bone_time;

    apply_pose( player, this->m_pose );
}

void ragebot::unpin_record( C_CSPlayer *player )
{
    apply_pose( player, this->m_held );
}
