#pragma once

#include "link.hpp"

class networking
{
private:
    void cl_sendmove( CClientState *cl );

    globals_backup_t m_state{ };
    bool             m_read_early{ };
    bool             m_warned_timeout{ };

public:
    void cl_move( float accumulated_extra_samples, bool final_tick );
    void remove_input_delay( bool final_tick );
    bool should_read_packets( );
};

extern networking g_networking;
