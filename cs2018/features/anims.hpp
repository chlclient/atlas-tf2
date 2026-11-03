#pragma once

#include "link.hpp"

#include "hitboxes.hpp"

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L2211
inline constexpr float CSGO_ANIM_LOWER_REALIGN_DELAY{ 1.1f };

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L2212
inline constexpr float CSGO_ANIM_READJUST_THRESHOLD{ 120.0f };

inline constexpr float CSGO_ANIM_LOWER_CATCHUP_IDLE{ 100.0f };

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L60
inline constexpr float CSGO_ANIM_AIMMATRIX_DEFAULT_YAW_MAX{ 58.0f };

inline constexpr i32 DESYNC_EDGE_COUNT{ 2 };

// what DT_Animationlayer networks of a layer
struct anim_layer_t
{
    i32   m_sequence{ };
    i32   m_order{ };
    float m_prev_cycle{ };
    float m_weight{ };
    float m_weight_delta_rate{ };
    float m_playback_rate{ };
    float m_cycle{ };
};

enum yaw_class_t : u8
{
    YAW_UNKNOWN,
    YAW_NETWORKED,
    YAW_LOWER_BODY,
    YAW_MOVING,
};

struct remote_anim_record_t
{
    anim_layer_t m_layers[ ANIMATION_LAYER_COUNT ]{ };
    vec3         m_origin{ };
    vec3         m_angles{ };
    C_CSPlayer  *m_player{ };
    float        m_lower_body_yaw{ };
    i32          m_tick{ };
    i32          m_flags{ };
    i32          m_model{ };
    i32          m_snapshot{ };
    yaw_class_t  m_yaw_class{ };
    u8           m_misses{ };
    bool         m_standing{ };
    bool         m_yaw_bounded{ };
};

struct desync_edges_t
{
    pose_hitboxes_t m_poses[ DESYNC_EDGE_COUNT ]{ };
    i32             m_tick{ };
};

// the server animstate's m_ActivityModifiers as UpdateActivityModifiers last built it
struct activity_modifiers_t
{
    const char *m_weapon_prefix{ };
    bool        m_moving{ };
    bool        m_crouch{ };
};

// a command the local predicted for the first time: what the server's update of it reads, and the emulated state after it
struct local_record_t
{
    activity_modifiers_t m_modifiers{ };
    i32                  m_sequences[ ANIMATION_LAYER_COUNT ]{ };
    float                m_cycles[ ANIMATION_LAYER_COUNT ]{ };
    i32                  m_command{ };
    float                m_lower_body_yaw{ };
    float                m_increment{ };
    float                m_duck_amount{ };
    float                m_run_portion{ };
    bool                 m_batch_start{ };
    bool                 m_deploy{ };
    bool                 m_weapon_changed{ };
    bool                 m_deploy_rate_limited{ };
};

struct local_anim_state_t
{
    anim_layer_t         m_layers[ ANIMATION_LAYER_COUNT ]{ };
    C_AnimationLayer     m_server_layers[ ANIMATION_LAYER_COUNT ]{ };
    activity_modifiers_t m_modifiers{ };
    C_CSPlayer          *m_player{ };
    C_BaseCombatWeapon  *m_weapon{ };
    float                m_primary_cycle{ };
    float                m_move_weight{ };
    float                m_lower_body_yaw{ };
    float                m_lower_body_realign_time{ };
    i32                  m_corrected_through{ };
    i32                  m_adopted_through[ ANIMATION_LAYER_COUNT ]{ };
    bool                 m_deploy_rate_limited{ };
    bool                 m_jumping{ };
    bool                 m_adjust_started{ };
};

// the animstate as the server's SetUpVelocity reads it, from the update before: SetUpMovement only moves it on after
struct movement_before_t
{
    C_AnimationLayer m_adjust{ };
    float            m_duration_still{ };
    float            m_stutter_step{ };
    bool             m_on_ground{ };
    bool             m_on_ladder{ };
    bool             m_landing{ };
    bool             m_land_started{ };
};

class animations
{
public:
    void                        update_others( );
    void                        on_create_move( i32 command );
    void                        update_local( C_CSPlayer *local );
    void                        restore_local( );
    void                        set_up_movement( CCSGOPlayerAnimState *anim_state );
    void                        on_jump( );
    bool                        replay_velocity( const C_BaseEntity *entity, vec3 &velocity ) const;
    const remote_anim_record_t *record( i32 index, i32 tick ) const;
    const desync_edges_t       *edges( i32 index, i32 tick ) const;
    const local_anim_state_t   &local_state( ) const;

private:
    void              replay( C_CSPlayer *player, const remote_anim_record_t &previous, remote_anim_record_t &current );
    void              build_edges( C_CSPlayer *player, const remote_anim_record_t &start, const remote_anim_record_t &current, const vec3 &angles,
                                   i32 flags, float lower_body );
    void              step( C_CSPlayer *player, const vec3 &angles ) const;
    void              step_local( C_CSPlayer *local, const CUserCmd *cmd, local_record_t &record );
    void              animate_local( C_CSPlayer *local, const CUserCmd *cmd );
    void              update_lower_body( CCSGOPlayerAnimState *anim_state, bool was_on_ground );
    void              swap_server_layers( C_CSPlayer *local );
    void              deploy( C_CSPlayer *local, const activity_modifiers_t &modifiers );
    void              weapon_action( C_CSPlayer *local, float increment, float duck_amount );
    void              alive_loop( C_CSPlayer *local, float increment, float run_portion, bool weapon_changed );
    void              adopt_movement_layers( C_CSPlayer *local, const local_record_t &anchor, i32 predicted );
    movement_before_t pick_movement_layers( C_CSPlayer *local ) const;
    void              settle_movement_layers( C_CSPlayer *local, const movement_before_t &before );
    void              idle_adjust( C_CSPlayer *local, const movement_before_t &before );

    remote_anim_record_t  m_records[ MAX_TRACKED_PLAYERS ]{ };
    desync_edges_t        m_edges[ MAX_TRACKED_PLAYERS ]{ };
    u8                    m_state_before[ ANIMSTATE_SIZE ]{ };
    u8                    m_state_after[ ANIMSTATE_SIZE ]{ };
    float                 m_poses_after[ MAXSTUDIOPOSEPARAM ]{ };
    local_anim_state_t    m_local{ };
    i32                   m_batch_starts[ MULTIPLAYER_BACKUP ]{ };
    local_record_t        m_local_records[ MULTIPLAYER_BACKUP ]{ };
    C_CSPlayer           *m_replay_player{ };
    vec3                  m_replay_velocity{ };
    CCSGOPlayerAnimState *m_local_step{ };
    i32                   m_local_step_buttons{ };
    i32                   m_snapshot{ };
    i32                   m_snapshot_break{ };
    bool                  m_jump_pending{ };
};

extern animations g_anims;
