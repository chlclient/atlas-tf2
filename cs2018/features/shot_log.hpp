#pragma once

#include "link.hpp"

#include "anims.hpp"
#include "hitboxes.hpp"
#include "nospread.hpp"

inline constexpr i32 MAX_SHOT_RECORDS{ 32 };

inline constexpr i32 MAX_SHOT_HURTS{ 4 };

inline constexpr usize SHOT_LINE_LENGTH{ 256 };

struct shot_aim_t
{
    i32             m_command{ };
    i32             m_target{ };
    i32             m_hitgroup{ };
    i32             m_damage{ };
    i32             m_hitchance{ };
    i32             m_backtrack{ };
    yaw_class_t     m_yaw_class{ };
    u8              m_misses{ };
    i32             m_walls{ };
    bool            m_lethal{ };
    bool            m_safe{ };
    bool            m_standing{ };
    fired_shot_t    m_shot{ };
    pose_hitboxes_t m_pose{ };
};

struct shot_hurt_t
{
    i32 m_victim{ };
    i32 m_hitgroup{ };
    i32 m_damage{ };
    i32 m_health{ };
};

struct shot_answer_t
{
    i32         m_ack{ };
    bool        m_impacted{ };
    vec3        m_impact{ };
    i32         m_impacts{ };
    shot_hurt_t m_hurts[ MAX_SHOT_HURTS ]{ };
    i32         m_hurt_count{ };
};

enum shot_miss_t : u8
{
    SHOT_MISS_UNKNOWN,
    SHOT_MISS_SPREAD,
    SHOT_MISS_OCCLUSION,
    SHOT_MISS_OCCLUSION_OR_RESOLVER,
    SHOT_MISS_RESOLVER,
};

enum shot_state_t : u8
{
    SHOT_PENDING,
    SHOT_FIRED,
    SHOT_UNFIRED,
};

struct shot_record_t
{
    shot_aim_t          m_aim{ };
    char                m_name[ sizeof( player_info_t::name ) ]{ };
    C_BaseCombatWeapon *m_weapon{ };
    i32                 m_weapon_index{ };
    i32                 m_command{ };
    float               m_curtime{ };
    vec3                m_shoot_position{ };
    i32                 m_id{ };
    i32                 m_choked{ };
    shot_state_t        m_state{ };
    bool                m_dead{ };
    shot_answer_t       m_answer{ };
};

enum shot_line_kind_t : u8
{
    SHOT_LINE_FIRE,
    SHOT_LINE_HIT,
    SHOT_LINE_MISS,
    SHOT_LINE_COUNT,
};

class shot_log final : public IGameEventListener2
{
public:
    bool install( );
    void stage( const shot_aim_t &aim );
    void on_command_simulated( i32 command, float curtime, C_CSPlayer *shooter, C_BaseCombatWeapon *weapon, float last_shot_before,
                               i32 clip_before );
    void on_post_data_update( );
    void on_net_update_end( );

    void FireGameEvent( IGameEvent *event ) override;
    int  GetEventDebugID( ) override;

private:
    shot_record_t &at( i32 offset );
    shot_record_t &push( );
    shot_answer_t &answer_at( i32 offset );
    void           pair( );
    void           clear( );

    void on_weapon_fire( IGameEvent *event );
    void on_bullet_impact( IGameEvent *event );
    void on_player_hurt( IGameEvent *event );

    void print( shot_line_kind_t kind, const char ( &text )[ SHOT_LINE_LENGTH ] ) const;
    void print_fire( const shot_record_t &shot );
    void print_result( const shot_record_t &shot );

    static shot_aim_t    s_staged;
    static shot_record_t s_shots[ MAX_SHOT_RECORDS ];
    static shot_answer_t s_answers[ MAX_SHOT_RECORDS ];

    shot_answer_t *m_open{ };
    i32            m_head{ };
    i32            m_count{ };
    i32            m_answer_head{ };
    i32            m_answer_count{ };
    i32            m_predicted{ };
    i32            m_last_id{ };
};

extern shot_log g_shot_log;
