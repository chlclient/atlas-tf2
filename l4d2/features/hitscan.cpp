#include "hitscan.hpp"

ATLAS_NO_DESTROY hitscan g_hitscan{ };

static constexpr i32 TRACE_BUDGET{ 128 };

ATLAS_FORCE_INLINE vec3 angle_to( const vec3 &from, const vec3 &to )
{
    vec3 angles{ };
    math::vector_angles( to - from, angles );
    return angles;
}

ATLAS_FORCE_INLINE void set_attack( bool pressed )
{
    CUserCmd       *cmd{ g_cl.m_cmd };
    C_TerrorPlayer *local{ g_cl.m_local };

    cmd->buttons         = pressed ? ( cmd->buttons | IN_ATTACK ) : ( cmd->buttons & ~IN_ATTACK );
    local->m_nButtons( ) = ( local->m_nButtons( ) & ~IN_ATTACK ) | ( cmd->buttons & IN_ATTACK );
}

ATLAS_FORCE_INLINE usize hitscan_group( i32 hitgroup )
{
    switch ( hitgroup )
    {
        case HITGROUP_HEAD: return config::HITSCAN_HEAD;
        case HITGROUP_CHEST: return config::HITSCAN_CHEST;
        case HITGROUP_STOMACH: return config::HITSCAN_STOMACH;
        case HITGROUP_LEFTARM:
        case HITGROUP_RIGHTARM: return config::HITSCAN_ARMS;
        case HITGROUP_LEFTLEG:
        case HITGROUP_RIGHTLEG: return config::HITSCAN_LEGS;

        default: return config::HITSCAN_GROUP_COUNT;
    }
}

ATLAS_FORCE_INLINE bool is_target( C_BaseEntity *entity )
{
    switch ( g_cl.aim_target( entity ) )
    {
        case static_cast< usize >( config::esp_target_t::enemy ): return g_cfg.m_hitscan.m_players;
        case config::TARGET_COMMON: return g_cfg.m_hitscan.m_common;
        case config::TARGET_WITCH: return g_cfg.m_hitscan.m_witch;

        default: return false;
    }
}

ATLAS_FORCE_INLINE bool is_burst_shot_due( C_Rifle_Desert *scar, float curtime )
{
    const float shots[]{ scar->m_flBurstShot1( ), scar->m_flBurstShot2( ) };

    for ( const float shot : shots )
    {
        if ( shot > 0.0f && curtime > shot )
        {
            return true;
        }
    }

    return false;
}

bool hitscan::active( ) const
{
    if ( !g_cfg.m_hitscan.m_enabled )
    {
        return false;
    }

    return gui::bind_active( g_cfg.m_hitscan.m_activate_key );
}

bool hitscan::think( C_TerrorGun *gun )
{
    if ( !this->should_run( gun ) )
    {
        return false;
    }

    this->m_eye    = g_cl.m_local->m_vecAbsOrigin( ) + g_cl.m_local->m_vecViewOffset( );
    this->m_window = g_lagcomp.window( );
    this->m_budget = TRACE_BUDGET;

    this->fetch_targets( gun->GetWeaponInfo( ) );

    hitscan_aim_t aim{ };
    if ( !this->get_best_target( aim ) )
    {
        return false;
    }

    if ( this->should_release( gun ) )
    {
        set_attack( false );
        return false;
    }

    this->aim_at_target( aim );
    return true;
}

void hitscan::commit( bool fired )
{
    if ( fired )
    {
        this->m_shot        = this->m_pending;
        this->m_shot.m_time = g_l4d2.m_globals->realtime;

        // the shot rides in the next packet behind a normal command, so the eye angles the server
        // networks for this packet are the real ones
        if ( g_cfg.m_hitscan.m_psilent &&
             g_l4d2.m_split_screen->GetClientState( 0 )->chokedcommands < MAX_NEW_COMMANDS - 1 )
        {
            g_cl.m_send_packet = false;
        }

        return;
    }

    CUserCmd *cmd{ g_cl.m_cmd };

    cmd->viewangles  = this->m_saved.m_viewangles;
    cmd->forwardmove = this->m_saved.m_forwardmove;
    cmd->sidemove    = this->m_saved.m_sidemove;
    cmd->buttons     = this->m_saved.m_buttons;
    cmd->tick_count  = this->m_saved.m_tick_count;
}

void hitscan::paint( ) const
{
    if ( !g_cfg.m_hitscan.m_enabled )
    {
        return;
    }

    this->paint_fov_circle( );
    this->paint_shot( );
}

