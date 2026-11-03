#include "resolver.hpp"

#include "math/math.hpp"

ATLAS_NO_DESTROY resolver g_resolver{ };

constexpr float LOWER_BODY_REALIGN_ANGLE{ 35.0f };

constexpr float ANGLEMOD_STEP{ 360.0f / 65536.0f };

constexpr f64 DEGREES_TO_RADIANS{ 3.14159265358979323846 / 180.0 };

ATLAS_FORCE_INLINE float anglemod( float angle )
{
    return ( 360.0f / 65536.0f ) * static_cast< float >( static_cast< i32 >( angle * ( 65536.0f / 360.0f ) ) & 65535 );
}

void yaw_samples_t::add( float yaw, i32 tick, bool consecutive )
{
    if ( consecutive && this->m_rates )
    {
        for ( i32 model{ }; model < YAW_MODEL_COUNT; ++model )
        {
            const float miss{ math::abs( math::normalize_yaw( this->predict( static_cast< yaw_model_t >( model ), tick ) - yaw ) ) };
            this->m_errors[ model ] += math::bigger( miss - ANGLEMOD_STEP, 0.0f );
        }
        ++this->m_scored;
    }

    if ( consecutive )
    {
        const float turn{ math::normalize_yaw( yaw - this->m_last ) };
        this->m_rate_sum += turn / static_cast< float >( tick - this->m_last_tick );
        this->m_center      = math::normalize_yaw( this->m_last + turn * 0.5f );
        this->m_batch_ticks = tick - this->m_last_tick;
        ++this->m_rates;
    }

    f64 sine{ };
    f64 cosine{ };
    math::sincos( static_cast< f64 >( yaw ) * DEGREES_TO_RADIANS, sine, cosine );
    this->m_cos += cosine;
    this->m_sin += sine;

    this->m_unwrapped = this->m_any ? this->m_unwrapped + math::normalize_yaw( yaw - this->m_last ) : yaw;
    this->m_unwrapped_sum += this->m_unwrapped;
    this->m_tick_sum += tick;
    ++this->m_samples;

    if ( !this->m_any )
    {
        this->m_first = yaw;
        this->m_any   = true;
    }
    this->m_spread    = math::bigger( this->m_spread, math::abs( math::normalize_yaw( yaw - this->m_first ) ) );
    this->m_last      = yaw;
    this->m_last_tick = tick;
    this->m_anchored  = false;
}

void yaw_samples_t::anchor( float yaw, i32 tick )
{
    this->m_last      = yaw;
    this->m_last_tick = tick;
    this->m_anchored  = true;
}

float yaw_samples_t::mean( ) const
{
    return static_cast< float >( math::atan2( this->m_sin, this->m_cos ) / DEGREES_TO_RADIANS );
}

float yaw_samples_t::predict( yaw_model_t model, i32 tick ) const
{
    switch ( model )
    {
        case YAW_MODEL_RATE:
        {
            if ( !this->m_rates )
            {
                return this->m_last;
            }

            const f64 rate{ this->m_rate_sum / this->m_rates };
            if ( this->m_anchored )
            {
                return math::normalize_yaw( this->m_last + static_cast< float >( rate * ( tick - this->m_last_tick ) ) );
            }

            const f64 line{ this->m_unwrapped_sum / this->m_samples + rate * ( tick - this->m_tick_sum / this->m_samples ) };
            return math::normalize_yaw( static_cast< float >( line - 360.0 * static_cast< i32 >( line / 360.0 ) ) );
        }
        case YAW_MODEL_ALTERNATE:
            return this->m_batch_ticks && ( ( tick - this->m_last_tick ) / this->m_batch_ticks ) & 1
                       ? math::normalize_yaw( this->m_center + math::normalize_yaw( this->m_center - this->m_last ) )
                       : this->m_last;
        default: break;
    }

    return this->mean( );
}

f64 yaw_samples_t::length_sqr( ) const
{
    return this->m_cos * this->m_cos + this->m_sin * this->m_sin;
}

#if defined( ATLAS_DEV )
ATLAS_FORCE_INLINE i32 adjust_start( C_CSPlayer *player, const anim_layer_t &before, const anim_layer_t &adjust )
{
    if ( adjust.m_sequence == before.m_sequence && adjust.m_cycle >= before.m_cycle )
    {
        return 0;
    }

    const i32 activity{ player->m_pStudioHdr( )->GetSequenceActivity( adjust.m_sequence ) };
    return activity == ACT_CSGO_IDLE_TURN_BALANCEADJUST || activity == ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING ? activity : 0;
}
#endif // ATLAS_DEV

