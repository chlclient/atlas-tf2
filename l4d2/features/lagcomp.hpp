#pragma once

#include "link.hpp"

constexpr i32   MAX_RECORDS{ 64 };
constexpr float MAX_UNLAG{ 1.0f };
constexpr float UNLAG_WINDOW{ 0.2f };

struct lag_record_t
{
    vec3  m_origin{ };
    float m_yaw{ };
    float m_simtime{ };
};

struct rewind_window_t
{
    float m_curtime{ };
    float m_lerp{ };
    float m_correct{ };

    float age( i32 tick_count ) const
    {
        return this->m_curtime - ( static_cast< float >( tick_count ) * TICK_INTERVAL - this->m_lerp );
    }

    bool honoured( i32 tick_count ) const
    {
        return math::abs( this->m_correct - this->age( tick_count ) ) <= UNLAG_WINDOW;
    }

    // our latency estimate is not the server's, so a chosen tick keeps a tick away from the edge
    bool accepts( i32 tick_count ) const
    {
        const float age{ this->age( tick_count ) };
        return age >= 0.0f && age <= MAX_UNLAG && math::abs( this->m_correct - age ) <= UNLAG_WINDOW - TICK_INTERVAL;
    }

    float target( i32 tick_count ) const
    {
        return this->honoured( tick_count ) ? this->m_curtime - this->age( tick_count ) : this->m_curtime - this->m_correct;
    }
};

class lag_compensation
{
public:
    void update( );
    void clear( );

    i32                 count( i32 index ) const;
    const lag_record_t *record( i32 index, i32 age ) const;
    bool                rewind( i32 index, float time, lag_record_t &out ) const;
    bool                live( i32 index, float curtime, lag_record_t &out ) const;
    bool                compensated( C_BaseEntity *entity ) const;
    rewind_window_t     window( ) const;
    i32                 command_tick( float time ) const;

    static float lerp_time( );

private:
    struct track_t
    {
        lag_record_t *m_records{ };
        i32           m_head{ };
        i32           m_count{ };
        u32           m_handle{ };
    };

    void store( i32 index, C_BaseEntity *entity, usize kind );
    void invalidate( i32 index );

    track_t m_tracks[ 1 << MAX_EDICT_BITS ]{ };
};

extern lag_compensation g_lagcomp;
