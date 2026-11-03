#include "server_hitboxes.hpp"

ATLAS_NO_DESTROY server_hitboxes g_server_hitboxes{ };

ATLAS_FORCE_INLINE bool clip_segment_to_screen( const vec3 &start, const vec3 &end, vec2 &screen_start, vec2 &screen_end )
{
    constexpr float NEAR_W{ 0.01f };

    const auto &m = g_cl.m_w2s;

    const auto projected_w{ [ & ]( const vec3 &point )
    {
        return m[ 3 ][ 0 ] * point.x + m[ 3 ][ 1 ] * point.y + m[ 3 ][ 2 ] * point.z + m[ 3 ][ 3 ];
    } };

    const float start_w{ projected_w( start ) };
    const float end_w{ projected_w( end ) };

    if ( start_w < NEAR_W && end_w < NEAR_W )
    {
        return false;
    }

    vec3 visible_start{ start };
    vec3 visible_end{ end };

    if ( start_w < NEAR_W )
    {
        visible_start = start + ( end - start ) * ( ( NEAR_W - start_w ) / ( end_w - start_w ) );
    } else if ( end_w < NEAR_W )
    {
        visible_end = end + ( start - end ) * ( ( NEAR_W - end_w ) / ( start_w - end_w ) );
    }

    return g_cl.world_to_screen( visible_start, screen_start ) && g_cl.world_to_screen( visible_end, screen_end );
}

ATLAS_FORCE_INLINE void draw_3d_box( const matrix3x4 &transform, const vec3 &mins, const vec3 &maxs, const color_t &col )
{
    vec3 points[ 8 ]{ };
    for ( i32 i{ }; i < 8; ++i )
    {
        points[ i ].x = ( i & 0x1 ) ? maxs.x : mins.x;
        points[ i ].y = ( i & 0x2 ) ? maxs.y : mins.y;
        points[ i ].z = ( i & 0x4 ) ? maxs.z : mins.z;

        points[ i ] = math::transform_point( points[ i ], transform );
    }

    static const i32 face_indices[ 6 ][ 4 ]{
        { 0, 4, 6, 2 },
        { 5, 1, 3, 7 },
        { 0, 1, 5, 4 },
        { 2, 6, 7, 3 },
        { 0, 2, 3, 1 },
        { 4, 5, 7, 6 },
    };

    for ( const auto &face : face_indices )
    {
        for ( i32 j{ }; j < 4; ++j )
        {
            vec2 from{ };
            vec2 to{ };
            if ( !clip_segment_to_screen( points[ face[ j ] ], points[ face[ j == 3 ? 0 : j + 1 ] ], from, to ) )
            {
                continue;
            }

            g_draw.line( from, to, col );
        }
    }
}

ATLAS_FORCE_INLINE void draw_3d_segment( const vec3 &start, const vec3 &end, const color_t &col )
{
    vec2 from{ };
    vec2 to{ };
    if ( clip_segment_to_screen( start, end, from, to ) )
    {
        g_draw.line( from, to, col );
    }
}

