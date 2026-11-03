#include "networking.hpp"

#include "gui/notifications.hpp"
#include "math/math.hpp"

ATLAS_NO_DESTROY networking g_networking{ };

ATLAS_FORCE_INLINE u32 to_tick_microseconds( float seconds )
{
    return math::smaller( static_cast< u32 >( static_cast< double >( seconds ) * 1000000.0 ), u32{ 1000000 } );
}

i32 networking::sequence_number( ) const
{
    const CClientState *cl{ g_csgo.m_client_states[ 0 ] };
    return cl->lastoutgoingcommand + cl->chokedcommands + 1;
}

bool networking::can_send_now( bool final_tick ) const
{
    CClientState *cl{ g_csgo.m_client_states[ 0 ] };
    CNetChannel  *channel{ cl->m_NetChannel };

    if ( channel->IsLoopback( ) && !g_csgo.host_limitlocal->GetInt( ) )
    {
        return true;
    }

    return !( cl->m_flNextCmdTime > *g_csgo.m_net_time ) && channel->CanPacket( ) && final_tick;
}

void networking::clear( )
{
    this->m_packets.reset( -1, 0, 0 );
    this->m_channel             = nullptr;
    this->m_keep_frame          = -1;
    this->m_sent_reliable_state = -1;
    this->m_last_send_time      = 0.0;
}

bool networking::synchronize( )
{
    CClientState *cl{ g_csgo.m_client_states[ 0 ] };
    if ( cl->m_nSignonState != SIGNONSTATE_FULL )
    {
        this->clear( );
        return false;
    }

    CNetChannel *channel{ cl->m_NetChannel };
    const i32    last_sent{ channel->m_nOutSequenceNr - channel->m_nChokedPackets - 1 };

    if ( this->m_channel != channel || last_sent < this->m_packets.last_sequence( ) )
    {
        this->clear( );
        this->m_channel = channel;
        this->m_packets.reset( last_sent, cl->lastoutgoingcommand, cl->last_command_ack );
    } else if ( last_sent > this->m_packets.last_sequence( ) )
    {
        this->m_packets.record( last_sent, this->m_packets.last_acknowledged( ), false );
        this->m_last_send_time = *g_csgo.m_net_time;
    }

    return true;
}

i32 networking::acknowledged( i32 sequence )
{
    const CClientState *cl{ g_csgo.m_client_states[ 0 ] };
    if ( !this->m_channel || cl->m_nSignonState != SIGNONSTATE_FULL || this->m_channel != cl->m_NetChannel )
    {
        return sequence;
    }

    this->synchronize( );
    return this->m_packets.acknowledge( sequence );
}

bool networking::send_datagram( i32 command, bool movement )
{
    const i32 sequence{ this->m_channel->m_nOutSequenceNr };
    const i32 choked{ this->m_channel->m_nChokedPackets };

    if ( movement )
    {
        this->m_channel->m_nChokedPackets = sequence - this->m_packets.known_sequence( ) - 1;
    } else
    {
        this->m_channel->m_nOutSequenceNr = this->m_packets.last_sequence( ) + 1;
        this->m_channel->m_nChokedPackets = 0;
    }

    const i32 sent{ this->m_channel->SendDatagram( nullptr ) };
    if ( sent <= 0 )
    {
        DBGOUT( "net tx failed: result=%d seq=%d cmd=%d", sent, sequence, command );
        this->m_channel->m_nOutSequenceNr = sequence;
        this->m_channel->m_nChokedPackets = choked;
        return false;
    }

    if ( !movement )
    {
        this->m_channel->m_nOutSequenceNr = sequence + 1;
        this->m_channel->m_nChokedPackets = sequence - sent;
    }

    this->m_packets.record( sent, command, movement );
    this->m_last_send_time      = *g_csgo.m_net_time;
    this->m_sent_reliable_state = this->m_channel->m_nInReliableState;
    return true;
}

double networking::ack_budget( ) const
{
    return math::bigger( static_cast< double >( this->m_channel->GetLatency( FLOW_OUTGOING ) ), 0.0 ) + 2.0 * TICK_INTERVAL;
}

