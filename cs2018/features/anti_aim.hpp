#pragma once

#include "link.hpp"

#include "hitboxes.hpp"

inline constexpr i32 HIDE_HEAD_BINS{ 24 };
inline constexpr i32 HIDE_HEAD_THREATS{ 3 };
inline constexpr i32 HEAD_SCAN_POINTS{ 3 };

struct anti_aim_threat_t
{
    vec3  m_eye{ };
    float m_fov{ };
};

class anti_aim
{
public:
    void think( );

private:
    bool  should_run( ) const;
    usize movement_state( ) const;
    void  fetch_threats( );
    float base_yaw( usize base ) const;
    float plan_real( const config::anti_aim_profile_t &profile, float base );
    float plan_fake( const config::anti_aim_profile_t &profile, float base );
    bool  breaks_lower_body( ) const;
    float hide_head( float preferred );
    i32   head_exposure( const pose_hitboxes_t &hitboxes, const vec3 *points, float turn ) const;

    matrix3x4         m_bones[ MAXSTUDIOBONES ]{ };
    anti_aim_threat_t m_threats[ HIDE_HEAD_THREATS ]{ };
    vec3              m_view{ };
    i32               m_threat_count{ };
    i32               m_exposure{ };
    float             m_real{ };
    float             m_spin{ };
    bool              m_real_flip{ };
    bool              m_fake_flip{ };
};

extern anti_aim g_anti_aim;
