#pragma once

#include "link.hpp"

class misc
{
public:
    void on_create_move( );
    void on_override_view( CViewSetup *setup, const vec3 &eye );
    void on_post_data_update( );
    void on_render_start( );
    void leg_movement( );
    void paint( ) const;

private:
    void bhop( );
    void auto_strafe( );
    void slow_walk( );
    void remove_visual_recoil( CViewSetup *setup );
    void force_crosshair( );
    void fov( CViewSetup *setup );
    void thirdperson( CViewSetup *setup, const vec3 &eye );
    void advance_thirdperson( bool wanted );

    float m_strafe_side{ };
    float m_third_fraction{ };
    float m_third_time{ };
};

extern misc g_misc;