bool networking::send_keep_alive( )
{
    if ( g_csgo.m_demo_player->IsPlayingBack( ) || this->m_keep_frame == g_csgo.m_globals->framecount ||
         !this->m_channel->CanPacket( ) )
    {
        return false;
    }

    const bf_write queued{ this->m_channel->m_StreamUnreliable };
    if ( !this->send_tick( g_csgo.m_client_states[ 0 ] ) ||
         !this->send_datagram( this->m_packets.last_acknowledged( ), false ) )
    {
        this->m_channel->m_StreamUnreliable = queued;
        return false;
    }

    this->m_keep_frame = g_csgo.m_globals->framecount;
    return true;
}

void networking::on_choked( )
{
    if ( !g_cl.m_final_tick || !this->synchronize( ) )
    {
        return;
    }

    const bool control_overdue{ this->m_packets.acknowledged_sequence( ) < this->m_packets.last_sequence( ) &&
                                *g_csgo.m_net_time - this->m_last_send_time > this->ack_budget( ) };
    if ( this->m_channel->m_nInReliableState == this->m_sent_reliable_state &&
         this->m_channel->m_StreamReliable.m_iCurBit == 0 &&
         !control_overdue )
    {
        return;
    }

    if ( !this->m_packets.can_keep_alive( ) )
    {
        return;
    }

    this->send_keep_alive( );
}

move_failure_t networking::cl_sendmove( CClientState *cl, i32 command, i32 commands )
{
    alignas( 4 ) u8 data[ MAX_CMD_BUFFER ]{ };

    bf_write buffer{ };
    buffer.StartWriting( data, sizeof( data ) );

    const i32 new_commands{ math::clamp( commands, 0, MAX_NEW_COMMANDS ) };
    const i32 total{ new_commands + NUM_BACKUP_COMMANDS };

    i32  from{ -1 };
    bool ok{ true };

    for ( i32 to{ command - total + 1 }; to <= command; ++to )
    {
        const bool is_new_command{ to >= command - new_commands + 1 };
        ok   = ok && g_csgo.m_client->WriteUsercmdDeltaToBuffer( 0, &buffer, from, to, is_new_command );
        from = to;
    }

    if ( !ok )
    {
        DBGOUT( "net move encode failed: cmd=%d new=%d backup=%d bits=%d overflow=%d",
                command, new_commands, NUM_BACKUP_COMMANDS, buffer.m_iCurBit, buffer.m_bOverflow );
        return move_failure_t::encode;
    }

    std_string_t written{ data, { }, static_cast< usize >( buffer.GetNumBytesWritten( ) ), sizeof( data ) };

    CCLCMsg_Move move{ };
    move.m_vmt               = g_csgo.m_CCLCMsg_Move_vmt;
    move.m_message_vmt       = g_csgo.m_CCLCMsg_Move_message_vmt;
    move.num_backup_commands = NUM_BACKUP_COMMANDS;
    move.num_new_commands    = new_commands;
    move.data                = &written;
    move.m_has_bits          = 0x7;

    if ( !cl->m_NetChannel->SendNetMsg( move, false, false ) )
    {
        DBGOUT( "net move message failed: cmd=%d new=%d bytes=%d", command, new_commands, buffer.GetNumBytesWritten( ) );
        return move_failure_t::move_message;
    }

    return move_failure_t::none;
}

bool networking::send_tick( CClientState *cl )
{
    CNETMsg_Tick tick{ };
    tick.m_vmt                              = g_csgo.m_CNETMsg_Tick_vmt;
    tick.m_message_vmt                      = g_csgo.m_CNETMsg_Tick_message_vmt;
    tick.tick                               = static_cast< u32 >( cl->m_nDeltaTick );
    tick.host_computationtime               = to_tick_microseconds( *g_csgo.m_host_frameendtime_computationduration );
    tick.host_computationtime_std_deviation = to_tick_microseconds( *g_csgo.m_host_frametime_stddeviation );
    tick.host_framestarttime_std_deviation  = to_tick_microseconds( *g_csgo.m_host_framestarttime_stddeviation );
    tick.m_has_bits                         = 0xF;

    if ( cl->m_nHltvReplayDelay )
    {
        tick.hltv_replay_flags = 1;
        tick.m_has_bits |= 0x10;
    }

    return cl->m_NetChannel->SendNetMsg( tick, false, false );
}

