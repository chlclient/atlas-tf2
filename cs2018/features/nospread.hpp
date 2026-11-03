#pragma once

#include "link.hpp"

inline constexpr i32 SPREAD_SEEDS{ 256 };

inline constexpr i32 SYNC_MATCHES{ 2 };

struct spread_draw_t
{
    float m_inaccuracy_density{ };
    vec2  m_inaccuracy_direction{ };
    float m_spread_density{ };
    vec2  m_spread_direction{ };
};

struct spread_t
{
    float m_inaccuracy{ };
    float m_spread{ };
    float m_recoil_index{ };
    bool  m_negev{ };
    bool  m_patterns{ };
};

struct fired_shot_t
{
    spread_t m_spread{ };
    vec3     m_start{ };
    vec3     m_angles{ };
    i32      m_seed{ };
};

class nospread
{
public:
    void init( );
    vec2 offset( i32 seed, const spread_t &spread ) const;
    bool compensate( const vec3 &direction, const vec2 &offset, vec3 &angles ) const;

    void observe( const fired_shot_t &shot, const vec3 &impact );
    void clear( );

    bool synced( ) const
    {
        return this->m_matches >= SYNC_MATCHES;
    }

private:
    i32 nearest_seed( const vec2 &landed, const spread_t &spread, float &distance ) const;

    spread_draw_t m_draws[ 2 ][ SPREAD_SEEDS ]{ };
    i32           m_matches{ };
};

extern nospread g_nospread;