ATLAS_FORCE_INLINE void draw_3d_capsule( const vec3 &start, const vec3 &end, float radius, const color_t &col )
{
    constexpr i32 SEGMENTS{ 12 };

    vec3 axis{ end - start };
    if ( axis.normalize( ) == 0.0f )
    {
        axis = vec3{ 0.0f, 0.0f, 1.0f };
    }

    const vec3 seed{ math::abs( axis.z ) < 0.9f ? vec3{ 0.0f, 0.0f, 1.0f } : vec3{ 1.0f, 0.0f, 0.0f } };
    const vec3 side{ axis.cross( seed ).normalized( ) };
    const vec3 up{ axis.cross( side ) };

    for ( i32 index{ }; index < SEGMENTS; ++index )
    {
        const float from_angle{ 2.0f * PI * static_cast< float >( index ) / SEGMENTS };
        const float to_angle{ 2.0f * PI * static_cast< float >( index + 1 ) / SEGMENTS };
        const vec3  from{ ( side * math::cos( from_angle ) + up * math::sin( from_angle ) ) * radius };
        const vec3  to{ ( side * math::cos( to_angle ) + up * math::sin( to_angle ) ) * radius };

        draw_3d_segment( start + from, start + to, col );
        draw_3d_segment( end + from, end + to, col );
    }

    const vec3 directions[]{ side, up };

    for ( const vec3 &direction : directions )
    {
        for ( i32 index{ }; index < SEGMENTS / 2; ++index )
        {
            const float from_angle{ PI * static_cast< float >( index ) / ( SEGMENTS / 2 ) };
            const float to_angle{ PI * static_cast< float >( index + 1 ) / ( SEGMENTS / 2 ) };
            const vec3  across_from{ direction * ( math::cos( from_angle ) * radius ) };
            const vec3  across_to{ direction * ( math::cos( to_angle ) * radius ) };
            const vec3  along_from{ axis * ( math::sin( from_angle ) * radius ) };
            const vec3  along_to{ axis * ( math::sin( to_angle ) * radius ) };

            draw_3d_segment( end + across_from + along_from, end + across_to + along_to, col );
            draw_3d_segment( start + across_from - along_from, start + across_to - along_to, col );
        }

        draw_3d_segment( start + direction * radius, end + direction * radius, col );
        draw_3d_segment( start - direction * radius, end - direction * radius, col );
    }
}

ATLAS_FORCE_INLINE void draw_hitbox( const matrix3x4 &bone_to_world, const mstudiobbox_t *box, const color_t &col )
{
    matrix3x4 offset{ };
    math::angle_matrix( box->angOffsetOrientation, vec3{ }, offset );

    matrix3x4 hitbox_to_world{ };
    math::concat_transforms( bone_to_world, offset, hitbox_to_world );

    if ( box->flCapsuleRadius > 0.0f )
    {
        draw_3d_capsule( math::transform_point( box->bbmin, hitbox_to_world ),
                         math::transform_point( box->bbmax, hitbox_to_world ),
                         box->flCapsuleRadius, col );
        return;
    }

    draw_3d_box( hitbox_to_world, box->bbmin, box->bbmax, col );
}

void server_hitboxes::paint( ) const
{
    if ( !g_cfg.m_debug.m_server_hitboxes && !g_cfg.m_debug.m_client_hitboxes && !g_cfg.m_debug.m_lag_server )
    {
        return;
    }

    if ( !g_csgo.m_engine->IsInGame( ) || !g_cl.m_local )
    {
        return;
    }

    const i32  max_clients{ g_csgo.m_engine->GetMaxClients( ) };
    const i32  local_index{ g_csgo.m_engine->GetLocalPlayer( ) };
    const bool thirdperson{ g_csgo.m_input->CAM_IsThirdPerson( ) };

    for ( i32 index{ 1 }; index <= max_clients; ++index )
    {
        IClientEntity *entity{ g_csgo.m_entity_list->GetClientEntity( index ) };
        if ( !entity || ( index == local_index && !thirdperson ) )
        {
            continue;
        }

        auto *player{ entity->As< C_CSPlayer * >( ) };
        if ( player->IsDormant( ) || !player->IsAlive( ) )
        {
            continue;
        }

        const studiohdr_t *studio{ g_csgo.m_model_info->GetStudiomodel( player->GetModel( ) ) };
        if ( !studio )
        {
            continue;
        }

        const mstudiohitboxset_t *set{ studio->pHitboxSet( player->m_nHitboxSet( ) ) };
        if ( !set )
        {
            continue;
        }

        if ( g_cfg.m_debug.m_server_hitboxes )
        {
            this->draw_server( index, set );
        }

        if ( g_cfg.m_debug.m_client_hitboxes )
        {
            this->draw_client( player, set );
        }

        if ( g_cfg.m_debug.m_lag_server )
        {
            this->draw_lag_check( index, studio, set );
        }
    }
}

