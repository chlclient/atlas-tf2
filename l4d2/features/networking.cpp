#include "networking.hpp"

#include "gui/notifications.hpp"

ATLAS_NO_DESTROY networking g_networking{ };

template< typename fn_t >
ATLAS_FORCE_INLINE void for_each_split_screen_player( fn_t &&fn )
{
    CSplitScreen *split_screen{ g_l4d2.m_split_screen };

    for ( i32 slot{ split_screen->FirstValidSplitScreenSlot( ) }; slot != -1; slot = split_screen->NextValidSplitScreenSlot( slot ) )
    {
        const i32  previous_slot{ split_screen->SetActiveSplitScreenPlayerSlot( slot ) };
        const bool previous_resolvable{ split_screen->SetLocalPlayerIsResolvable( nullptr, 0, true ) };

        if ( !split_screen->IsDisconnecting( slot ) )
        {
            fn( slot );
        }

        split_screen->SetActiveSplitScreenPlayerSlot( previous_slot );
        split_screen->SetLocalPlayerIsResolvable( nullptr, 0, previous_resolvable );
    }
}

void networking::remove_input_delay( bool final_tick )
{
    CNetChannel *channel{ g_l4d2.m_split_screen->GetClientState( 0 )->m_NetChannel };
    if ( !channel || channel->IsLoopback( ) || !g_l4d2.m_engine->IsInGame( ) )
    {
        return;
    }

    globals_backup_t frame{ };
    frame.store( );

    g_hooks.m_cl_read_packets.ccall< void >( final_tick );
    this->m_state.store( );
    this->m_read_early = true;

    frame.restore( );
}

bool networking::should_read_packets( )
{
    if ( !this->m_read_early )
    {
        return true;
    }

    this->m_read_early = false;
    this->m_state.restore( );
    return false;
}

void networking::cl_sendmove( CClientState *cl )
{
    alignas( 4 ) u8 data[ MAX_CMD_BUFFER ]{ };

    const i32 command{ cl->lastoutgoingcommand + cl->chokedcommands + 1 };
    const i32 new_commands{ math::clamp( cl->chokedcommands + 1, 0, MAX_NEW_COMMANDS ) };

    for_each_split_screen_player( [ & ]( i32 slot ) {
        CLC_Move move{ };
        move.m_vmt = g_l4d2.m_CLC_Move_vmt;
        move.m_DataOut.StartWriting( data, sizeof( data ) );

        move.m_nBackupCommands = NUM_BACKUP_COMMANDS;
        move.m_nNewCommands    = new_commands;

        const i32 total{ move.m_nNewCommands + move.m_nBackupCommands };

        i32  from{ -1 };
        bool ok{ true };

        for ( i32 to{ command - total + 1 }; to <= command; ++to )
        {
            const bool is_new_command{ to >= command - move.m_nNewCommands + 1 };
            ok   = ok && g_l4d2.m_client->WriteUsercmdDeltaToBuffer( slot, &move.m_DataOut, from, to, is_new_command );
            from = to;
        }

        if ( ok )
        {
            g_l4d2.m_split_screen->GetClientState( slot )->m_NetChannel->SendNetMsg( move, false, false );
        }
    } );
}

void networking::cl_move( float accumulated_extra_samples, bool final_tick )
{
    CClientState *cl{ g_l4d2.m_split_screen->GetClientState( 0 ) };
    if ( cl->m_nSignonState < SIGNONSTATE_CONNECTED || !g_l4d2.m_Host_ShouldRun.call_as< bool >( ) )
    {
        return;
    }

    g_cl.m_send_packet = true;

    if ( g_l4d2.m_demo_player->IsPlayingBack( ) )
    {
        if ( !cl->ishltv )
        {
            return;
        }

        g_cl.m_send_packet = false;
    }

    CNetChannel *channel{ cl->m_NetChannel };
    if ( ( !channel->IsLoopback( ) || g_l4d2.host_limitlocal->GetInt( ) ) &&
         ( cl->m_flNextCmdTime > *g_l4d2.m_net_time || !channel->CanPacket( ) || !final_tick ) )
    {
        g_cl.m_send_packet = false;
    }

    if ( cl->m_nSignonState == SIGNONSTATE_FULL )
    {
        const i32 command{ cl->lastoutgoingcommand + cl->chokedcommands + 1 };

        for_each_split_screen_player( [ & ]( i32 ) {
            g_l4d2.m_client->CreateMove( command, TICK_INTERVAL - accumulated_extra_samples, !cl->IsPaused( ) );

            if ( g_l4d2.m_demo_recorder->IsRecording( ) )
            {
                g_l4d2.m_demo_recorder->RecordUserInput( command );
            }
        } );

        if ( !g_cl.m_send_packet )
        {
            channel->SetChoked( );
            ++cl->chokedcommands;
            return;
        }

        this->cl_sendmove( cl );
    }

    if ( !g_cl.m_send_packet )
    {
        return;
    }

    const bool timing_out{ channel->IsTimingOut( ) && !g_l4d2.m_demo_player->IsPlayingBack( ) && cl->m_nSignonState == SIGNONSTATE_FULL };

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

    if ( cl->m_nSignonState == SIGNONSTATE_FULL )
    {
        NET_Tick tick{ };
        tick.m_vmt                         = g_l4d2.m_NET_Tick_vmt;
        tick.m_nTick                       = cl->m_nDeltaTick;
        tick.m_flHostFrameTime             = *g_l4d2.m_host_frametime_unbounded;
        tick.m_flHostFrameTimeStdDeviation = *g_l4d2.m_host_frametime_stddeviation;
        channel->SendNetMsg( tick, false, false );
    }

    cl->lastoutgoingcommand = channel->SendDatagram( nullptr );
    cl->chokedcommands      = 0;

    if ( cl->m_nSignonState != SIGNONSTATE_FULL )
    {
        cl->m_flNextCmdTime = *g_l4d2.m_net_time + 0.2;
        return;
    }

    const float command_interval{ 1.0f / g_l4d2.cl_cmdrate->GetFloat( ) };
    const float max_delta{ math::smaller( TICK_INTERVAL, command_interval ) };
    const float delta{ math::clamp( static_cast< float >( *g_l4d2.m_net_time - cl->m_flNextCmdTime ), 0.0f, max_delta ) };
    cl->m_flNextCmdTime = *g_l4d2.m_net_time + command_interval - delta;
}
