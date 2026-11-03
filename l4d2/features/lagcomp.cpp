#include "lagcomp.hpp"

ATLAS_NO_DESTROY lag_compensation g_lagcomp{ };

static constexpr float TELEPORT_DISTANCE_SQR{ 64.0f * 64.0f };
static constexpr float MAX_EXTRAPOLATION{ 0.25f };

float lag_compensation::lerp_time( )
{
    if ( !g_l4d2.cl_interpolate->GetInt( ) )
    {
        return 0.0f;
    }

    const i32 updaterate{ math::clamp( g_l4d2.cl_updaterate->GetInt( ),
                                       g_l4d2.sv_minupdaterate->GetInt( ),
                                       g_l4d2.sv_maxupdaterate->GetInt( ) ) };

    float       ratio{ g_l4d2.cl_interp_ratio->GetFloat( ) };
    const float min_ratio{ g_l4d2.sv_client_min_interp_ratio->GetFloat( ) };

    if ( ratio == 0.0f )
    {
        ratio = 1.0f;
    }

    if ( min_ratio != -1.0f )
    {
        ratio = math::clamp( ratio, min_ratio, g_l4d2.sv_client_max_interp_ratio->GetFloat( ) );
    }

    return math::bigger( g_l4d2.cl_interp->GetFloat( ),
                         ratio / static_cast< float >( math::bigger( updaterate, 1 ) ) );
}

rewind_window_t lag_compensation::window( ) const
{
    CNetChannel *channel{ g_l4d2.m_split_screen->GetClientState( g_l4d2.m_engine->GetActiveSplitScreenPlayerSlot( ) )->m_NetChannel };
    const float  lerp{ lerp_time( ) };

    return rewind_window_t{ g_l4d2.m_globals->curtime,
                            lerp,
                            math::clamp( channel->GetLatency( FLOW_OUTGOING ) + lerp, 0.0f, MAX_UNLAG ) };
}

i32 lag_compensation::command_tick( float time ) const
{
    return TIME_TO_TICKS( time + lerp_time( ) );
}

void lag_compensation::invalidate( i32 index )
{
    track_t &track{ this->m_tracks[ index ] };

    track.m_head   = 0;
    track.m_count  = 0;
    track.m_handle = 0;
}

void lag_compensation::clear( )
{
    for ( track_t &track : this->m_tracks )
    {
        if ( track.m_records )
        {
            g_l4d2.m_alloc->Free( track.m_records );
        }

        track = track_t{ };
    }
}

i32 lag_compensation::count( i32 index ) const
{
    return this->m_tracks[ index ].m_count;
}

const lag_record_t *lag_compensation::record( i32 index, i32 age ) const
{
    const track_t &track{ this->m_tracks[ index ] };

    Assert( age >= 0 && age < track.m_count );
    return &track.m_records[ ( track.m_head - age + MAX_RECORDS ) % MAX_RECORDS ];
}

void lag_compensation::store( i32 index, C_BaseEntity *entity, usize kind )
{
    track_t    &track{ this->m_tracks[ index ] };
    const u32   handle{ static_cast< u32 >( entity->GetRefEHandle( ).ToInt( ) ) };
    const float simtime{ entity->m_flSimulationTime( ) };

    if ( track.m_handle != handle )
    {
        this->invalidate( index );
        track.m_handle = handle;
    }

    // ::FrameUpdatePostEntityThink only records an entity that simulated since its newest record
    if ( track.m_count > 0 && simtime <= this->record( index, 0 )->m_simtime )
    {
        return;
    }

    if ( !track.m_records )
    {
        track.m_records = static_cast< lag_record_t * >( g_l4d2.m_alloc->Alloc( sizeof( lag_record_t ) * MAX_RECORDS ) );
        if ( !track.m_records )
        {
            return;
        }
    }

    const float yaw{ kind == static_cast< usize >( config::esp_target_t::enemy ) ? entity->As< C_TerrorPlayer * >( )->m_angEyeAngles( ).y
                                                                                 : entity->m_angRotation( ).y };
    const i32   slot{ track.m_count > 0 ? ( track.m_head + 1 ) % MAX_RECORDS : 0 };

    track.m_records[ slot ] = lag_record_t{ entity->m_vecOrigin( ), yaw, simtime };
    track.m_head            = slot;
    track.m_count           = math::smaller( track.m_count + 1, MAX_RECORDS );
}

