#include "global.hpp"

ClientClass *CHLClient::GetAllClasses( )
{
    return util::call_virtual< ClientClass *( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_chl_client_get_all_classes )( this );
}

IClientEntity *CBaseHandle::Get( ) const
{
    return g_csgo.m_entity_list->GetClientEntityFromHandle( *this );
}

IClientEntity *CClientEntityList::GetClientEntity( int num )
{
    return util::call_virtual< IClientEntity *( ATLAS_THISCALL * )( void *, int ) >(
        this, g_csgo.m_client_entity_list_get_client_entity )( this, num );
}

IClientEntity *CClientEntityList::GetClientEntityFromHandle( CBaseHandle num )
{
    return util::call_virtual< IClientEntity *( ATLAS_THISCALL * )( void *, CBaseHandle ) >(
        this, g_csgo.m_client_entity_list_get_client_entity_from_handle )( this, num );
}

void CMatSystemSurface::UnlockCursor( )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_mat_system_surface_unlock_cursor )( this );
}

void CMatSystemSurface::PlaySound( const char *file_name )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, const char * ) >(
        this, g_csgo.m_mat_system_surface_play_sound )( this, file_name );
}

bool CEngineClient::GetPlayerInfo( i32 index, player_info_t *info )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void *, i32, player_info_t * ) >(
        this, g_csgo.m_engine_client_get_player_info )( this, index, info );
}

i32 CEngineClient::GetPlayerForUserID( i32 user_id )
{
    return util::call_virtual< i32( ATLAS_THISCALL * )( void *, i32 ) >(
        this, g_csgo.m_engine_client_get_player_for_user_id )( this, user_id );
}

i32 CEngineClient::GetLocalPlayer( )
{
    return util::call_virtual< i32( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_engine_client_get_local_player )( this );
}

i32 CEngineClient::GetMaxClients( )
{
    return util::call_virtual< i32( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_engine_client_get_max_clients )( this );
}

void CEngineClient::GetViewAngles( vec3 &angles )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, vec3 & ) >(
        this, g_csgo.m_engine_client_get_view_angles )( this, angles );
}

INetChannelInfo *CEngineClient::GetNetChannelInfo( )
{
    return util::call_virtual< INetChannelInfo *( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_engine_client_get_net_channel_info )( this );
}

bool CGameTrace::DidHitNonWorldEntity( ) const
{
    return this->m_pEnt && this->m_pEnt != g_csgo.m_entity_list->GetClientEntity( 0 );
}

CTraceFilterSkipTwoEntities::CTraceFilterSkipTwoEntities( const IHandleEntity *pass, const IHandleEntity *pass2 ) :
    m_vmt{ g_csgo.m_CTraceFilterSkipTwoEntities_vmt },
    m_pPassEnt{ pass },
    m_pPassEnt2{ pass2 }
{
}

bool IsBreakableEntity( C_BaseEntity *entity )
{
    return g_csgo.m_IsBreakableEntity.get_as< bool( ATLAS_THISCALL * )( C_BaseEntity * ) >( )( entity );
}

i32 CEngineTrace::GetPointContents( const vec3 &position, u32 mask, IHandleEntity **entity )
{
    return util::call_virtual< i32( ATLAS_THISCALL * )( void *, const vec3 &, u32, IHandleEntity ** ) >(
        this, g_csgo.m_engine_trace_get_point_contents )( this, position, mask, entity );
}

void CEngineTrace::ClipRayToEntity( const Ray_t &ray, u32 mask, IHandleEntity *entity, trace_t *trace )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, const Ray_t &, u32, IHandleEntity *, trace_t * ) >(
        this, g_csgo.m_engine_trace_clip_ray_to_entity )( this, ray, mask, entity, trace );
}

void CEngineTrace::TraceRay( const Ray_t &ray, u32 mask, ITraceFilter *filter, trace_t *trace )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, const Ray_t &, u32, ITraceFilter *, trace_t * ) >(
        this, g_csgo.m_engine_trace_trace_ray )( this, ray, mask, filter, trace );
}

