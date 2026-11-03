#include "viewmodel.hpp"

#include "math/math.hpp"

ATLAS_NO_DESTROY viewmodel g_viewmodel{ };

static float approach( float current, float target, float delta )
{
    return current + ( target - current ) * math::clamp( delta / 0.14f, 0.0f, 1.0f );
}

static void approach_angles( vec3 &current, const vec3 &target, float step )
{
    current.x += math::normalize_yaw( target.x - current.x ) * step;
    current.y += math::normalize_yaw( target.y - current.y ) * step;
    current.z += math::normalize_yaw( target.z - current.z ) * step;
}

float viewmodel::view_model_fov( float original ) const
{
    return g_cfg.m_viewmodel.m_fov_override ? static_cast< float >( g_cfg.m_viewmodel.m_fov ) : original;
}

void viewmodel::on_calc_view( C_CSPlayer *owner, vec3 &eye_position, vec3 &eye_angles )
{
    if ( g_cfg.m_misc.m_removals[ config::REMOVAL_VISUAL_RECOIL ] )
    {
        eye_angles -= owner->m_viewPunchAngle( ) +
                      owner->GetAimPunchAngleScaled( ) * g_csgo.view_recoil_tracking->GetFloat( );
    }

    const float now{ g_csgo.m_globals->realtime };
    const float delta{ this->m_last_time > 0.0f ? math::smaller( now - this->m_last_time, 0.1f ) : 0.1f };

    this->m_last_time = now;

    vec3 wanted{ this->sway( eye_angles, delta ) };

    if ( g_cfg.m_viewmodel.m_enabled )
    {
        wanted += this->offset( delta );
    }

    vec3 forward{ };
    vec3 right{ };
    vec3 up{ };
    math::angle_vectors( eye_angles, &forward, &right, &up );

    eye_position += forward * wanted.x + right * wanted.y + up * wanted.z;
}

vec3 viewmodel::sway( const vec3 &eye_angles, float delta )
{
    const float mix{ math::clamp( delta / 0.1f, 0.0f, 1.0f ) };

    approach_angles( this->m_sway_angle, eye_angles, mix );

    vec3 lagged{ };
    math::angle_vectors( eye_angles - this->m_sway_angle, &lagged );

    const vec3 lag{ vec3{ 1.0f, 0.0f, 0.0f } - lagged };

    return vec3{ lag.x, -lag.y, lag.z } *
           ( static_cast< float >( g_cfg.m_viewmodel.m_sway ) * 0.05f );
}

vec3 viewmodel::offset( float delta )
{
    this->m_offset.x = approach( this->m_offset.x, static_cast< float >( g_cfg.m_viewmodel.m_forward ), delta );
    this->m_offset.y = approach( this->m_offset.y, static_cast< float >( g_cfg.m_viewmodel.m_right ), delta );
    this->m_offset.z = approach( this->m_offset.z, static_cast< float >( g_cfg.m_viewmodel.m_up ), delta );

    return this->m_offset;
}