bool hitscan::should_run( C_TerrorGun *gun ) const
{
    if ( !this->active( ) ||      // the hitscan is off or its key is up
         !this->can_fire( gun ) ) // this command cannot shoot
    {
        return false;
    }

    return true;
}

bool hitscan::can_fire( C_TerrorGun *gun ) const
{
    const float curtime{ g_l4d2.m_globals->curtime };

    if ( gun->m_iClip1( ) <= 0 )
    {
        return false;
    }

    if ( gun->GetClientClass( )->GetClassID( ) == EL4D2ClassID::CRifle_Desert )
    {
        auto *scar{ gun->As< C_Rifle_Desert * >( ) };

        // the rest of a burst leaves on its own timers whatever the buttons say
        if ( is_burst_shot_due( scar, curtime ) )
        {
            return true;
        }

        if ( scar->m_flBurstResetTime( ) > curtime )
        {
            return false;
        }
    }

    return gun->m_flNextPrimaryAttack( ) <= curtime;
}

bool hitscan::should_release( C_TerrorGun *gun ) const
{
    // a held button blocks the next shot of a non automatic gun, so it has to be let go for one command
    if ( !gun->m_bFireAllowed( ) )
    {
        return true;
    }

    if ( gun->m_bHoldingFire( ) && !gun->IsFullyAutomatic( ) )
    {
        return true;
    }

    return false;
}

void hitscan::fetch_targets( CTerrorWeaponInfo *info )
{
    C_TerrorPlayer *local{ g_cl.m_local };
    const vec3     &view{ g_cl.m_cmd->viewangles };
    const f32       fov{ static_cast< f32 >( g_cfg.m_hitscan.m_fov ) };

    this->m_target_count = 0;

    const i32 highest{ g_l4d2.m_entity_list->GetHighestEntityIndex( ) };
    for ( i32 index{ 1 }; index <= highest && this->m_target_count < MAX_HITSCAN_TARGETS; ++index )
    {
        IClientEntity *client_entity{ g_l4d2.m_entity_list->GetClientEntity( index ) };
        if ( !client_entity )
        {
            continue;
        }

        C_BaseEntity *entity{ client_entity->As< C_BaseEntity * >( ) };
        if ( entity == local ||      // that's us
             entity->IsDormant( ) || // no fresh data to aim with
             !is_target( entity ) )  // the target settings leave it out
        {
            continue;
        }

        const vec3 center{ entity->m_vecOrigin( ) + ( entity->m_vecMins( ) + entity->m_vecMaxs( ) ) * 0.5f };
        const f32  distance{ ( center - this->m_eye ).length( ) };
        if ( distance > info->m_flRange( ) )
        {
            continue;
        }

        // the box center can sit outside the fov while a hitbox on the same entity is inside it
        const f32 radius{ ( entity->m_vecMaxs( ) - entity->m_vecMins( ) ).length( ) * 0.5f };
        const f32 center_fov{ math::calc_fov( view, angle_to( this->m_eye, center ) ) };
        if ( center_fov > fov + RAD2DEG( math::atan2( radius, distance ) ) )
        {
            continue;
        }

        this->m_targets[ this->m_target_count++ ] = hitscan_target_t{ entity, center_fov };
    }

    atl::sort( this->m_targets, this->m_targets + this->m_target_count, []( const hitscan_target_t &first, const hitscan_target_t &second )
    {
        return first.m_fov < second.m_fov;
    } );
}

bool hitscan::get_best_target( hitscan_aim_t &aim )
{
    const vec3 &view{ g_cl.m_cmd->viewangles };
    const f32   fov{ static_cast< f32 >( g_cfg.m_hitscan.m_fov ) };

    pose_t pose{ };

    for ( i32 index{ }; index < this->m_target_count; ++index )
    {
        C_BaseEntity *entity{ this->m_targets[ index ].m_entity };

        if ( !this->build_pose( entity, pose ) )
        {
            continue;
        }

        if ( !this->get_target_aim( entity, pose, aim ) )
        {
            continue;
        }

        if ( math::calc_fov( view, angle_to( this->m_eye, aim.m_point ) ) > fov )
        {
            continue;
        }

        this->remember_shot( pose, aim );
        return true;
    }

    return false;
}

