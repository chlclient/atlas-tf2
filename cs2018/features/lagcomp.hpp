#pragma once

#include "link.hpp"

#include "anims.hpp"

constexpr float SV_MAXUNLAG{ 1.0f };

constexpr i32 TIMESTAMP_BASE{ 100 };

constexpr i32 MAX_RECORDS{ 129 };

constexpr i32 MAX_RECORD_BONES{ 128 };

struct lag_record_t
{
    matrix3x4          m_bones[ MAX_RECORD_BONES ]{ };
    pose_hitboxes_t    m_edges[ DESYNC_EDGE_COUNT ]{ };
    const studiohdr_t *m_header{ };
    i32                m_bone_count{ };
    vec3               m_origin{ };
    vec3               m_mins{ };
    vec3               m_maxs{ };
    i32                m_tick{ };
    yaw_class_t        m_yaw_class{ };
    u8                 m_misses{ };
    bool               m_standing{ };
    bool               m_yaw_bounded{ };
    bool               m_has_edges{ };
};

struct rewind_window_t
{
    i32   m_tickbase{ -1 };
    i32   m_lerp_ticks{ };
    float m_lerp{ };
    float m_correct{ };

    i32 command_tick( i32 tick ) const
    {
        return tick + this->m_lerp_ticks;
    }

    bool accepts( i32 tick ) const;
};

class lag_compensation
{
public:
    void update( );
#if defined( ATLAS_DEV )
    void paint( ) const;
#endif // ATLAS_DEV

    i32                 count( i32 index ) const;
    const lag_record_t *record( i32 index, i32 age ) const;
    bool                continuous( i32 index, const lag_record_t &entry ) const;
    bool                reachable( i32 index, const lag_record_t &entry, const rewind_window_t &window ) const;
    vec3                rewound_origin( i32 index, i32 age, const rewind_window_t &window ) const;

    static rewind_window_t rewind_window( i32 tickbase );
    static float           lerp_time( );

private:
    struct track_t
    {
        lag_record_t *m_records{ };
        C_CSPlayer   *m_player{ };
        i32           m_head{ };
        i32           m_count{ };
        i32           m_break_tick{ };
    };

    C_CSPlayer *trackable( i32 index ) const;
    void        store( i32 index, C_CSPlayer *player );
    void        invalidate( i32 index );

    track_t m_tracks[ MAX_TRACKED_PLAYERS ]{ };
#if defined( ATLAS_DEV )
    float m_reported_lerp{ };
#endif // ATLAS_DEV
};

extern lag_compensation g_lagcomp;
