#include "misc.hpp"

ATLAS_NO_DESTROY misc g_misc{ };

void misc::on_create_move( )
{
    this->bhop( );
    this->auto_strafe( );
}

void misc::on_override_view( CViewSetup *setup )
{
    if ( g_cl.m_local && g_cfg.m_misc.m_removals[ config::REMOVAL_VIEWPUNCH ] )
    {
        setup->angles -= g_cl.m_local->m_vecPunchAngle( );
    }

    this->fov( setup );
}

void misc::bhop( )
{
    if ( !g_cfg.m_misc.m_bhop )
    {
        return;
    }

    if ( g_cl.m_local->m_MoveType( ) == MOVETYPE_LADDER ||
         g_cl.m_local->m_MoveType( ) == MOVETYPE_NOCLIP )
    {
        return;
    }

    const bool held{ ( g_cl.m_cmd->buttons & IN_JUMP ) != 0 };
    const bool pressed{ held && !this->m_bhop_jump_held };

    this->m_bhop_jump_held = held;

    if ( !held )
    {
        return;
    }

    if ( pressed && g_cl.m_local->IsOnGround( ) )
    {
        return;
    }

    if ( !g_cl.m_local->IsOnGround( ) || g_cl.m_local->IsDucked( ) ||
         ( g_cl.m_local->m_afButtonLast( ) & IN_JUMP ) )
    {
        g_cl.m_cmd->buttons &= ~IN_JUMP;
    }
}

void misc::auto_strafe( )
{
    if ( !g_cfg.m_misc.m_auto_strafe )
    {
        return;
    }

    const bool jump_down{ ( g_cl.m_cmd->buttons & IN_JUMP ) != 0 };
    const bool jump_pressed{ jump_down && !this->m_jump_down };

    this->m_jump_down = jump_down;

    if ( g_cl.m_local->m_MoveType( ) != MOVETYPE_WALK ||
         g_cl.m_local->IsOnGround( ) ||
         g_cl.m_local->m_nWaterLevel( ) > 1 )
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

    const float wish_speed{ math::smaller( 450.f, g_cl.m_local->m_flMaxspeed( ) ) };
    const float max_air_accel{ math::smaller( 30.f, g_l4d2.sv_airaccelerate->GetFloat( ) * wish_speed * TICK_INTERVAL ) };

    if ( max_air_accel <= 0.f )
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

    if ( jump_pressed )
    {
        push_yaw = velocity.is_zero( ) ? wish_yaw : velocity_yaw;
    } else if ( math::abs( wish_delta ) > turn_limit )
    {
        push_yaw = velocity_yaw + ( wish_delta > 0.0f ? 1.0f : -1.0f ) * ( 90.0f + turn_limit );
    } else
    {
        push_yaw = wish_yaw + this->m_strafe_side * ( 90.0f - gain_offset );
    }

    const float push_delta{ DEG2RAD( math::normalize_yaw( push_yaw - view_yaw ) ) };

    g_cl.m_cmd->forwardmove = 450.f * math::cos( push_delta );
    g_cl.m_cmd->sidemove    = -450.f * math::sin( push_delta );
}

void misc::fov( CViewSetup *setup )
{
    if ( !g_cfg.m_misc.m_fov_override )
    {
        return;
    }

    if ( g_cl.m_local && g_cl.m_local->m_iFOV( ) != 0 )
    {
        return;
    }

    setup->fov = static_cast< float >( g_cfg.m_misc.m_fov );
}