bool hitscan::build_pose( C_BaseEntity *entity, pose_t &pose ) const
{
    auto *animating{ entity->As< C_BaseAnimating * >( ) };

    if ( !animating->SetupBones( pose.m_bones, BONE_USED_BY_HITBOX ) )
    {
        return false;
    }

    pose.m_hitboxes = animating->GetStudioHdr( )->pHitboxSet( animating->m_nHitboxSet( ) );
    pose.m_origin   = entity->m_vecAbsOrigin( );

    return true;
}

bool hitscan::rewind_target( C_BaseEntity *entity, lag_record_t &server ) const
{
    const i32 index{ entity->entindex( ) };

    if ( g_lagcomp.compensated( entity ) &&
         g_lagcomp.rewind( index, this->m_window.target( g_cl.m_cmd->tick_count ), server ) )
    {
        return true;
    }

    g_lagcomp.live( index, this->m_window.m_curtime, server );
    return false;
}

bool hitscan::get_target_aim( C_BaseEntity *entity, const pose_t &pose, hitscan_aim_t &aim )
{
    // the drawn pose is taken as the server's pose at the command's own tick, only moved to where the server puts it
    lag_record_t server{ pose.m_origin };
    const bool   rewound{ this->rewind_target( entity, server ) };

    if ( this->get_aimpoint( entity, pose, hitscan_placement_t{ pose.m_origin, server.m_origin }, aim ) )
    {
        aim.m_tick_count = g_cl.m_cmd->tick_count;
        return true;
    }

    if ( !rewound || !g_cfg.m_hitscan.m_backtrack )
    {
        return false;
    }

    return this->get_backtrack_aim( entity, pose, server, aim );
}

bool hitscan::get_backtrack_aim( C_BaseEntity *entity, const pose_t &pose, const lag_record_t &server, hitscan_aim_t &aim )
{
    const i32 index{ entity->entindex( ) };

    for ( i32 age{ }; age < g_lagcomp.count( index ); ++age )
    {
        const i32    tick{ g_lagcomp.command_tick( g_lagcomp.record( index, age )->m_simtime ) };
        lag_record_t record{ };

        if ( tick == g_cl.m_cmd->tick_count ||                                   // already tried as the current pose
             !this->m_window.accepts( tick ) ||                                  // the server would not honour it
             !g_lagcomp.rewind( index, this->m_window.target( tick ), record ) ) // the server would not rewind to it
        {
            continue;
        }

        const f32                 turn{ DEG2RAD( math::normalize_yaw( record.m_yaw - server.m_yaw ) ) };
        const hitscan_placement_t placement{ pose.m_origin, record.m_origin, math::sin( turn ), math::cos( turn ) };

        if ( this->get_aimpoint( entity, pose, placement, aim ) )
        {
            aim.m_tick_count = tick;
            return true;
        }
    }

    return false;
}

bool hitscan::get_aimpoint( C_BaseEntity *entity, const pose_t &pose, const hitscan_placement_t &placement, hitscan_aim_t &aim )
{
    const config::hitscan_t &cfg{ g_cfg.m_hitscan };

    for ( usize pass{ }; pass <= config::HITSCAN_GROUP_COUNT; ++pass )
    {
        const usize group{ pass == 0 ? cfg.m_priority : pass - 1 };
        if ( pass != 0 && ( group == cfg.m_priority || !cfg.m_hitboxes[ group ] ) )
        {
            continue;
        }

        for ( i32 index{ }; index < pose.m_hitboxes->numhitboxes; ++index )
        {
            mstudiobbox_t *box{ pose.m_hitboxes->pHitbox( index ) };
            if ( hitscan_group( box->group ) != group )
            {
                continue;
            }

            vec3 center{ };
            math::vector_transform( ( box->bbmin + box->bbmax ) * 0.5f, pose.m_bones[ box->bone ], center );

            const vec3 point{ placement.apply( center ) };
            if ( !this->is_visible( entity, point ) )
            {
                continue;
            }

            aim.m_point     = point;
            aim.m_placement = placement;
            aim.m_hitbox    = index;
            return true;
        }
    }

    return false;
}

bool hitscan::is_visible( C_BaseEntity *entity, const vec3 &point )
{
    if ( this->m_budget <= 0 )
    {
        return false;
    }

    --this->m_budget;

    CTraceFilterSkipEntity filter{ g_cl.m_local, entity };
    CGameTrace             trace{ };

    g_l4d2.m_engine_trace->TraceRay( Ray_t{ this->m_eye, point }, MASK_SHOT, &filter, &trace );

    return trace.fraction >= 1.0f;
}

