#pragma once

#include "link.hpp"

#include "anims.hpp"

enum lower_body_window_t : u8
{
    LOWER_BODY_WINDOW_UNKNOWN,
    LOWER_BODY_WINDOW_MOVING,
    LOWER_BODY_WINDOW_REALIGN,
};

enum yaw_model_t : u8
{
    YAW_MODEL_MEAN,
    YAW_MODEL_RATE,
    YAW_MODEL_ALTERNATE,
    YAW_MODEL_COUNT,
};

enum yaw_frame_t : u8
{
    YAW_FRAME_NETWORKED,
    YAW_FRAME_WORLD,
    YAW_FRAME_TARGET,
    YAW_FRAME_COUNT,
};

struct yaw_samples_t
{
    f64   m_cos{ };
    f64   m_sin{ };
    f64   m_rate_sum{ };
    f64   m_unwrapped{ };
    f64   m_unwrapped_sum{ };
    f64   m_tick_sum{ };
    f64   m_errors[ YAW_MODEL_COUNT ]{ };
    float m_first{ };
    float m_spread{ };
    float m_last{ };
    float m_center{ };
    i32   m_last_tick{ };
    i32   m_batch_ticks{ };
    i32   m_samples{ };
    i32   m_rates{ };
    i32   m_scored{ };
    bool  m_any{ };
    bool  m_anchored{ };

    void  add( float yaw, i32 tick, bool consecutive );
    void  anchor( float yaw, i32 tick );
    float mean( ) const;
    float predict( yaw_model_t model, i32 tick ) const;
    f64   length_sqr( ) const;
};

struct lower_body_flick_t
{
    float m_yaw{ };
    float m_time{ };
    float m_increments[ 2 ]{ };
    float m_least{ };
    i8    m_side{ };
    bool  m_pending{ };
};

struct resolver_player_t
{
    C_CSPlayer         *m_player{ };
    yaw_samples_t       m_frames[ YAW_FRAME_COUNT ]{ };
    yaw_samples_t       m_phase_frames[ YAW_FRAME_COUNT ]{ };
    lower_body_flick_t  m_flicks[ 2 ]{ };
    float               m_realign_time{ };
    float               m_update_time{ };
    float               m_update_increment{ };
    float               m_exact_yaw{ };
    i32                 m_exact_tick{ };
    lower_body_window_t m_window{ };
    u8                  m_misses{ };
    u8                  m_standing_misses{ };
    bool                m_moving{ };
};

class resolver
{
public:
    float on_batch( C_CSPlayer                 *player,
                    const remote_anim_record_t &previous,
                    remote_anim_record_t       &current,
                    float                       networked,
                    bool                        continuous,
                    bool                        animated_ground,
                    bool                        snapshots_seen,
                    bool                        balance_adjust );
    void  on_miss( i32 index, u8 misses, bool standing );

private:
    resolver_player_t m_players[ MAX_TRACKED_PLAYERS ]{ };
};

extern resolver g_resolver;