const surfacedata_t *IPhysicsSurfaceProps::GetSurfaceData( i32 surface_data_index )
{
    return util::call_virtual< const surfacedata_t *( ATLAS_THISCALL * )( void *, i32 ) >(
        this, g_csgo.m_physics_surface_props_get_surface_data )( this, surface_data_index );
}

float INetChannelInfo::GetLatency( i32 flow )
{
    return util::call_virtual< float( ATLAS_THISCALL * )( void *, i32 ) >(
        this, g_csgo.m_net_channel_get_latency )( this, flow );
}

bool CEngineClient::IsInGame( )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_engine_client_is_in_game )( this );
}

const VMatrix &CEngineClient::WorldToScreenMatrix( )
{
    return util::call_virtual< const VMatrix &( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_engine_client_world_to_screen_matrix )( this );
}

CRC32_t CUserCmd::GetChecksum( ) const
{
    return g_csgo.m_CUserCmd_GetChecksum.get_as< CRC32_t( ATLAS_THISCALL * )( const CUserCmd * ) >( )( this );
}

bool CInput::CAM_IsThirdPerson( )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void *, i32 ) >(
        this, g_csgo.m_input_cam_is_third_person )( this, -1 );
}

void CInput::CAM_ToThirdPerson( )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_input_cam_to_third_person )( this );
}

void CInput::CAM_ToFirstPerson( )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_input_cam_to_first_person )( this );
}

CUserCmd *CInput::GetUserCmd( i32 sequence_number )
{
    CUserCmd *commands{ *reinterpret_cast< CUserCmd ** >( reinterpret_cast< u8 * >( this ) + 0xEC ) };
    return &commands[ sequence_number % MULTIPLAYER_BACKUP ];
}

CVerifiedUserCmd *CInput::GetVerifiedUserCmd( i32 sequence_number )
{
    CVerifiedUserCmd *commands{ *reinterpret_cast< CVerifiedUserCmd ** >( reinterpret_cast< u8 * >( this ) + 0xF0 ) };
    return &commands[ sequence_number % MULTIPLAYER_BACKUP ];
}

float ConVar::GetFloat( ) const
{
    const u8 *parent{ *reinterpret_cast< const u8 *const * >( reinterpret_cast< const u8 * >( this ) + 0x1C ) };
    const u32 bits{ *reinterpret_cast< const u32 * >( parent + 0x2C ) ^ static_cast< u32 >( reinterpret_cast< uptr >( parent ) ) };

    float value{ };
    atl::memcpy( &value, &bits, sizeof( value ) );
    return value;
}

i32 ConVar::GetInt( ) const
{
    const u8 *parent{ *reinterpret_cast< const u8 *const * >( reinterpret_cast< const u8 * >( this ) + 0x1C ) };
    return static_cast< i32 >( *reinterpret_cast< const u32 * >( parent + 0x30 ) ^ static_cast< u32 >( reinterpret_cast< uptr >( parent ) ) );
}

void ConVar::SetValue( i32 value )
{
    void *parent{ *reinterpret_cast< void ** >( reinterpret_cast< u8 * >( this ) + 0x1C ) };
    util::call_virtual< void( ATLAS_THISCALL * )( void *, i32 ) >(
        parent, g_csgo.m_convar_internal_set_int_value )( parent, value );
}

ConVar *ICvar::FindVar( const char *name )
{
    return util::call_virtual< ConVar *( ATLAS_THISCALL * )( void *, const char * ) >(
        this, g_csgo.m_cvar_find_var )( this, name );
}

int CPredictionCopy::TransferData( const char *operation, int entindex, datamap_t *dmap )
{
    return g_csgo.m_CPredictionCopy_TransferData.get_as< int( ATLAS_THISCALL * )( void *, const char *, int, datamap_t * ) >( )(
        this, operation, entindex, dmap );
}

void *IMemAlloc::Alloc( usize size )
{
    return util::call_virtual< void *( ATLAS_THISCALL * )( void *, usize ) >(
        this, g_csgo.m_mem_alloc_alloc )( this, size );
}