void server_hitboxes::draw_server( i32 index, const mstudiohitboxset_t *set ) const
{
    if ( !g_csgo.m_player_info_manager )
    {
        return;
    }

    const CGlobalVars *globals{ g_csgo.m_player_info_manager->GetGlobalVars( ) };
    if ( !globals || !globals->pEdicts || index > globals->maxClients )
    {
        return;
    }

    const edict_t &edict{ globals->pEdicts[ index ] };
    if ( edict.m_fStateFlags & FL_EDICT_FREE || !edict.m_pUnk )
    {
        return;
    }

    CBaseAnimating *server_player{ edict.m_pUnk->GetBaseEntity( ) };
    if ( !server_player )
    {
        return;
    }

    alignas( 16 ) matrix3x4 bones[ MAXSTUDIOBONES ]{ };
    server_player->SetupBones( bones, BONE_USED_BY_HITBOX | BONE_USED_BY_ATTACHMENT );

    for ( i32 hitbox{ }; hitbox < set->numhitboxes; ++hitbox )
    {
        const mstudiobbox_t *box{ set->pHitbox( hitbox ) };
        draw_hitbox( bones[ box->bone ], box, g_cfg.m_debug.m_server_hitbox_color );
    }
}

void server_hitboxes::draw_client( C_CSPlayer *player, const mstudiohitboxset_t *set ) const
{
    alignas( 16 ) matrix3x4 bones[ MAXSTUDIOBONES ]{ };
    if ( !player->SetupBones( bones, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, player->m_flSimulationTime( ) ) )
    {
        return;
    }

    for ( i32 hitbox{ }; hitbox < set->numhitboxes; ++hitbox )
    {
        const mstudiobbox_t *box{ set->pHitbox( hitbox ) };
        draw_hitbox( bones[ box->bone ], box, g_cfg.m_debug.m_client_hitbox_color );
    }
}

void server_hitboxes::draw_lag_check( i32 index, const studiohdr_t *studio, const mstudiohitboxset_t *set ) const
{
    const lag_check_t &check{ this->m_checks[ index ] };
    if ( check.m_header != studio )
    {
        return;
    }

    for ( i32 hitbox{ }; hitbox < set->numhitboxes; ++hitbox )
    {
        const mstudiobbox_t *box{ set->pHitbox( hitbox ) };
        draw_hitbox( check.m_server[ box->bone ], box, g_cfg.m_debug.m_server_hitbox_color );
        draw_hitbox( check.m_ours[ box->bone ], box, g_cfg.m_debug.m_lag_newest_color );
    }

    vec2 at{ };
    if ( !g_cl.world_to_screen( check.m_origin, at ) )
    {
        return;
    }

    char label[ 96 ]{ };
    atl::format( label, sizeof( label ), "t %d %s land %d origin %.3f hitbox %.3f", check.m_tick, check.m_accepted ? "reachable" : "rejected",
                 check.m_landed, check.m_origin_error, check.m_hitbox_error );
    g_draw.text( ATLAS_XORSTR( "ESP Small" ).get( ), point_t{ static_cast< i32 >( at.x ), static_cast< i32 >( at.y ) },
                 g_cfg.m_debug.m_server_hitbox_color, label, true, render::text_align::center );
}

ATLAS_FORCE_INLINE IServerUnknown *server_unknown( const CGlobalVars *globals, i32 index )
{
    const edict_t &edict{ globals->pEdicts[ index ] };
    return edict.m_fStateFlags & FL_EDICT_FREE ? nullptr : edict.m_pUnk;
}

ATLAS_FORCE_INLINE const LagRecordNode *server_records( u32 handle )
{
    const LagCompensatedEntityNode *nodes{ g_csgo.m_lag_compensation->nodes( ) };

    u16 pending[ 128 ]{ };
    i32 count{ };
    pending[ count++ ] = g_csgo.m_lag_compensation->root( );

    while ( count > 0 )
    {
        const u16 at{ pending[ --count ] };
        if ( at == 0xFFFF )
        {
            continue;
        }

        const LagCompensatedEntityNode &node{ nodes[ at ] };
        if ( node.m_handle == handle )
        {
            return node.m_entity ? node.m_entity->m_head : nullptr;
        }

        if ( count + 2 <= static_cast< i32 >( sizeof( pending ) / sizeof( pending[ 0 ] ) ) )
        {
            pending[ count++ ] = node.m_left;
            pending[ count++ ] = node.m_right;
        }
    }

    return nullptr;
}

