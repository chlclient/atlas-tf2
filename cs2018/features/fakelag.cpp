#include "fakelag.hpp"

#include "math/math.hpp"

ATLAS_NO_DESTROY fakelag g_fakelag{ };

ATLAS_FORCE_INLINE i32 roll( i32 minimum, i32 maximum )
{
    CUniformRandomStream random{ };
    random.Construct( );
    random.SetSeed( g_cl.m_cmd->random_seed );
    return random.RandomInt( minimum, maximum );
}

void fakelag::think( )
{
    if ( !this->should_run( ) )
    {
        return;
    }

    const config::anti_hack_t &config{ g_cfg.m_anti_hack };
    const i32                  held{ g_csgo.m_client_states[ 0 ]->chokedcommands };

    i32 goal{ };
    switch ( config.m_fakelag_mode )
    {
        case config::FAKELAG_MODE_STATIC: goal = config.m_static_amount; break;

        case config::FAKELAG_MODE_ADAPTIVE:
        {
            if ( held == 0 )
            {
                this->m_roll = config.m_adaptive_randomized_choke ? roll( 0, 4 ) : 0;
            }

            const float step{ g_cl.m_local->m_vecVelocity( ).length( ) * TICK_INTERVAL };
            goal = static_cast< i32 >( math::sqrt( LAG_COMPENSATION_TELEPORTED_DISTANCE_SQR ) / step ) + 1 + this->m_roll;
            break;
        }

        case config::FAKELAG_MODE_RANDOM:
        {
            if ( held == 0 )
            {
                this->m_roll = roll( config.m_random_min, config.m_random_max );
            }

            goal = this->m_roll;
            break;
        }

        default: break;
    }

    if ( held + 1 < math::smaller( goal, MAX_NEW_COMMANDS ) )
    {
        g_cl.m_send_packet = false;
    }
}

bool fakelag::should_run( )
{
    config::anti_hack_t &config{ g_cfg.m_anti_hack };
    if ( !config.m_fakelag_enabled || !gui::bind_active( config.m_fakelag_key ) )
    {
        return false;
    }

    if ( ( g_cl.m_local->m_fFlags( ) & FL_DUCKING ) && g_cl.m_local->IsOnGround( ) )
    {
        return false;
    }

    if ( config.m_fakelag_mode == config::FAKELAG_MODE_ADAPTIVE && g_cl.m_local->m_vecVelocity( ).length( ) < 64.0f )
    {
        return false;
    }

    if ( config.m_fakelag_mode == config::FAKELAG_MODE_RANDOM && config.m_random_min > config.m_random_max )
    {
        return false;
    }

    return true;
}
