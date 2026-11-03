#pragma once

#include "link.hpp"

class viewmodel
{
public:
    void  on_calc_view( C_CSPlayer *owner, vec3 &eye_position, vec3 &eye_angles );
    float view_model_fov( float original ) const;

private:
    vec3 offset( float delta );
    vec3 sway( const vec3 &eye_angles, float delta );

    vec3  m_offset{ };
    vec3  m_sway_angle{ };
    float m_last_time{ };
};

extern viewmodel g_viewmodel;