void lag_compensation::update( )
{
    C_TerrorPlayer *local{ g_cl.m_local };
    if ( !local )
    {
        return;
    }

    const i32 highest{ g_l4d2.m_entity_list->GetHighestEntityIndex( ) };
    for ( i32 index{ 1 }; index <= highest; ++index )
    {
        IClientEntity *client_entity{ g_l4d2.m_entity_list->GetClientEntity( index ) };
        if ( !client_entity )
        {
            this->invalidate( index );
            continue;
        }

        C_BaseEntity *entity{ client_entity->As< C_BaseEntity * >( ) };
        const usize   kind{ entity == local || entity->IsDormant( ) ? config::TARGET_COUNT : g_cl.aim_target( entity ) };
        if ( kind == config::TARGET_COUNT )
        {
            this->invalidate( index );
            continue;
        }

        this->store( index, entity, kind );
    }
}

bool lag_compensation::rewind( i32 index, float time, lag_record_t &out ) const
{
    if ( this->count( index ) == 0 )
    {
        return false;
    }

    const lag_record_t *newer{ };
    const lag_record_t *entry{ };

    for ( i32 age{ }; age < this->count( index ); ++age )
    {
        entry = this->record( index, age );

        if ( newer && ( newer->m_origin - entry->m_origin ).length_sqr( ) > TELEPORT_DISTANCE_SQR )
        {
            return false;
        }

        if ( entry->m_simtime <= time )
        {
            break;
        }

        newer = entry;
    }

    // a target older than the whole history lands on the oldest record, like the server's walk
    out = *entry;

    if ( newer && time > entry->m_simtime )
    {
        const float fraction{ ( time - entry->m_simtime ) / ( newer->m_simtime - entry->m_simtime ) };

        out.m_origin += ( newer->m_origin - entry->m_origin ) * fraction;
        out.m_yaw = math::normalize_yaw( entry->m_yaw + math::normalize_yaw( newer->m_yaw - entry->m_yaw ) * fraction );
    }

    return true;
}

bool lag_compensation::live( i32 index, float curtime, lag_record_t &out ) const
{
    if ( this->count( index ) == 0 )
    {
        return false;
    }

    const lag_record_t *newest{ this->record( index, 0 ) };

    out = *newest;

    if ( this->count( index ) < 2 )
    {
        return true;
    }

    const lag_record_t *previous{ this->record( index, 1 ) };
    const vec3          step{ newest->m_origin - previous->m_origin };

    if ( step.length_sqr( ) <= TELEPORT_DISTANCE_SQR )
    {
        out.m_origin += step * ( math::clamp( curtime - newest->m_simtime, 0.0f, MAX_EXTRAPOLATION ) / ( newest->m_simtime - previous->m_simtime ) );
    }

    return true;
}

bool lag_compensation::compensated( C_BaseEntity *entity ) const
{
    const EL4D2ClassID kind{ entity->GetClientClass( )->GetClassID( ) };

    if ( kind == EL4D2ClassID::Infected || kind == EL4D2ClassID::Witch )
    {
        return true;
    }

    auto *player{ entity->As< C_TerrorPlayer * >( ) };

    if ( player->m_carryAttacker( ).IsValid( ) )
    {
        return false;
    }

    if ( kind != EL4D2ClassID::Jockey )
    {
        return true;
    }

    for ( i32 index{ 1 }; index <= g_l4d2.m_globals->maxClients; ++index )
    {
        IClientEntity *client_entity{ g_l4d2.m_entity_list->GetClientEntity( index ) };

        if ( client_entity && client_entity->As< C_TerrorPlayer * >( )->m_jockeyAttacker( ).Get( ) == entity )
        {
            return false;
        }
    }

    return true;
}
