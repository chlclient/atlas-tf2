#pragma once

#include "lagcomp.hpp"
#include "link.hpp"

constexpr i32 MAX_HITSCAN_TARGETS{ 128 };
constexpr i32 MAX_DRAW_BOXES{ 32 };

struct hitscan_target_t
{
    C_BaseEntity *m_entity{ };
    f32           m_fov{ };
};

struct hitscan_placement_t
{
    vec3 m_from{ };
    vec3 m_to{ };
    f32  m_sin{ };
    f32  m_cos{ 1.0f };

    vec3 apply( const vec3 &point ) const
    {
        const vec3 offset{ point - this->m_from };
        return vec3{ offset.x * this->m_cos - offset.y * this->m_sin, offset.x * this->m_sin + offset.y * this->m_cos, offset.z } + this->m_to;
    }
};

struct hitscan_aim_t
{
    vec3                m_point{ };
    hitscan_placement_t m_placement{ };
    i32                 m_hitbox{ };
    i32                 m_tick_count{ };
};

class hitscan
{
public:
    bool active( ) const;
    bool think( C_TerrorGun *gun );
    void commit( bool fired );
    void paint( ) const;

private:
    struct cmd_state_t
    {
        vec3  m_viewangles{ };
        float m_forwardmove{ };
        float m_sidemove{ };
        i32   m_buttons{ };
        i32   m_tick_count{ };
    };

    struct pose_t
    {
        matrix3x4           m_bones[ MAXSTUDIOBONES ];
        mstudiohitboxset_t *m_hitboxes{ };
        vec3                m_origin{ };
    };

    struct shot_draw_t
    {
        i32   m_boxes{ };
        i32   m_aimed{ -1 };
        float m_time{ };
        vec3  m_shoot_pos{ };
        vec3  m_aim_point{ };
        vec3  m_corners[ MAX_DRAW_BOXES ][ 8 ]{ };
    };

    bool should_run( C_TerrorGun *gun ) const;
    bool can_fire( C_TerrorGun *gun ) const;
    bool should_release( C_TerrorGun *gun ) const;

    void fetch_targets( CTerrorWeaponInfo *info );
    bool get_best_target( hitscan_aim_t &aim );
    bool build_pose( C_BaseEntity *entity, pose_t &pose ) const;
    bool rewind_target( C_BaseEntity *entity, lag_record_t &server ) const;
    bool get_target_aim( C_BaseEntity *entity, const pose_t &pose, hitscan_aim_t &aim );
    bool get_backtrack_aim( C_BaseEntity *entity, const pose_t &pose, const lag_record_t &server, hitscan_aim_t &aim );
    bool get_aimpoint( C_BaseEntity *entity, const pose_t &pose, const hitscan_placement_t &placement, hitscan_aim_t &aim );
    bool is_visible( C_BaseEntity *entity, const vec3 &point );
    void aim_at_target( const hitscan_aim_t &aim );

    void  remember_shot( const pose_t &pose, const hitscan_aim_t &aim );
    float shot_fade( ) const;
    void  paint_fov_circle( ) const;
    void  paint_shot( ) const;

    hitscan_target_t m_targets[ MAX_HITSCAN_TARGETS ]{ };
    i32              m_target_count{ };
    vec3             m_eye{ };
    rewind_window_t  m_window{ };
    i32              m_budget{ };
    cmd_state_t      m_saved{ };
    shot_draw_t      m_pending{ };
    shot_draw_t      m_shot{ };
};

extern hitscan g_hitscan;