float resolver::on_batch( C_CSPlayer                 *player,
                          const remote_anim_record_t &previous,
                          remote_anim_record_t       &current,
                          float                       networked,
                          bool                        continuous,
                          bool                        animated_ground,
                          bool                        snapshots_seen,
                          bool                        balance_adjust )
{
    resolver_player_t &state{ this->m_players[ player->entindex( ) ] };
    const bool         complete{ continuous && ( current.m_tick - previous.m_tick == 1 || snapshots_seen ) };
    if ( state.m_player != player )
    {
        state          = resolver_player_t{ };
        state.m_player = player;
    } else if ( !complete )
    {
        state.m_window = LOWER_BODY_WINDOW_UNKNOWN;
    }

    if ( current.m_yaw_class == YAW_NETWORKED || current.m_yaw_class == YAW_LOWER_BODY )
    {
        state.m_exact_yaw  = current.m_yaw_class == YAW_NETWORKED ? current.m_angles.y : current.m_lower_body_yaw;
        state.m_exact_tick = current.m_tick;
    }

    const float time{ TICKS_TO_TIME( previous.m_tick + 1 ) };
    const float move_rate{ current.m_layers[ ANIMATION_LAYER_MOVEMENT_MOVE ].m_playback_rate };
    const bool  written{ current.m_lower_body_yaw != previous.m_lower_body_yaw };
    const bool  snapshot_ground{ ( previous.m_flags & FL_ONGROUND ) != 0 };
    const bool  grounded{ animated_ground && snapshot_ground };
    const bool  ground_unknown{ animated_ground != snapshot_ground };
    const bool  expired{ complete && move_rate == 0.0f && !written && state.m_window != LOWER_BODY_WINDOW_UNKNOWN && time > state.m_realign_time };
    const bool  check{ expired && grounded };
    const bool  bounded{ check && state.m_window == LOWER_BODY_WINDOW_REALIGN && current.m_yaw_class == YAW_UNKNOWN };

    vec3 toward{ };
    if ( g_cl.m_local )
    {
        math::vector_angles( g_cl.m_local->GetAbsOrigin( ) - current.m_origin, toward );
    }
    const float bases[ YAW_FRAME_COUNT ]{ networked, 0.0f, toward.y };

    const yaw_samples_t &networked_frame{ state.m_frames[ YAW_FRAME_NETWORKED ] };
    i32                  frame{ YAW_FRAME_NETWORKED };
    yaw_model_t          model{ YAW_MODEL_MEAN };
    if ( networked_frame.m_scored )
    {
        for ( i32 kind{ }; kind < YAW_MODEL_COUNT; ++kind )
        {
            for ( i32 candidate{ }; candidate < YAW_FRAME_COUNT; ++candidate )
            {
                const f64 floor{ candidate != YAW_FRAME_NETWORKED && frame == YAW_FRAME_NETWORKED ? networked_frame.m_scored * ANGLEMOD_STEP : 0.0 };
                if ( state.m_frames[ candidate ].m_errors[ kind ] + floor < state.m_frames[ frame ].m_errors[ model ] )
                {
                    frame = candidate;
                    model = static_cast< yaw_model_t >( kind );
                }
            }
        }
    } else if ( networked_frame.m_spread >= ANGLEMOD_STEP )
    {
        for ( i32 candidate{ YAW_FRAME_WORLD }; candidate < YAW_FRAME_COUNT; ++candidate )
        {
            if ( state.m_frames[ candidate ].length_sqr( ) > state.m_frames[ frame ].length_sqr( ) )
            {
                frame = candidate;
            }
        }
    }

    const float predicted{ state.m_frames[ frame ].predict( model, current.m_tick ) };
    float       yaw{ math::normalize_yaw( bases[ frame ] + predicted ) };

    const bool standing{ grounded && move_rate == 0.0f };

    const bool          flick{ standing && current.m_yaw_class == YAW_NETWORKED && current.m_angles.y == anglemod( current.m_lower_body_yaw ) &&
                               ( written || expired ) };
    const float         increment{ time - state.m_update_time };
    lower_body_flick_t &last{ state.m_flicks[ 0 ] };
    if ( !standing || !complete )
    {
        state.m_flicks[ 0 ] = state.m_flicks[ 1 ] = lower_body_flick_t{ };
    } else
    {
        if ( last.m_pending && balance_adjust && !written && !flick )
        {
            const float back{ time - last.m_time };
            const float approached{ CSGO_ANIM_LOWER_CATCHUP_IDLE * ( last.m_increments[ 0 ] + last.m_increments[ 1 ] ) };
            last.m_side  = -1;
            last.m_least = approached > ( CSGO_ANIM_READJUST_THRESHOLD + CSGO_ANIM_LOWER_CATCHUP_IDLE ) * back
                               ? CSGO_ANIM_AIMMATRIX_DEFAULT_YAW_MAX + LOWER_BODY_REALIGN_ANGLE + approached
                               : 2.0f * CSGO_ANIM_AIMMATRIX_DEFAULT_YAW_MAX +
                                     math::bigger( CSGO_ANIM_LOWER_CATCHUP_IDLE * last.m_increments[ 0 ],
                                                   ( CSGO_ANIM_READJUST_THRESHOLD + CSGO_ANIM_LOWER_CATCHUP_IDLE ) * back -
                                                       CSGO_ANIM_LOWER_CATCHUP_IDLE * last.m_increments[ 1 ] );
        }
        last.m_pending = false;

        const float exact{ current.m_yaw_class == YAW_NETWORKED ? current.m_angles.y : current.m_lower_body_yaw };
        const float apart{ static_cast< float >( last.m_side ) * math::normalize_yaw( exact - last.m_yaw ) };
        if ( flick )
        {
            state.m_flicks[ 1 ] = last;
            last                = lower_body_flick_t{ current.m_lower_body_yaw, time, { state.m_update_increment, increment } };
            if ( balance_adjust )
            {
                last.m_side  = 1;
                last.m_least = 2.0f * CSGO_ANIM_AIMMATRIX_DEFAULT_YAW_MAX + CSGO_ANIM_LOWER_CATCHUP_IDLE * last.m_increments[ 0 ] +
                               ( CSGO_ANIM_READJUST_THRESHOLD - CSGO_ANIM_LOWER_CATCHUP_IDLE ) * last.m_increments[ 1 ];
            } else
            {
                last.m_pending = true;
            }
        } else if ( expired || ( ( current.m_yaw_class == YAW_NETWORKED || current.m_yaw_class == YAW_LOWER_BODY ) && apart < last.m_least ) )
        {
            last.m_side = 0;
        }
    }
    state.m_update_increment = increment;
    state.m_update_time      = time;

    const lower_body_flick_t &before{ state.m_flicks[ 1 ] };
    const bool                arc{ last.m_side && last.m_side == before.m_side && last.m_least < 180.0f &&
                                   math::abs( math::normalize_yaw( last.m_yaw - before.m_yaw ) ) < CSGO_ANIM_AIMMATRIX_DEFAULT_YAW_MAX };

    const u8 misses{ standing ? state.m_standing_misses : state.m_misses };
    if ( arc && current.m_yaw_class == YAW_UNKNOWN )
    {
        yaw = math::normalize_yaw( last.m_yaw + static_cast< float >( last.m_side ) * ( last.m_least + 180.0f ) * 0.5f );
    } else if ( misses & 1 || ( bounded && math::abs( math::normalize_yaw( yaw - anglemod( current.m_lower_body_yaw ) ) ) > LOWER_BODY_REALIGN_ANGLE ) )
    {
        yaw = current.m_lower_body_yaw;
    }
    current.m_misses      = misses;
    current.m_standing    = standing;
    current.m_yaw_bounded = bounded;

#if defined( ATLAS_DEV )
    if ( g_cfg.m_debug.m_lag_records )
    {
        char errors[ 160 ]{ };
        char candidates[ 160 ]{ };
        for ( i32 at{ }; at < static_cast< i32 >( YAW_FRAME_COUNT ) * YAW_MODEL_COUNT; ++at )
        {
            const yaw_samples_t &samples{ state.m_frames[ at / YAW_MODEL_COUNT ] };
            const yaw_model_t    kind{ static_cast< yaw_model_t >( at % YAW_MODEL_COUNT ) };
            const usize          used{ str::length( errors ) };
            const usize          placed{ str::length( candidates ) };
            atl::format( errors + used, sizeof( errors ) - used, at ? ",%.2f" : "%.2f", samples.m_errors[ kind ] );
            atl::format( candidates + placed, sizeof( candidates ) - placed, at ? ",%.4f" : "%.4f",
                         math::normalize_yaw( bases[ at / YAW_MODEL_COUNT ] + samples.predict( kind, current.m_tick ) ) );
        }

        DBGOUT( "RSLV-SET idx=%d t=%d class=%d time=%.6f window=%d timer=%.6f check=%d misses=%d standing=%d set=%s center=%.4f extent=%.1f "
                "flick=%d side=%d least=%.2f "
                "adjust=%d air=%d land=%d exact=%.4f@%d frame=%d model=%d scored=%d predicted=%.4f error=%.2f toward=%.4f errors=%s candidates=%s",
                player->entindex( ), current.m_tick, static_cast< i32 >( current.m_yaw_class ), time, static_cast< i32 >( state.m_window ),
                state.m_realign_time, check, misses, standing, bounded ? "arc" : "full", anglemod( current.m_lower_body_yaw ),
                bounded ? LOWER_BODY_REALIGN_ANGLE : 180.0f, flick, arc ? static_cast< i32 >( last.m_side ) : 0, last.m_least,
                adjust_start( player, previous.m_layers[ ANIMATION_LAYER_ADJUST ], current.m_layers[ ANIMATION_LAYER_ADJUST ] ), !grounded,
                ground_unknown, state.m_exact_yaw, state.m_exact_tick, frame, static_cast< i32 >( model ), networked_frame.m_scored, predicted,
                state.m_frames[ frame ].m_errors[ model ], toward.y, errors, candidates );
    }
#endif // ATLAS_DEV

    const bool started{ snapshot_ground && previous.m_layers[ ANIMATION_LAYER_MOVEMENT_MOVE ].m_playback_rate == 0.0f };
    const bool batched{ current.m_tick - previous.m_tick > 1 };
    const bool sampled{ complete && move_rate > 0.0f && batched && !started && ( grounded || ( ground_unknown && written ) ) };

    if ( sampled )
    {
        for ( i32 at{ }; at < YAW_FRAME_COUNT; ++at )
        {
            if ( !state.m_moving )
            {
                state.m_phase_frames[ at ] = yaw_samples_t{ };
            }

            state.m_phase_frames[ at ].add( math::normalize_yaw( current.m_lower_body_yaw - bases[ at ] ), current.m_tick, state.m_moving );
        }

        const bool replaced{ state.m_phase_frames[ YAW_FRAME_NETWORKED ].m_scored || !networked_frame.m_any };
        for ( i32 at{ }; at < YAW_FRAME_COUNT; ++at )
        {
            if ( replaced )
            {
                state.m_frames[ at ] = state.m_phase_frames[ at ];
            } else
            {
                state.m_frames[ at ].anchor( math::normalize_yaw( current.m_lower_body_yaw - bases[ at ] ), current.m_tick );
            }
        }

        if ( replaced )
        {
            state.m_misses = 0;
        }
    }

    if ( complete && grounded && written && move_rate == 0.0f && batched && networked_frame.m_any )
    {
        for ( i32 at{ }; at < YAW_FRAME_COUNT; ++at )
        {
            state.m_frames[ at ].anchor( math::normalize_yaw( current.m_lower_body_yaw - bases[ at ] ), current.m_tick );
        }
    }
    state.m_moving = sampled;

    if ( !complete || !( grounded || ground_unknown ) )
    {
        return yaw;
    }

    if ( move_rate > 0.0f )
    {
        state.m_realign_time = time + CSGO_ANIM_LOWER_REALIGN_DELAY * 0.2f;
        state.m_window       = LOWER_BODY_WINDOW_MOVING;
    } else if ( written || ( expired && ( current.m_yaw_class != YAW_NETWORKED || current.m_angles.y == anglemod( current.m_lower_body_yaw ) ) ) )
    {
        state.m_realign_time = time + CSGO_ANIM_LOWER_REALIGN_DELAY;
        state.m_window       = LOWER_BODY_WINDOW_REALIGN;
    }

    return yaw;
}

void resolver::on_miss( i32 index, u8 misses, bool standing )
{
    resolver_player_t &state{ this->m_players[ index ] };
    u8                &counter{ standing ? state.m_standing_misses : state.m_misses };
    if ( counter == misses )
    {
        ++counter;
    }
}
