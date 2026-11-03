#include "misc.hpp"

#include "gui/gui.hpp"

ATLAS_NO_DESTROY misc g_misc{ };

void misc::on_create_move( )
{
    this->bhop( );
    this->auto_strafe( );
    this->slow_walk( );
}

void misc::bhop( )
{
    if ( !g_cfg.m_misc.m_bhop ||
         g_cl.m_local->m_MoveType( ) != MOVETYPE_WALK ||
         g_cl.m_local->m_nWaterLevel( ) > 1 )
    {
        return;
    }

    if ( !g_cl.m_local->IsOnGround( ) )
    {
        g_cl.m_cmd->buttons &= ~IN_JUMP;
    }
}

void misc::auto_strafe( )
{
    if ( !g_cfg.m_misc.m_auto_strafe || g_cl.m_local->m_MoveType( ) != MOVETYPE_WALK ||
         g_cl.m_local->IsOnGround( ) || g_cl.m_local->m_nWaterLevel( ) > 1 )
    {
        return;
    }

    const bool  follow_wasd{ g_cfg.m_misc.m_auto_strafe_mode == 1 };
    const float forward_move{ g_cl.m_cmd->forwardmove };
    const float side_move{ g_cl.m_cmd->sidemove };

    if ( follow_wasd && forward_move == 0.0f && side_move == 0.0f )
    {
        return;
    }

    vec3 velocity{ g_cl.m_local->m_vecVelocity( ) };
    velocity.z = 0.0f;

    const float speed{ velocity.length2d( ) };
    const float velocity_yaw{ RAD2DEG( math::atan2( velocity.y, velocity.x ) ) };
    const float view_yaw{ g_cl.m_cmd->viewangles.y };

    const float wish_speed{ math::smaller( MAX_MOVE, g_cl.m_local->m_flMaxspeed( ) ) };
    const float max_air_accel{ math::smaller( AIR_WISH_SPEED_CAP, g_csgo.sv_airaccelerate->GetFloat( ) * wish_speed * g_csgo.m_globals->interval_per_tick ) };

    if ( max_air_accel <= 0.f || speed <= max_air_accel )
    {
        return;
    }

    const float gain_offset{ RAD2DEG( math::asin( max_air_accel * 0.5f / speed ) ) };
    const float turn_max{ RAD2DEG( math::asin( max_air_accel / speed ) ) };

    float wish_yaw{ view_yaw };
    float turn_limit{ turn_max };

    if ( follow_wasd )
    {
        wish_yaw   = math::normalize_yaw( view_yaw + RAD2DEG( math::atan2( -side_move, forward_move ) ) );
        turn_limit = turn_max * math::clamp( g_cfg.m_misc.m_auto_strafe_retrack / 100.0f, 0.0f, 1.0f );
    }

    this->m_strafe_side = this->m_strafe_side > 0.0f ? -1.0f : 1.0f;

    const float wish_delta{ math::normalize_yaw( wish_yaw - velocity_yaw ) };

    float push_yaw{ };

    if ( math::abs( wish_delta ) > turn_limit )
    {
        push_yaw = velocity_yaw + ( wish_delta > 0.0f ? 1.0f : -1.0f ) * ( 90.0f + turn_limit );
    } else
    {
        push_yaw = wish_yaw + this->m_strafe_side * ( 90.0f - gain_offset );
    }

    const float push_delta{ DEG2RAD( math::normalize_yaw( push_yaw - view_yaw ) ) };

    g_cl.m_cmd->forwardmove = MAX_MOVE * math::cos( push_delta );
    g_cl.m_cmd->sidemove    = -MAX_MOVE * math::sin( push_delta );
}

