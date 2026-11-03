#pragma once

#include "link.hpp"

inline constexpr float PLAYER_RAY_EXTENSION{ 40.0f };

struct bullet_t
{
    vec3  m_start{ };
    vec3  m_direction{ };
    float m_damage{ };
    float m_range{ };
    float m_range_modifier{ };
    float m_penetration{ };
    float m_armor_ratio{ };
    bool  m_shock{ };
};

struct wall_t
{
    vec3  m_enter{ };
    vec3  m_exit{ };
    float m_impact_damage{ };
};

struct bullet_hit_t
{
    wall_t      m_walls[ MAX_PENETRATIONS ]{ };
    i32         m_wall_count{ };
    vec3        m_end{ };
    float       m_impact_damage{ };
    C_CSPlayer *m_player{ };
    i32         m_hitgroup{ };
    i32         m_hitbox{ };
    i32         m_damage{ };
};

struct armor_t
{
    i32   m_value{ };
    bool  m_helmet{ };
    bool  m_heavy{ };
    bool  m_immune{ };
    float m_head_scale{ };
    float m_body_scale{ };
};

class autowall
{
public:
    bool    setup( C_CSPlayer *shooter, bullet_t &bullet ) const;
    bool    fire( C_CSPlayer *shooter, const bullet_t &bullet, const C_CSPlayer *target, bullet_hit_t &hit ) const;
    armor_t armor_of( C_CSPlayer *player ) const;
    i32     scale_damage( const armor_t &armor, float damage, float armor_ratio, i32 hitgroup, bool shock ) const;

#if defined( ATLAS_DEV )
    void think( );
    void paint( ) const;

private:
    bullet_hit_t m_debug_hit{ };
    bool         m_debug_fired{ };
#endif // ATLAS_DEV
};

extern autowall g_autowall;