void server_hitboxes::pair_record( i32 index, const lag_record_t &entry, const LagRecordNode *head )
{
    for ( const LagRecordNode *node{ head }; node; node = node->m_next )
    {
        const LagRecord &record{ node->m_record };
        if ( TIME_TO_TICKS( record.m_flSimulationTime ) != entry.m_tick )
        {
            continue;
        }

        DBGOUT( "LCLAB P idx=%d t=%d newest=%d origin=%.4f mins=%.4f maxs=%.4f", index, entry.m_tick,
                TIME_TO_TICKS( head->m_record.m_flSimulationTime ), ( record.m_vecOrigin - entry.m_origin ).length( ),
                ( record.m_vecMinsPreScaled - entry.m_mins ).length( ), ( record.m_vecMaxsPreScaled - entry.m_maxs ).length( ) );
        return;
    }

    DBGOUT( "LCLAB P idx=%d t=%d newest=%d missing", index, entry.m_tick, TIME_TO_TICKS( head->m_record.m_flSimulationTime ) );
}

ATLAS_FORCE_INLINE float worst_hitbox( const mstudiohitboxset_t *set, const matrix3x4 *server, const matrix3x4 *ours, const vec3 &shift,
                                       i32 &worst )
{
    float error{ };
    for ( i32 hitbox{ }; set && hitbox < set->numhitboxes; ++hitbox )
    {
        const mstudiobbox_t *box{ set->pHitbox( hitbox ) };
        const vec3           middle{ ( box->bbmin + box->bbmax ) * 0.5f };
        const vec3           theirs{ math::transform_point( middle, server[ box->bone ] ) };
        const vec3           mine{ math::transform_point( middle, ours[ box->bone ] ) + shift };
        const float          distance{ ( theirs - mine ).length( ) };
        if ( distance > error )
        {
            error = distance;
            worst = hitbox;
        }
    }

    return error;
}