move_result_t networking::cl_move( float accumulated_extra_samples, bool final_tick )
{
    move_result_t result{ };
    CClientState *cl{ g_csgo.m_client_states[ 0 ] };

    if ( cl->m_nSignonState < SIGNONSTATE_CONNECTED )
    {
        result.m_failure = move_failure_t::disconnected;
        return result;
    }

    if ( !g_csgo.m_Host_ShouldRun_fn.call_as< bool >( ) )
    {
        result.m_failure = move_failure_t::host_stopped;
        return result;
    }

    g_cl.m_send_packet = true;
    g_cl.m_final_tick  = final_tick;

    if ( g_csgo.m_demo_player->IsPlayingBack( ) )
    {
        if ( !cl->ishltv )
        {
            result.m_failure = move_failure_t::demo_playback;
            return result;
        }

        g_cl.m_send_packet = false;
    }

    if ( !this->can_send_now( final_tick ) )
    {
        g_cl.m_send_packet = false;
    }

    const bool active{ this->synchronize( ) };

    const bool create{ active && cl->chokedcommands < MAX_NEW_COMMANDS };
    i32        command{ cl->lastoutgoingcommand + cl->chokedcommands };

    if ( create )
    {
        command = this->sequence_number( );

        g_csgo.m_client->CreateMove( command, *g_csgo.m_host_interval_per_tick - accumulated_extra_samples, !cl->IsPaused( ) );

        if ( g_csgo.m_demo_recorder->IsRecording( ) )
        {
            g_csgo.m_demo_recorder->RecordUserInput( command );
        }

        result.m_generated = true;
        result.m_sequence  = command;

        if ( !g_cl.m_send_packet )
        {
            cl->m_NetChannel->SetChoked( );
            ++cl->chokedcommands;
            this->on_choked( );
            result.m_failure = move_failure_t::deferred;
            return result;
        }
    }

    if ( !g_cl.m_send_packet )
    {
        result.m_failure = move_failure_t::deferred;
        return result;
    }

    if ( active )
    {
        const i32 commands{ cl->chokedcommands + ( create ? 1 : 0 ) };
        result.m_failure = this->cl_sendmove( cl, command, commands );
        if ( result.m_failure == move_failure_t::none )
        {
            result.m_commands = commands;
        }
    }

    const bool timing_out{ cl->m_NetChannel->IsTimingOut( ) && !g_csgo.m_demo_player->IsPlayingBack( ) &&
                           cl->m_nSignonState == SIGNONSTATE_FULL };

    if ( timing_out != this->m_warned_timeout )
    {
        this->m_warned_timeout = timing_out;

        if ( timing_out )
        {
            push_notification( notify_kind_t::warning, ICON_ZAP,
                               ATLAS_XORSTR_STATIC( "Networking" ),
                               ATLAS_XORSTR_STATIC( "Connection is timing out; the server has "
                                                    "stopped acknowledging our packets." ) );
        }
    }

    if ( timing_out )
    {
        cl->ForceFullUpdate( ATLAS_XORSTR( "connection problem" ).get( ) );
    }

    if ( active )
    {
        this->send_tick( cl );
        this->send_datagram( command, true );
        cl->lastoutgoingcommand = command;
    } else
    {
        cl->lastoutgoingcommand = cl->m_NetChannel->SendDatagram( nullptr );
        if ( cl->lastoutgoingcommand <= 0 )
        {
            DBGOUT( "net tx failed: result=%d cmd=%d", cl->lastoutgoingcommand, result.m_sequence );
        }
    }

    cl->chokedcommands = 0;
    result.m_sent      = true;

    if ( cl->m_nSignonState != SIGNONSTATE_FULL )
    {
        cl->m_flNextCmdTime = *g_csgo.m_net_time + 0.2;
        return result;
    }

    const float command_interval{ 1.0f / g_csgo.cl_cmdrate->GetFloat( ) };
    const float max_delta{ math::smaller( *g_csgo.m_host_interval_per_tick, command_interval ) };
    const float delta{ math::clamp( static_cast< float >( *g_csgo.m_net_time - cl->m_flNextCmdTime ), 0.0f, max_delta ) };
    cl->m_flNextCmdTime = *g_csgo.m_net_time + command_interval - delta;
    return result;
}
