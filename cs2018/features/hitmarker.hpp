#pragma once

#include "link.hpp"

inline constexpr i32 MAX_HIT_IMPACTS{ 64 };

class hitmarker
{
public:
    void on_bullet_impact( IGameEvent *event );
    void on_player_hurt( IGameEvent *event );
    void on_net_update_end( );
    void paint( ) const;

private:
    struct hit_t
    {
        vec3  m_position{ };
        float m_time{ };
        i32   m_damage{ };
        usize m_event{ };
    };

    static constexpr usize CAPACITY{ 16 };

    bool take_impact( i32 victim, C_CSPlayer *player, vec3 &position );

    hit_t m_hits[ CAPACITY ]{ };
    usize m_next{ };
    vec3  m_impacts[ MAX_HIT_IMPACTS ]{ };
    i32   m_impact_count{ };
};

extern hitmarker g_hitmarker;
