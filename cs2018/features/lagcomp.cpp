#include "lagcomp.hpp"
#include "bone_setup.hpp"

#include "math/math.hpp"

ATLAS_NO_DESTROY lag_compensation g_lagcomp{ };

constexpr i32 SIMULATION_TIME_WINDOW{ 127 };

#if defined( ATLAS_DEV )
static vec3 bone_origin( const matrix3x4 &bone )
{
    return vec3{ bone[ 0 ][ 3 ], bone[ 1 ][ 3 ], bone[ 2 ][ 3 ] };
}

static void draw_skeleton( const lag_record_t &entry, color_t value )
{
    for ( i32 index{ }; index < entry.m_bone_count; ++index )
    {
        const mstudiobone_t *bone{ entry.m_header->pBone( index ) };

        if ( bone->parent < 0 || !( bone->flags & BONE_USED_BY_HITBOX ) ||
             !( entry.m_header->pBone( bone->parent )->flags & BONE_USED_BY_HITBOX ) )
        {
            continue;
        }

        vec2 from{ };
        vec2 to{ };

        if ( !g_cl.world_to_screen( bone_origin( entry.m_bones[ index ] ), from ) ||
             !g_cl.world_to_screen( bone_origin( entry.m_bones[ bone->parent ] ), to ) )
        {
            continue;
        }

        g_draw.line( from, to, value );
    }
}
#endif // ATLAS_DEV

bool rewind_window_t::accepts( i32 tick ) const
{
    const float now{ TICKS_TO_TIME( this->m_tickbase ) };
    const float record{ TICKS_TO_TIME( tick ) };
    const float target{ TICKS_TO_TIME( this->command_tick( tick ) ) - this->m_lerp };

    return this->m_tickbase >= 0 &&
           now - record <= SV_MAXUNLAG &&
           math::abs( this->m_correct - ( now - target ) ) <= 0.2f;
}

float lag_compensation::lerp_time( )
{
    const float requested{ g_csgo.cl_updaterate->GetFloat( ) };
    const float minimum{ g_csgo.sv_minupdaterate->GetFloat( ) };
    const float updaterate{ minimum <= requested ? math::smaller( g_csgo.sv_maxupdaterate->GetFloat( ), requested ) : minimum };

    float ratio{ g_csgo.cl_interp_ratio->GetFloat( ) };
    if ( ratio == 0.0f )
    {
        ratio = 1.0f;
    }

    const float min_ratio{ g_csgo.sv_client_min_interp_ratio->GetFloat( ) };
    if ( min_ratio != -1.0f )
    {
        ratio = math::clamp( ratio, min_ratio, g_csgo.sv_client_max_interp_ratio->GetFloat( ) );
    }

    return math::bigger( ratio / updaterate, g_csgo.cl_interp->GetFloat( ) );
}

rewind_window_t lag_compensation::rewind_window( i32 tickbase )
{
    INetChannelInfo *net_channel{ g_csgo.m_engine->GetNetChannelInfo( ) };
    if ( !net_channel )
    {
        return rewind_window_t{ };
    }

    const float lerp{ lerp_time( ) };

    i32 lerp_ticks{ TIME_TO_TICKS( lerp ) };
    if ( TICKS_TO_TIME( lerp_ticks ) < lerp )
    {
        ++lerp_ticks;
    }

    rewind_window_t window{ };
    window.m_tickbase   = tickbase;
    window.m_lerp_ticks = lerp_ticks;
    window.m_lerp       = lerp;

    window.m_correct = math::clamp( net_channel->GetLatency( FLOW_OUTGOING ) + net_channel->GetLatency( FLOW_INCOMING ) + lerp,
                                    0.0f, SV_MAXUNLAG );
    return window;
}