void misc::slow_walk( )
{
    C_BaseCombatWeapon *weapon{ g_cl.m_local->m_hActiveWeapon( ).Get( ) };
    if ( !g_cfg.m_misc.m_slow_walk || !gui::bind_active( g_cfg.m_misc.m_slow_walk_key ) || !weapon ||
         g_cl.m_local->m_MoveType( ) != MOVETYPE_WALK || !g_cl.m_local->IsOnGround( ) )
    {
        return;
    }

    CUserCmd   *cmd{ g_cl.m_cmd };
    const float wish{ math::sqrt( cmd->forwardmove * cmd->forwardmove + cmd->sidemove * cmd->sidemove ) };
    const float limit{ g_cl.m_local->GetMaxSpeed( weapon ) * static_cast< float >( g_cfg.m_misc.m_slow_walk_speed ) / 100.0f };
    if ( wish <= limit )
    {
        return;
    }

    cmd->forwardmove *= limit / wish;
    cmd->sidemove *= limit / wish;
}

void misc::leg_movement( )
{
    const usize mode{ g_cfg.m_anti_hack.m_leg_movement };
    if ( mode == config::LEG_MOVEMENT_DEFAULT || g_cl.m_local->m_MoveType( ) != MOVETYPE_WALK )
    {
        return;
    }

    CUserCmd *cmd{ g_cl.m_cmd };
    cmd->buttons &= ~( IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT );

    if ( mode == config::LEG_MOVEMENT_NEVER_SLIDE )
    {
        return;
    }

    if ( cmd->forwardmove > 0.0f )
    {
        cmd->buttons |= IN_BACK;
    } else if ( cmd->forwardmove < 0.0f )
    {
        cmd->buttons |= IN_FORWARD;
    }

    if ( cmd->sidemove > 0.0f )
    {
        cmd->buttons |= IN_MOVELEFT;
    } else if ( cmd->sidemove < 0.0f )
    {
        cmd->buttons |= IN_MOVERIGHT;
    }
}

void misc::paint( ) const
{
    if ( !g_cfg.m_misc.m_removals[ config::REMOVAL_SCOPE_OVERLAY ] || !g_csgo.m_engine->IsInGame( ) || !g_cl.m_local ||
         !g_cl.m_local->IsAlive( ) || !g_cl.m_local->m_bIsScoped( ) )
    {
        return;
    }

    C_BaseCombatWeapon *weapon{ g_cl.m_local->m_hActiveWeapon( ).Get( ) };
    if ( !weapon || weapon->GetCSWpnData( )->m_WeaponType != WEAPONTYPE_SNIPER_RIFLE )
    {
        return;
    }

    const i32 width{ g_cl.m_screen_width };
    const i32 height{ g_cl.m_screen_height };

    g_draw.line( point_t{ 0, height / 2 }, point_t{ width, height / 2 }, color_t{ 0, 0, 0 } );
    g_draw.line( point_t{ width / 2, 0 }, point_t{ width / 2, height }, color_t{ 0, 0, 0 } );
}

void misc::on_override_view( CViewSetup *setup, const vec3 &eye )
{
    if ( !g_csgo.m_engine->IsInGame( ) || !g_cl.m_local )
    {
        return;
    }

    this->remove_visual_recoil( setup );
    this->fov( setup );
    this->thirdperson( setup, eye );
}

void misc::on_post_data_update( )
{
    if ( !g_cfg.m_misc.m_removals[ config::REMOVAL_FLASH ] || !g_cl.m_local )
    {
        return;
    }

    g_cl.m_local->m_flFlashMaxAlpha( ) = 0.0f;
}

void misc::on_render_start( )
{
    const i32 wanted{ g_cfg.m_misc.m_removals[ config::REMOVAL_POST_FX ] ? 0 : 1 };

    if ( g_csgo.mat_postprocess_enable->GetInt( ) != wanted )
    {
        g_csgo.mat_postprocess_enable->SetValue( wanted );
    }

    this->force_crosshair( );
}

void misc::force_crosshair( )
{
    const bool wanted{ g_cfg.m_misc.m_force_crosshair && g_csgo.m_engine->IsInGame( ) && g_cl.m_local &&
                       g_cl.m_local->IsAlive( ) && !g_cl.m_local->m_bIsScoped( ) };
    const i32  value{ wanted ? 3 : 0 };

    if ( g_csgo.weapon_debug_spread_show->GetInt( ) != value )
    {
        g_csgo.weapon_debug_spread_show->SetValue( value );
    }
}