void *IMemAlloc::Realloc( void *mem, usize size )
{
    return util::call_virtual< void *( ATLAS_THISCALL * )( void *, void *, usize ) >(
        this, g_csgo.m_mem_alloc_realloc )( this, mem, size );
}

void IMemAlloc::Free( void *mem )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, void * ) >(
        this, g_csgo.m_mem_alloc_free )( this, mem );
}

void CMoveHelperClient::SetHost( C_BasePlayer *host )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, C_BasePlayer * ) >(
        this, g_csgo.m_move_helper_client_set_host )( this, host );
}

void CPrediction::GetLocalViewAngles( vec3 &angles )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, vec3 & ) >(
        this, g_csgo.m_prediction_get_local_view_angles )( this, angles );
}

void CPrediction::SetLocalViewAngles( vec3 &angles )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, vec3 & ) >(
        this, g_csgo.m_prediction_set_local_view_angles )( this, angles );
}

void CPrediction::RunCommand( C_BasePlayer *player, CUserCmd *cmd, CMoveHelperClient *move_helper )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, C_BasePlayer *, CUserCmd *, CMoveHelperClient * ) >(
        this, g_csgo.m_prediction_run_command )( this, player, cmd, move_helper );
}

i32 CStudioHdr::GetNumSeq( ) const
{
    return this->m_pVModel ? *reinterpret_cast< const i32 * >( this->m_pVModel + 0x14 ) : this->m_pStudioHdr->numlocalseq;
}

const mstudioseqdesc_t &CStudioHdr::pSeqdesc( i32 sequence )
{
    return *g_csgo.m_CStudioHdr_pSeqdesc.get_as< const mstudioseqdesc_t *( ATLAS_THISCALL * )( CStudioHdr *, i32 ) >( )( this, sequence );
}

i32 CStudioHdr::GetSequenceActivity( i32 sequence )
{
    return g_csgo.m_GetSequenceActivity.get_as< i32( ATLAS_FASTCALL * )( CStudioHdr *, i32 ) >( )( this, sequence );
}

#if defined( ATLAS_DEV )
const studiohdr_t *CModelInfoClient::GetStudiomodel( const model_t *model )
{
    return util::call_virtual< const studiohdr_t *( ATLAS_THISCALL * )( void *, const model_t * ) >( this, 30 )( this, model );
}

void CBaseAnimating::SetupBones( matrix3x4 *bone_to_world, i32 bone_mask )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, matrix3x4 *, i32 ) >( this, g_csgo.m_base_animating_setup_bones )( this, bone_to_world, bone_mask );
}

void CBaseAnimating::InvalidateBoneCache( )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void * ) >( this, 221 )( this );
}

void ILagCompensationManager::StartLagCompensation( CBaseAnimating *player, i32 type, const vec3 &weapon_pos, const vec3 &weapon_angles, float weapon_range )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, CBaseAnimating *, i32, const vec3 &, const vec3 &, float ) >( this, 0 )(
        this, player, type, weapon_pos, weapon_angles, weapon_range );
}

void ILagCompensationManager::FinishLagCompensation( CBaseAnimating *player )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, CBaseAnimating * ) >( this, 1 )( this, player );
}

CGlobalVars *IPlayerInfoManager::GetGlobalVars( )
{
    return util::call_virtual< CGlobalVars *( ATLAS_THISCALL * )( void * ) >( this, 1 )( this );
}
#endif // ATLAS_DEV

void ICallQueue::QueueFunctor( CFunctor *functor )
{
    functor->AddRef( );
    util::call_virtual< void( ATLAS_THISCALL * )( void *, CFunctor * ) >(
        this, g_csgo.m_call_queue_queue_functor_internal )( this, functor );
}

void IMatRenderContext::Release( )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_render_context_release )( this );
}

ICallQueue *IMatRenderContext::GetCallQueue( )
{
    return util::call_virtual< ICallQueue *( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_render_context_get_call_queue )( this );
}