C_CSPlayer *lag_compensation::trackable( i32 index ) const
{
    IClientEntity *client_entity{ g_csgo.m_entity_list->GetClientEntity( index ) };
    C_CSPlayer    *player{ client_entity ? client_entity->As< C_CSPlayer * >( ) : nullptr };

    if ( !player ||
         player->IsDormant( ) ||
         !player->IsAlive( ) ||
         player->m_AnimOverlay( ).count( ) < ANIMATION_LAYER_COUNT )
    {
        return nullptr;
    }

    if ( player->m_iTeamNum( ) == g_cl.m_local->m_iTeamNum( ) && !g_csgo.mp_friendlyfire->GetInt( ) &&
         !g_csgo.mp_teammates_are_enemies->GetInt( ) )
    {
        return nullptr;
    }

    return player;
}

void lag_compensation::invalidate( i32 index )
{
    track_t &track{ this->m_tracks[ index ] };
    track.m_player     = nullptr;
    track.m_head       = 0;
    track.m_count      = 0;
    track.m_break_tick = 0;
}

i32 lag_compensation::count( i32 index ) const
{
    if ( index < 1 || index >= MAX_TRACKED_PLAYERS )
    {
        return 0;
    }

    return this->m_tracks[ index ].m_count;
}

const lag_record_t *lag_compensation::record( i32 index, i32 age ) const
{
    if ( age < 0 || age >= this->count( index ) )
    {
        return nullptr;
    }

    const track_t &track{ this->m_tracks[ index ] };
    return &track.m_records[ ( track.m_head - age + MAX_RECORDS ) % MAX_RECORDS ];
}

bool lag_compensation::continuous( i32 index, const lag_record_t &entry ) const
{
    return entry.m_tick > this->m_tracks[ index ].m_break_tick;
}

bool lag_compensation::reachable( i32 index, const lag_record_t &entry, const rewind_window_t &window ) const
{
    return this->continuous( index, entry ) && window.accepts( entry.m_tick );
}

vec3 lag_compensation::rewound_origin( i32 index, i32 age, const rewind_window_t &window ) const
{
    const lag_record_t *entry{ this->record( index, age ) };
    const lag_record_t *newer{ this->record( index, age - 1 ) };
    Assert( entry != nullptr );

    if ( !newer )
    {
        return entry->m_origin;
    }

    const float record{ TICKS_TO_TIME( entry->m_tick ) };
    const float target{ TICKS_TO_TIME( window.command_tick( entry->m_tick ) ) - window.m_lerp };
    const float fraction{ ( target - record ) / ( TICKS_TO_TIME( newer->m_tick ) - record ) };
    return entry->m_origin + ( newer->m_origin - entry->m_origin ) * fraction;
}

