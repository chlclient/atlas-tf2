#pragma once

#include "link.hpp"

class no_spread
{
public:
    void pre_predict( );
    void post_predict( );
    void clear( );

private:
    float target_spread( CTerrorWeaponInfo *info, C_TerrorGun *gun, i32 fov_ratio ) const;

    bool         m_armed{ };
    C_TerrorGun *m_gun{ };
    float        m_spread{ };
    i32          m_shots{ };
    i32          m_clip{ };
    bool  m_shadow_valid{ };
    float m_shadow{ };
    i32   m_shadow_weapon{ -1 };
};

extern no_spread g_nospread;