void IMatRenderContext::GetMatrix( i32 mode, VMatrix *matrix )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, i32, VMatrix * ) >(
        this, g_csgo.m_render_context_get_matrix )( this, mode, matrix );
}

IMatRenderContext *CMaterialSystem::GetRenderContext( )
{
    return util::call_virtual< IMatRenderContext *( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_material_system_get_render_context )( this );
}

void CMeshDX8::HandleLateCreation( )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_mesh_handle_late_creation )( this );
}

const studiohdr_t *IMDLCache::GetStudioHdr( MDLHandle_t handle )
{
    return util::call_virtual< const studiohdr_t *( ATLAS_THISCALL * )( void *, MDLHandle_t ) >(
        this, g_csgo.m_mdl_cache_get_studio_hdr )( this, handle );
}

const studiohwdata_t *IMDLCache::GetHardwareData( MDLHandle_t handle )
{
    return util::call_virtual< const studiohwdata_t *( ATLAS_THISCALL * )( void *, MDLHandle_t ) >(
        this, g_csgo.m_mdl_cache_get_hardware_data )( this, handle );
}

void CHLClient::CreateMove( i32 sequence_number, float input_sample_frametime, bool active )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, i32, float, bool ) >(
        this, g_csgo.m_chl_client_create_move )( this, sequence_number, input_sample_frametime, active );
}

bool CHLClient::WriteUsercmdDeltaToBuffer( i32 slot, bf_write *buffer, i32 from, i32 to, bool is_new_command )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void *, i32, bf_write *, i32, i32, bool ) >(
        this, g_csgo.m_chl_client_write_usercmd_delta_to_buffer )( this, slot, buffer, from, to, is_new_command );
}

bool CNetChannel::IsLoopback( )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_net_channel_is_loopback )( this );
}

bool CNetChannel::IsTimingOut( )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_net_channel_is_timing_out )( this );
}

bool CNetChannel::SendNetMsg( INetMessage &msg, bool force_reliable, bool voice )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void *, INetMessage &, bool, bool ) >(
        this, g_csgo.m_net_channel_send_net_msg )( this, msg, force_reliable, voice );
}

void CNetChannel::SetChoked( )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_net_channel_set_choked )( this );
}

i32 CNetChannel::SendDatagram( bf_write *data )
{
    return util::call_virtual< i32( ATLAS_THISCALL * )( void *, bf_write * ) >(
        this, g_csgo.m_net_channel_send_datagram )( this, data );
}

bool CNetChannel::CanPacket( )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_net_channel_can_packet )( this );
}

bool CClientState::IsPaused( )
{
    return g_csgo.m_CClientState_IsPaused.get_as< bool( ATLAS_THISCALL * )( void * ) >( )( reinterpret_cast< u8 * >( this ) + 8 );
}

void CClientState::ForceFullUpdate( const char *reason )
{
    if ( this->m_nDeltaTick == -1 )
    {
        return;
    }

    g_csgo.m_CBaseClientState_FreeEntityBaselines.get_as< void( ATLAS_THISCALL * )( void * ) >( )( reinterpret_cast< u8 * >( this ) + 8 );
    this->m_nDeltaTick = -1;
    g_csgo.m_Tier0_DevMsg_fn.call_variadic< void >( ATLAS_XORSTR( "Requesting full game update (%s)...\n" ).get( ), reason );
}

bool CDemoPlayer::IsPlayingBack( )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_demo_player_is_playing_back )( this );
}

void CDemoRecorder::RecordUserInput( i32 command )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, i32 ) >(
        this, g_csgo.m_demo_recorder_record_user_input )( this, command );
}

bool CDemoRecorder::IsRecording( )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_demo_recorder_is_recording )( this );
}

const char *IGameEvent::GetName( )
{
    return util::call_virtual< const char *( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_game_event_get_name )( this );
}