void lag_compensation::store( i32 index, C_CSPlayer *player )
{
    track_t            &track{ this->m_tracks[ index ] };
    const lag_record_t *newest{ this->record( index, 0 ) };
    const i32           tick{ TIME_TO_TICKS( player->m_flSimulationTime( ) ) };
    const vec3          origin{ player->GetAbsOrigin( ) };

    if ( newest && ( tick <= newest->m_tick || ( tick % TIMESTAMP_BASE == 0 && origin == newest->m_origin ) ) )
    {
        return;
    }

    if ( !track.m_records )
    {
        constexpr usize size{ sizeof( lag_record_t ) * MAX_RECORDS };
        track.m_records = static_cast< lag_record_t * >( g_csgo.m_alloc->Alloc( size ) );

        if ( !track.m_records )
        {
            DBGOUT( "lagcomp[%d]: out of memory for %d records", index, MAX_RECORDS );
            return;
        }

        atl::memset( track.m_records, 0, size );
    }

    const i32     slot{ track.m_count > 0 ? ( track.m_head + 1 ) % MAX_RECORDS : 0 };
    lag_record_t &entry{ track.m_records[ slot ] };

    const remote_anim_record_t *animated{ g_anims.record( index, tick ) };
    entry.m_tick        = tick;
    entry.m_yaw_class   = animated ? animated->m_yaw_class : YAW_UNKNOWN;
    entry.m_misses      = animated ? animated->m_misses : 0;
    entry.m_standing    = animated && animated->m_standing;
    entry.m_yaw_bounded = animated && animated->m_yaw_bounded;
    entry.m_origin      = origin;
    entry.m_mins        = player->m_vecMins( );
    entry.m_maxs        = player->m_vecMaxs( );
    entry.m_header      = nullptr;
    entry.m_bone_count  = 0;

    const desync_edges_t *edges{ g_anims.edges( index, tick ) };
    entry.m_has_edges = edges != nullptr;
    if ( edges )
    {
        atl::memcpy( entry.m_edges, edges->m_poses, sizeof( entry.m_edges ) );
    }

    const studiohdr_t *header{ player->m_pStudioHdr( )->m_pStudioHdr };
    if ( header->numbones <= MAX_RECORD_BONES &&
         g_bone_mgr.player_setup( player, entry.m_bones, MAX_RECORD_BONES ) )
    {
        entry.m_header     = header;
        entry.m_bone_count = header->numbones;
    }

    if ( newest && origin.dist_to_sqr( newest->m_origin ) > LAG_COMPENSATION_TELEPORTED_DISTANCE_SQR )
    {
        DBGOUT( "lagcomp[%d]: teleport, %d units over %d ticks, chain cut", index,
                static_cast< i32 >( math::sqrt( origin.dist_to_sqr( newest->m_origin ) ) ), tick - newest->m_tick );
        track.m_break_tick = newest->m_tick;
    }

    track.m_head  = slot;
    track.m_count = math::smaller( track.m_count + 1, MAX_RECORDS );
}

void lag_compensation::update( )
{
    if ( !g_csgo.m_engine->IsInGame( ) || !g_cl.m_local )
    {
        return;
    }

#if defined( ATLAS_DEV )
    const float lerp{ lerp_time( ) };
    if ( lerp != this->m_reported_lerp )
    {
        this->m_reported_lerp = lerp;
        if ( TICKS_TO_TIME( TIME_TO_TICKS( lerp ) ) != lerp )
        {
            DBGOUT( "lagcomp: lerp %.6f is not a whole number of %.6f ticks, rewound origins slide toward the next record", lerp,
                    TICK_INTERVAL );
        }
    }
#endif // ATLAS_DEV

    const i32 local_index{ g_csgo.m_engine->GetLocalPlayer( ) };
    const i32 highest{ math::smaller( g_csgo.m_globals->maxClients, MAX_TRACKED_PLAYERS - 1 ) };

    for ( i32 index{ 1 }; index <= highest; ++index )
    {
        C_CSPlayer *player{ index == local_index ? nullptr : this->trackable( index ) };
        if ( !player )
        {
            this->invalidate( index );
            continue;
        }

        const lag_record_t *newest{ this->record( index, 0 ) };
        if ( this->m_tracks[ index ].m_player != player ||
             ( newest && newest->m_tick > g_csgo.m_globals->tickcount + SIMULATION_TIME_WINDOW ) )
        {
            this->invalidate( index );
            this->m_tracks[ index ].m_player = player;
        }

        this->store( index, player );
    }
}

#if defined( ATLAS_DEV )
void lag_compensation::paint( ) const
{
    if ( !g_cfg.m_debug.m_lag_records || !g_csgo.m_engine->IsInGame( ) || !g_cl.m_local )
    {
        return;
    }

    const rewind_window_t window{ rewind_window( g_cl.m_local->m_nTickBase( ) ) };

    for ( i32 index{ 1 }; index < MAX_TRACKED_PLAYERS; ++index )
    {
        for ( i32 age{ }; age < this->count( index ); ++age )
        {
            const lag_record_t *entry{ this->record( index, age ) };
            if ( entry->m_bone_count <= 0 || !this->reachable( index, *entry, window ) )
            {
                continue;
            }

            draw_skeleton( *entry, age == 0 ? g_cfg.m_debug.m_lag_newest_color : g_cfg.m_debug.m_lag_record_color );
        }
    }
}
#endif // ATLAS_DEV
