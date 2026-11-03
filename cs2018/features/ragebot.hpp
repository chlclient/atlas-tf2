#pragma once

#include "link.hpp"

#include "autowall.hpp"
#include "hitboxes.hpp"
#include "lagcomp.hpp"
#include "nospread.hpp"

#include "runtime/thread_pool.hpp"

inline constexpr i32 MAX_HITBOX_POINTS{ 4 };
inline constexpr i32 MAX_SCAN_POINTS{ MAX_POSE_HITBOXES * MAX_HITBOX_POINTS };

inline constexpr i32 MAX_SCAN_RECORDS{ 3 * MAX_PLAYERS };

struct trace_pose_t
{
    matrix3x4 m_bones[ MAX_RECORD_BONES ]{ };
    vec3      m_origin{ };
    vec3      m_mins{ };
    vec3      m_maxs{ };
    i32       m_bone_count{ };
    i32       m_bone_counter{ };
    float     m_bone_time{ };
};

struct scan_point_t
{
    vec3 m_position{ };
    i32  m_bound{ };
    bool m_safe{ };
};

struct scan_record_t
{
    C_CSPlayer               *m_player{ };
    const lag_record_t       *m_entry{ };
    const mstudiohitboxset_t *m_set{ };
    vec3                      m_origin{ };
    armor_t                   m_armor{ };
    i32                       m_health{ };
    i32                       m_required{ };
    pose_hitboxes_t           m_hitboxes{ };
    pose_hitboxes_t           m_edges[ DESYNC_EDGE_COUNT ]{ };
    i32                       m_edge_count{ };
    bool                      m_safe_point{ };
    bool                      m_safe_only{ };
    scan_point_t              m_points[ MAX_SCAN_POINTS ]{ };
    i32                       m_count{ };
};

struct ragebot_target_t
{
    const scan_record_t *m_record{ };
    vec3                 m_point{ };
    i32                  m_damage{ };
    i32                  m_hitgroup{ };
    i32                  m_walls{ };
    bool                 m_lethal{ };
    bool                 m_safe{ };
};

class ragebot
{
public:
    void think( );
    void begin_command( );
    void end_command( );
    void on_simulated( C_BaseCombatWeapon *weapon, i32 activity, float curtime ) const;
    bool uses_safe_point( ) const;
    void on_pose_miss( i32 index );
    void on_round_start( );

private:
    bool should_run( C_BaseCombatWeapon *weapon );
    bool cock_revolver( C_BaseCombatWeapon *weapon ) const;
    bool find_target( const bullet_t &bullet, const rewind_window_t &window, ragebot_target_t &best );
    void collect_records( const rewind_window_t &window );
    void add_record( C_CSPlayer *player, i32 index, i32 age, const rewind_window_t &window );
    void build_record( scan_record_t &record, matrix3x4 *bones, const bullet_t &bullet ) const;
    i32  gather_points( const scan_record_t &record, const bullet_t &bullet, scan_point_t *points ) const;
    void fire_record( const scan_record_t &record, const bullet_t &bullet, ragebot_target_t &best );
    bool scope_in( C_BaseCombatWeapon *weapon ) const;
    void aim_at_target( const ragebot_target_t &target, const fired_shot_t &shot, i32 hits, const rewind_window_t &window ) const;

    void pose_record( const scan_record_t &record );
    void pin_record( C_CSPlayer *player );
    void unpin_record( C_CSPlayer *player );

    static void build_record_job( void *context, i32 index, i32 worker );

    const config::ragebot_profile_t *m_profile{ };

    scan_record_t m_records[ MAX_SCAN_RECORDS ]{ };
    i32           m_record_count{ };
    bool          m_pose_missed[ MAX_TRACKED_PLAYERS ]{ };

    matrix3x4 m_scratch[ atl::thread_pool::MAX_WORKERS + 1 ][ MAX_RECORD_BONES ]{ };

    trace_pose_t m_pose{ };
    trace_pose_t m_held{ };

    float m_forwardmove{ };
    float m_sidemove{ };
    i32   m_buttons{ };
    bool  m_stopping{ };
    bool  m_keep_stop{ };
};

extern ragebot g_ragebot;