i32 IGameEvent::GetInt( const char *name, i32 default_value )
{
    return util::call_virtual< i32( ATLAS_THISCALL * )( void *, const char *, i32 ) >(
        this, g_csgo.m_game_event_get_int )( this, name, default_value );
}

float IGameEvent::GetFloat( const char *name, float default_value )
{
    return util::call_virtual< float( ATLAS_THISCALL * )( void *, const char *, float ) >(
        this, g_csgo.m_game_event_get_float )( this, name, default_value );
}

const char *IGameEvent::GetString( const char *name, const char *default_value )
{
    return util::call_virtual< const char *( ATLAS_THISCALL * )( void *, const char *, const char * ) >(
        this, g_csgo.m_game_event_get_string )( this, name, default_value );
}

void IGameEvent::SetString( const char *name, const char *value )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, const char *, const char * ) >(
        this, g_csgo.m_game_event_set_string )( this, name, value );
}

bool CGameEventManager::AddListener( IGameEventListener2 *listener, const char *name, bool server_side )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void *, IGameEventListener2 *, const char *, bool ) >(
        this, g_csgo.m_game_event_manager_add_listener )( this, listener, name, server_side );
}

void CUniformRandomStream::Construct( )
{
    g_csgo.m_CUniformRandomStream_ctor.get_as< void( ATLAS_THISCALL * )( CUniformRandomStream * ) >( )( this );
}

void CUniformRandomStream::SetSeed( i32 seed )
{
    g_csgo.m_CUniformRandomStream_SetSeed.get_as< void( ATLAS_THISCALL * )( CUniformRandomStream *, i32 ) >( )( this, seed );
}

float CUniformRandomStream::RandomFloat( float minimum, float maximum )
{
    return g_csgo.m_CUniformRandomStream_RandomFloat.get_as< float( ATLAS_THISCALL * )( CUniformRandomStream *, float, float ) >( )(
        this, minimum, maximum );
}

i32 CUniformRandomStream::RandomInt( i32 minimum, i32 maximum )
{
    return g_csgo.m_CUniformRandomStream_RandomInt.get_as< i32( ATLAS_THISCALL * )( CUniformRandomStream *, i32, i32 ) >( )(
        this, minimum, maximum );
}

i32 CModelInfoClient::GetModelIndex( const char *name )
{
    return util::call_virtual< i32( ATLAS_THISCALL * )( void *, const char * ) >(
        this, g_csgo.m_model_info_get_model_index )( this, name );
}

Beam_t *CViewRenderBeams::CreateBeamPoints( BeamInfo_t &info )
{
    return util::call_virtual< Beam_t *( ATLAS_THISCALL * )( void *, BeamInfo_t * ) >(
        this, g_csgo.m_view_render_beams_create_beam_points )( this, &info );
}

void *IFileSystem::base( )
{
    return reinterpret_cast< u8 * >( this ) + sizeof( void * );
}

FileHandle_t IFileSystem::Open( const char *file_name, const char *options, const char *path_id )
{
    void *self{ this->base( ) };

    return util::call_virtual< FileHandle_t( ATLAS_THISCALL * )( void *, const char *, const char *, const char * ) >(
        self, g_csgo.m_filesystem_open )( self, file_name, options, path_id );
}

u32 IFileSystem::Size( FileHandle_t file )
{
    void *self{ this->base( ) };

    return util::call_virtual< u32( ATLAS_THISCALL * )( void *, FileHandle_t ) >(
        self, g_csgo.m_filesystem_size )( self, file );
}

i32 IFileSystem::Read( void *destination, i32 size, FileHandle_t file )
{
    void *self{ this->base( ) };

    return util::call_virtual< i32( ATLAS_THISCALL * )( void *, void *, i32, FileHandle_t ) >(
        self, g_csgo.m_filesystem_read )( self, destination, size, file );
}

void IFileSystem::Close( FileHandle_t file )
{
    void *self{ this->base( ) };

    util::call_virtual< void( ATLAS_THISCALL * )( void *, FileHandle_t ) >(
        self, g_csgo.m_filesystem_close )( self, file );
}