void misc::remove_visual_recoil( CViewSetup *setup )
{
    if ( !g_cfg.m_misc.m_removals[ config::REMOVAL_VISUAL_RECOIL ] )
    {
        return;
    }

    setup->angles -= g_cl.m_local->m_viewPunchAngle( ) +
                     g_cl.m_local->GetAimPunchAngleScaled( ) * g_csgo.view_recoil_tracking->GetFloat( );
}

void misc::fov( CViewSetup *setup )
{
    const bool scoped{ g_cl.m_local->m_bIsScoped( ) };

    if ( scoped && !g_cfg.m_misc.m_removals[ config::REMOVAL_SCOPE_ZOOM ] )
    {
        return;
    }

    if ( scoped )
    {
        g_cl.m_local->m_iFOV( ) = g_cl.m_local->m_iFOVStart( ) = g_cl.m_local->m_iDefaultFOV( );
    }

    if ( g_cfg.m_misc.m_fov_override )
    {
        setup->fov = static_cast< float >( g_cfg.m_misc.m_fov );
    } else if ( scoped )
    {
        setup->fov = static_cast< float >( g_cl.m_local->m_iDefaultFOV( ) );
    }
}

void misc::thirdperson( CViewSetup *setup, const vec3 &eye )
{
    const bind_t &key{ g_cfg.m_misc.m_thirdperson_key };
    const bool    gated{ key.m_key != 0 || key.m_mode == bind_mode_t::always };

    const bool wanted{ g_cfg.m_misc.m_thirdperson && g_cl.m_local->IsAlive( ) &&
                       ( !gated || gui::bind_active( g_cfg.m_misc.m_thirdperson_key ) ) };

    this->advance_thirdperson( wanted );

    const bool pulled_out{ this->m_third_fraction > 0.0f };
    if ( pulled_out != g_csgo.m_input->CAM_IsThirdPerson( ) )
    {
        if ( pulled_out )
        {
            g_csgo.m_input->CAM_ToThirdPerson( );
        } else
        {
            g_csgo.m_input->CAM_ToFirstPerson( );
        }
    }

    if ( !pulled_out )
    {
        return;
    }

    vec3 angles{ };
    g_csgo.m_engine->GetViewAngles( angles );

    vec3 forward{ };
    vec3 right{ };
    vec3 up{ };
    math::angle_vectors( angles, &forward, &right, &up );

    const float travel{ this->m_third_fraction };

    const vec3 wanted_origin{ eye - forward * ( static_cast< float >( g_cfg.m_misc.m_thirdperson_distance ) * travel ) +
                              right * ( static_cast< float >( g_cfg.m_misc.m_thirdperson_side ) * travel ) +
                              up * ( static_cast< float >( g_cfg.m_misc.m_thirdperson_height ) * travel ) };

    setup->angles = angles;
    setup->origin = wanted_origin;

    if ( !g_cfg.m_misc.m_thirdperson_collision )
    {
        return;
    }

    trace_t                       trace{ };
    Ray_t                         ray{ };
    CTraceFilterWorldAndPropsOnly filter{ };

    ray.Init( eye, wanted_origin, vec3{ -8.0f, -8.0f, -8.0f }, vec3{ 8.0f, 8.0f, 8.0f } );
    g_csgo.m_trace->TraceRay( ray, MASK_SOLID, &filter, &trace );

    setup->origin = trace.endpos;
}

void misc::advance_thirdperson( bool wanted )
{
    const float now{ g_csgo.m_globals->realtime };
    const float previous{ this->m_third_time };

    this->m_third_time = now;

    const float seconds{ static_cast< float >( g_cfg.m_misc.m_thirdperson_smooth_time ) / 1000.0f };

    if ( !g_cfg.m_misc.m_thirdperson_smooth || seconds <= 0.0f || previous <= 0.0f )
    {
        this->m_third_fraction = wanted ? 1.0f : 0.0f;
        return;
    }

    const float delta{ math::smaller( now - previous, 0.1f ) };
    const float step{ delta / seconds };

    this->m_third_fraction = math::clamp( this->m_third_fraction + ( wanted ? step : -step ), 0.0f, 1.0f );
}