void hitscan::aim_at_target( const hitscan_aim_t &aim )
{
    CUserCmd  *cmd{ g_cl.m_cmd };
    const vec3 original{ cmd->viewangles };

    this->m_saved = cmd_state_t{ cmd->viewangles, cmd->forwardmove, cmd->sidemove, cmd->buttons, cmd->tick_count };

    vec3 angles{ angle_to( this->m_eye, aim.m_point ) - g_cl.m_local->m_vecPunchAngle( ) };
    math::clamp_angles( angles );

    cmd->viewangles = angles;
    cmd->tick_count = aim.m_tick_count;

    g_cl.fix_movement( cmd, original );
    set_attack( true );
}

void hitscan::remember_shot( const pose_t &pose, const hitscan_aim_t &aim )
{
    shot_draw_t &draw{ this->m_pending };

    if ( !g_cfg.m_hitscan.m_draw_hitboxes && !g_cfg.m_hitscan.m_draw_shot )
    {
        draw.m_boxes = 0;
        return;
    }

    draw.m_boxes     = math::smaller( pose.m_hitboxes->numhitboxes, MAX_DRAW_BOXES );
    draw.m_aimed     = aim.m_hitbox;
    draw.m_shoot_pos = this->m_eye;
    draw.m_aim_point = aim.m_point;

    for ( i32 index{ }; index < draw.m_boxes; ++index )
    {
        const mstudiobbox_t *box{ pose.m_hitboxes->pHitbox( index ) };

        for ( i32 corner{ }; corner < 8; ++corner )
        {
            vec3 world{ };
            math::vector_transform( math::box_corner( box->bbmin, box->bbmax, corner ), pose.m_bones[ box->bone ], world );

            draw.m_corners[ index ][ corner ] = aim.m_placement.apply( world );
        }
    }
}

float hitscan::shot_fade( ) const
{
    if ( this->m_shot.m_boxes == 0 )
    {
        return 0.0f;
    }

    const float age{ g_l4d2.m_globals->realtime - this->m_shot.m_time };
    return math::bigger( 1.0f - age / math::bigger( g_cfg.m_hitscan.m_draw_duration, 0.1f ), 0.0f );
}

void hitscan::paint_fov_circle( ) const
{
    if ( !g_cfg.m_hitscan.m_draw_fov )
    {
        return;
    }

    const f32 half_cone{ static_cast< f32 >( g_cfg.m_hitscan.m_fov ) };
    const f32 half_view{ g_cl.m_view_fov * 0.5f };

    if ( half_cone >= 90.0f || half_view <= 0.0f || half_view >= 90.0f )
    {
        return;
    }

    const f32 width{ static_cast< f32 >( g_cl.m_screen_width ) };
    const f32 height{ static_cast< f32 >( g_cl.m_screen_height ) };
    const f32 radius{ math::tan( DEG2RAD( half_cone ) ) / math::tan( DEG2RAD( half_view ) ) * ( width * 0.5f ) };

    g_draw.circle( vec2{ width * 0.5f, height * 0.5f }, g_cfg.m_hitscan.m_fov_color, radius, 1.5f );
}

void hitscan::paint_shot( ) const
{
    const config::hitscan_t &cfg{ g_cfg.m_hitscan };
    const shot_draw_t       &shot{ this->m_shot };
    const f32                alpha{ this->shot_fade( ) };

    if ( alpha <= 0.0f )
    {
        return;
    }

    const auto faded{ [ alpha ]( color_t value )
    {
        value.a = static_cast< u8 >( static_cast< f32 >( value.a ) * alpha );
        return value;
    } };

    if ( cfg.m_draw_hitboxes )
    {
        for ( i32 index{ }; index < shot.m_boxes; ++index )
        {
            const vec3   *corners{ shot.m_corners[ index ] };
            const color_t value{ faded( index == shot.m_aimed ? cfg.m_target_color : cfg.m_hitbox_color ) };

            for ( i32 corner{ }; corner < 8; ++corner )
            {
                for ( i32 axis{ }; axis < 3; ++axis )
                {
                    const i32 neighbour{ corner ^ ( 1 << axis ) };
                    if ( neighbour > corner )
                    {
                        g_cl.draw_3d_line( corners[ corner ], corners[ neighbour ], value );
                    }
                }
            }
        }
    }

    if ( cfg.m_draw_shot )
    {
        g_cl.draw_3d_line( shot.m_shoot_pos, shot.m_aim_point, faded( cfg.m_shot_color ) );
    }
}
