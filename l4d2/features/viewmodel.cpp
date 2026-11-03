#include "viewmodel.hpp"

ATLAS_NO_DESTROY viewmodel g_viewmodel{ };

static float approach( float current, float target, float delta )
{
    return current + ( target - current ) * math::clamp( delta / 0.14f, 0.0f, 1.0f );
}

void viewmodel::on_calc_view( vec3 &eye_position, vec3 &eye_angles )
{
    if ( g_cl.m_local && g_cfg.m_misc.m_removals[ config::REMOVAL_VIEWPUNCH ] )
    {
        eye_angles -= g_cl.m_local->m_vecPunchAngle( );
        eye_angles.z = 0.0f;
    }

    const float now{ g_l4d2.m_globals->realtime };
    const float delta{ this->m_last_time > 0.0f ? math::smaller( now - this->m_last_time, 0.1f ) : 0.1f };

    this->m_last_time = now;

    if ( !g_cfg.m_viewmodel.m_enabled )
    {
        return;
    }

    const vec3 wanted{ this->offset( delta ) };

    vec3 forward{ };
    vec3 right{ };
    vec3 up{ };
    math::angle_vectors( eye_angles, &forward, &right, &up );

    eye_position += forward * wanted.x + right * wanted.y + up * wanted.z;
}

vec3 viewmodel::offset( float delta )
{
    this->m_offset.x = approach( this->m_offset.x,
                                 static_cast< float >( g_cfg.m_viewmodel.m_forward ), delta );
    this->m_offset.y = approach( this->m_offset.y,
                                 static_cast< float >( g_cfg.m_viewmodel.m_right ), delta );
    this->m_offset.z = approach( this->m_offset.z,
                                 static_cast< float >( g_cfg.m_viewmodel.m_up ), delta );

    return this->m_offset;
}
