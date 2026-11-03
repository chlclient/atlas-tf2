#pragma once

#include "link.hpp"

enum class move_failure_t
{
    none,
    disconnected,
    host_stopped,
    demo_playback,
    deferred,
    encode,
    move_message,
};

struct move_result_t
{
    bool           m_generated{ };
    bool           m_sent{ };
    i32            m_sequence{ -1 };
    i32            m_commands{ };
    move_failure_t m_failure{ move_failure_t::none };
};

struct sent_packet_t
{
    i32 m_sequence{ -1 };
    i32 m_command{ };
};

class packet_history
{
public:
    static constexpr usize CAPACITY{ 256 };

    constexpr void reset( i32 sequence, i32 command, i32 acknowledged )
    {
        for ( sent_packet_t &packet : this->m_sent )
        {
            packet = sent_packet_t{ };
        }

        this->m_next          = 0;
        this->m_last_sequence = sequence;
        this->m_move_sequence = sequence;
        this->m_last_command  = command;
        this->m_acknowledged  = acknowledged;
        this->m_ack_sequence  = -1;
    }

    constexpr void record( i32 sequence, i32 command, bool movement )
    {
        this->m_sent[ this->m_next ] = sent_packet_t{ sequence, command };
        this->m_next                 = ( this->m_next + 1 ) % CAPACITY;
        this->m_last_sequence        = sequence;
        this->m_move_sequence        = movement ? sequence : this->m_move_sequence;
        this->m_last_command         = math::bigger( this->m_last_command, command );
    }

    constexpr i32 acknowledge( i32 sequence )
    {
        for ( const sent_packet_t &packet : this->m_sent )
        {
            if ( packet.m_sequence == sequence && packet.m_sequence >= 0 )
            {
                this->m_acknowledged = math::bigger( this->m_acknowledged, packet.m_command );
                this->m_ack_sequence = math::bigger( this->m_ack_sequence, sequence );
                break;
            }
        }

        return this->m_acknowledged;
    }

    constexpr i32 known_sequence( ) const
    {
        return math::bigger( this->m_ack_sequence, this->m_move_sequence );
    }

    constexpr bool can_keep_alive( ) const
    {
        return this->m_acknowledged >= this->m_last_command;
    }

    constexpr i32 last_sequence( ) const
    {
        return this->m_last_sequence;
    }

    constexpr i32 last_acknowledged( ) const
    {
        return this->m_acknowledged;
    }

    constexpr i32 acknowledged_sequence( ) const
    {
        return this->m_ack_sequence;
    }

private:
    sent_packet_t m_sent[ CAPACITY ]{ };
    usize         m_next{ };
    i32           m_last_sequence{ -1 };
    i32           m_move_sequence{ -1 };
    i32           m_last_command{ };
    i32           m_acknowledged{ };
    i32           m_ack_sequence{ -1 };
};

class networking
{
private:
    move_failure_t cl_sendmove( CClientState *cl, i32 command, i32 commands );
    bool           send_tick( CClientState *cl );
    bool           synchronize( );
    bool           send_datagram( i32 command, bool movement );
    bool           send_keep_alive( );
    double         ack_budget( ) const;

    packet_history m_packets{ };
    CNetChannel   *m_channel{ };
    i32            m_keep_frame{ -1 };
    i32            m_sent_reliable_state{ -1 };
    double         m_last_send_time{ };
    bool           m_warned_timeout{ };

public:
    move_result_t cl_move( float accumulated_extra_samples, bool final_tick );
    bool          can_send_now( bool final_tick ) const;
    i32           sequence_number( ) const;
    i32           acknowledged( i32 sequence );
    void          on_choked( );
    void          clear( );
};

extern networking g_networking;