void server_hitboxes::rewind( i32 index, i32 age, CGlobalVars *globals, CBaseAnimating *shooter, CBaseAnimating *target, const LagRecordNode *head )
{
    const lag_record_t   &entry{ *g_lagcomp.record( index, age ) };
    const rewind_window_t window{ lag_compensation::rewind_window( g_cl.m_local->m_nTickBase( ) ) };
    const vec3            center{ entry.m_origin + ( entry.m_mins + entry.m_maxs ) * 0.5f };

    CUserCmd command{ };
    command.tick_count = window.command_tick( entry.m_tick );
    command.buttons    = IN_ATTACK;
    math::vector_angles( center - ( g_cl.m_local->GetAbsOrigin( ) + g_cl.m_local->m_vecViewOffset( ) ), command.viewangles );

    CUserCmd *const held_command{ shooter->m_pCurrentCommand( ) };
    const float     held_curtime{ globals->curtime };
    shooter->m_pCurrentCommand( ) = &command;
    globals->curtime              = TICKS_TO_TIME( window.m_tickbase );

    lag_check_t &check{ this->m_checks[ index ] };
    g_csgo.m_lag_compensation->StartLagCompensation( shooter, LAG_COMPENSATE_HITBOXES_ALONG_RAY, vec3{ }, command.viewangles, 0.0f );
    target->SetupBones( check.m_server, BONE_USED_BY_HITBOX | BONE_USED_BY_ATTACHMENT );
    const vec3 rewound{ target->m_vecAbsOrigin( ) };
    g_csgo.m_lag_compensation->FinishLagCompensation( shooter );

    globals->curtime              = held_curtime;
    shooter->m_pCurrentCommand( ) = held_command;

    target->InvalidateBoneCache( );

    atl::memcpy( check.m_ours, entry.m_bones, sizeof( matrix3x4 ) * static_cast< usize >( entry.m_bone_count ) );
    check.m_header       = entry.m_header;
    check.m_origin       = entry.m_origin;
    check.m_tick         = entry.m_tick;
    check.m_accepted     = g_lagcomp.reachable( index, entry, window );
    check.m_origin_error = ( rewound - entry.m_origin ).length( );
    check.m_landed       = -1;

    for ( const LagRecordNode *node{ head }; node; node = node->m_next )
    {
        if ( node->m_record.m_vecOrigin == rewound )
        {
            check.m_landed = TIME_TO_TICKS( node->m_record.m_flSimulationTime );
            break;
        }
    }

    IClientEntity            *client_entity{ g_csgo.m_entity_list->GetClientEntity( index ) };
    const mstudiohitboxset_t *set{ entry.m_header->pHitboxSet( client_entity->As< C_CSPlayer * >( )->m_nHitboxSet( ) ) };

    i32 worst{ -1 };
    i32 worst_shifted{ -1 };
    i32 worst_predicted{ -1 };

    check.m_hitbox_error = worst_hitbox( set, check.m_server, check.m_ours, vec3{ }, worst );

    const vec3  expected{ g_lagcomp.rewound_origin( index, age, window ) };
    const float shifted_error{ worst_hitbox( set, check.m_server, check.m_ours, rewound - entry.m_origin, worst_shifted ) };
    const float predicted_error{ worst_hitbox( set, check.m_server, check.m_ours, expected - entry.m_origin, worst_predicted ) };

    DBGOUT( "LCLAB K idx=%d t=%d age=%d acc=%d land=%d tc=%d tb=%d origin=%.4f slide=%.4f hitbox=%.4f box=%d shifted=%.4f sbox=%d "
            "predicted=%.4f pbox=%d",
            index, entry.m_tick, window.m_tickbase - entry.m_tick, check.m_accepted ? 1 : 0, check.m_landed, command.tick_count,
            window.m_tickbase, check.m_origin_error, ( rewound - expected ).length( ), check.m_hitbox_error, worst, shifted_error,
            worst_shifted, predicted_error, worst_predicted );
}

void server_hitboxes::check_lag_records( )
{
    constexpr i32 CHECK_AGES{ 40 };

    if ( !g_cfg.m_debug.m_lag_server || !g_csgo.m_lag_compensation || !g_csgo.m_player_info_manager || !g_cl.m_local )
    {
        return;
    }

    CGlobalVars *globals{ g_csgo.m_player_info_manager->GetGlobalVars( ) };
    const i32    local_index{ g_csgo.m_engine->GetLocalPlayer( ) };
    if ( !globals || !globals->pEdicts || local_index > globals->maxClients )
    {
        return;
    }

    IServerUnknown *shooter{ server_unknown( globals, local_index ) };
    if ( !shooter )
    {
        return;
    }

    this->m_check_age = ( this->m_check_age + 11 ) % CHECK_AGES;

    const i32 tick{ g_cl.m_local->m_nTickBase( ) - this->m_check_age };
    const i32 highest{ math::smaller( globals->maxClients, MAX_TRACKED_PLAYERS - 1 ) };
    for ( i32 index{ 1 }; index <= highest; ++index )
    {
        const lag_record_t *newest{ g_lagcomp.record( index, 0 ) };
        IServerUnknown     *target{ server_unknown( globals, index ) };
        if ( !newest || !target )
        {
            continue;
        }

        const LagRecordNode *head{ server_records( target->GetRefEHandle( ).ToInt( ) ) };
        if ( !head )
        {
            continue;
        }

        if ( this->m_paired[ index ] != newest->m_tick )
        {
            this->m_paired[ index ] = newest->m_tick;
            this->pair_record( index, *newest, head );
        }

        for ( i32 age{ }; age < g_lagcomp.count( index ); ++age )
        {
            const lag_record_t *entry{ g_lagcomp.record( index, age ) };
            if ( entry->m_tick > tick )
            {
                continue;
            }

            if ( entry->m_bone_count > 0 )
            {
                this->rewind( index, age, globals, shooter->GetBaseEntity( ), target->GetBaseEntity( ), head );
            }
            break;
        }
    }
}
