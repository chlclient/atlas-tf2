#pragma once

#include "link.hpp"

class misc
{
public:
    void on_create_move( );
    void on_override_view( CViewSetup *setup );

private:
    void bhop( );
    void auto_strafe( );
    void fov( CViewSetup *setup );

    bool  m_bhop_jump_held{ };
    bool  m_jump_down{ };
    float m_strafe_side{ 1.0f };
};

extern misc g_misc;
