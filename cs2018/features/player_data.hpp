#pragma once

#include "link.hpp"

struct player_record_t
{
    C_CSPlayer *m_player{ };
    bool        m_dormant{ };

    vec3  m_dormant_origin{ };
    float m_dormant_expire{ };
};

class player_data
{
public:
    void         on_start_sound( const StartSoundParams_t &params );
    void         update( );
    void         on_round_start( );
    bool         dormant_origin( i32 index, vec3 &origin, float &remaining ) const;
    bool         alive( i32 index ) const;
    bool         connected( i32 index ) const;
    i32          team( i32 index ) const;
    static float window( );

private:
    player_record_t *player( i32 index );

    void remember( i32 index, const vec3 &origin );

    player_record_t m_players[ MAX_TRACKED_PLAYERS ]{ };
};

extern player_data g_player_data;
