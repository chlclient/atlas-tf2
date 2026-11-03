#pragma once

#include "link.hpp"

class viewmodel
{
public:
    void on_calc_view( vec3 &eye_position, vec3 &eye_angles );

private:
    vec3 offset( float delta );

    vec3  m_offset{ };
    float m_last_time{ };
};

extern viewmodel g_viewmodel;
