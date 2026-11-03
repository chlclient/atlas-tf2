#include "sdk.hpp"

#include "global.hpp"

EL4D2ClassID ClientClass::GetClassID( )
{
    return static_cast< EL4D2ClassID >( m_ClassID );
}

ClientClass *CHLClient::GetAllClasses( )
{
    return util::call_virtual< ClientClass *( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_chl_client_get_all_classes )( this );
}

void CHLClient::CreateMove( i32 sequence_number, float input_sample_frametime, bool active )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, i32, float, bool ) >(
        this, g_l4d2.m_chl_client_create_move )( this, sequence_number, input_sample_frametime, active );
}

bool CHLClient::WriteUsercmdDeltaToBuffer( i32 slot, bf_write *buffer, i32 from, i32 to, bool is_new_command )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void *, i32, bf_write *, i32, i32, bool ) >(
        this, g_l4d2.m_chl_client_write_usercmd_delta_to_buffer )( this, slot, buffer, from, to, is_new_command );
}

IClientEntity *CClientEntityList::GetClientEntity( int num )
{
    return util::call_virtual< IClientEntity *( ATLAS_THISCALL * )( void *, int ) >(
        this, g_l4d2.m_client_entity_list_get_client_entity )( this, num );
}

IClientEntity *CClientEntityList::GetClientEntityFromHandle( CBaseHandle num )
{
    return util::call_virtual< IClientEntity *( ATLAS_THISCALL * )( void *, CBaseHandle ) >(
        this, g_l4d2.m_client_entity_list_get_client_entity_from_handle )( this, num );
}

int CClientEntityList::GetHighestEntityIndex( )
{
    return util::call_virtual< int( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_client_entity_list_get_highest_entity_index )( this );
}

CBaseHandle::CBaseHandle( const CBaseHandle &other )
{
    m_Index = other.m_Index;
}

CBaseHandle::CBaseHandle( unsigned long value )
{
    m_Index = value;
}

CBaseHandle::CBaseHandle( int iEntry, int iSerialNumber )
{
    Init( iEntry, iSerialNumber );
}

void CBaseHandle::Init( int iEntry, int iSerialNumber )
{
    m_Index = ( static_cast< u32 >( iEntry ) & ENT_ENTRY_MASK ) |
              ( static_cast< u32 >( iSerialNumber ) << NUM_SERIAL_NUM_SHIFT_BITS );
}

void CBaseHandle::Term( )
{
    m_Index = INVALID_EHANDLE_INDEX;
}

bool CBaseHandle::IsValid( ) const
{
    return m_Index != INVALID_EHANDLE_INDEX;
}

int CBaseHandle::GetEntryIndex( ) const
{
    return IsValid( ) ? static_cast< int >( m_Index & ENT_ENTRY_MASK ) : NUM_ENT_ENTRIES - 1;
}

int CBaseHandle::GetSerialNumber( ) const
{
    return static_cast< int >( m_Index >> NUM_SERIAL_NUM_SHIFT_BITS );
}

int CBaseHandle::ToInt( ) const
{
    return static_cast< int >( m_Index );
}

unsigned long CBaseHandle::GetIndex( ) const
{
    return m_Index;
}

bool CBaseHandle::operator==( const CBaseHandle &other ) const
{
    return m_Index == other.m_Index;
}

bool CBaseHandle::operator!=( const CBaseHandle &other ) const
{
    return m_Index != other.m_Index;
}

bool CBaseHandle::operator<( const CBaseHandle &other ) const
{
    return m_Index < other.m_Index;
}

bool CBaseHandle::operator==( const IHandleEntity *pEnt ) const
{
    return Get( ) == pEnt;
}

bool CBaseHandle::operator!=( const IHandleEntity *pEnt ) const
{
    return Get( ) != pEnt;
}

bool CBaseHandle::operator<( const IHandleEntity *pEnt ) const
{
    return reinterpret_cast< uptr >( Get( ) ) < reinterpret_cast< uptr >( pEnt );
}

const CBaseHandle &CBaseHandle::operator=( const IHandleEntity *pEntity )
{
    return Set( pEntity );
}

const CBaseHandle &CBaseHandle::Set( const IHandleEntity *entity )
{
    m_Index = entity ? entity->GetRefEHandle( ).GetIndex( ) : INVALID_EHANDLE_INDEX;
    return *this;
}

IHandleEntity *CBaseHandle::Get( ) const
{
    return g_l4d2.m_entity_list->GetClientEntityFromHandle( *this );
}

void CEngineClient::GetScreenSize( i32 &width, i32 &height )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, i32 &, i32 & ) >(
        this, g_l4d2.m_engine_client_get_screen_size )( this, width, height );
}

i32 CEngineClient::GetLocalPlayer( )
{
    return util::call_virtual< i32( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_engine_client_get_local_player )( this );
}

bool CEngineClient::IsInGame( )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_engine_client_is_in_game )( this );
}

const VMatrix &CEngineClient::WorldToScreenMatrix( )
{
    return util::call_virtual< const VMatrix &( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_engine_client_world_to_screen_matrix )( this );
}

i32 CEngineClient::GetActiveSplitScreenPlayerSlot( )
{
    return util::call_virtual< i32( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_engine_client_get_active_split_screen_player_slot )( this );
}

void CUniformRandomStream::SetSeed( int seed )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, int ) >( this, 0 )( this, seed );
}

float CUniformRandomStream::RandomFloat( float minimum, float maximum )
{
    return util::call_virtual< float( ATLAS_THISCALL * )( void *, float, float ) >( this, 1 )( this, minimum, maximum );
}

float ConVar::GetFloat( ) const
{
    return m_pParent->m_fValue;
}

int ConVar::GetInt( ) const
{
    return m_pParent->m_nValue;
}

void CMatSystemSurface::UnlockCursor( )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_mat_system_surface_unlock_cursor )( this );
}

ConVar *ICvar::FindVar( const char *var_name )
{
    return util::call_virtual< ConVar *( ATLAS_THISCALL * )( void *, const char * ) >(
        this, g_l4d2.m_cvar_find_var )( this, var_name );
}

CRC32_t CUserCmd::GetChecksum( ) const
{
    return g_l4d2.m_CUserCmd_GetChecksum.get_as< CRC32_t( ATLAS_THISCALL * )( const CUserCmd * ) >( )( this );
}

// each split screen player owns a 0xCC byte block at +0x34 with the command ring at +0xB8 and the verified
// ring at +0xBC..
CUserCmd *CInput::GetUserCmd( i32 slot, i32 sequence_number )
{
    u8       *per_user{ reinterpret_cast< u8 * >( this ) + 0x34 + slot * 0xCC };
    CUserCmd *commands{ *reinterpret_cast< CUserCmd ** >( per_user + 0xB8 ) };
    return &commands[ sequence_number % MULTIPLAYER_BACKUP ];
}

CVerifiedUserCmd *CInput::GetVerifiedUserCmd( i32 slot, i32 sequence_number )
{
    u8               *per_user{ reinterpret_cast< u8 * >( this ) + 0x34 + slot * 0xCC };
    CVerifiedUserCmd *verified{ *reinterpret_cast< CVerifiedUserCmd ** >( per_user + 0xBC ) };
    return &verified[ sequence_number % MULTIPLAYER_BACKUP ];
}

int CPredictionCopy::TransferData( const char *operation, int entindex, datamap_t *dmap )
{
    return g_l4d2.m_CPredictionCopy_TransferData.get_as< int( ATLAS_THISCALL * )( void *, const char *, int, datamap_t * ) >( )(
        this, operation, entindex, dmap );
}

void *IMemAlloc::Alloc( usize size )
{
    return util::call_virtual< void *( ATLAS_THISCALL * )( void *, usize ) >(
        this, g_l4d2.m_mem_alloc_alloc )( this, size );
}

void *IMemAlloc::Realloc( void *mem, usize size )
{
    return util::call_virtual< void *( ATLAS_THISCALL * )( void *, void *, usize ) >(
        this, g_l4d2.m_mem_alloc_realloc )( this, mem, size );
}

void IMemAlloc::Free( void *mem )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, void * ) >(
        this, g_l4d2.m_mem_alloc_free )( this, mem );
}

void CMoveHelperClient::SetHost( C_BasePlayer *host )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, C_BasePlayer * ) >(
        this, g_l4d2.m_move_helper_client_set_host )( this, host );
}

void CPrediction::GetLocalViewAngles( vec3 &angles )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, vec3 & ) >(
        this, g_l4d2.m_prediction_get_local_view_angles )( this, angles );
}

void CPrediction::SetLocalViewAngles( vec3 &angles )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, vec3 & ) >(
        this, g_l4d2.m_prediction_set_local_view_angles )( this, angles );
}

void CPrediction::RunCommand( C_BasePlayer *player, CUserCmd *cmd, CMoveHelperClient *move_helper )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, C_BasePlayer *, CUserCmd *, CMoveHelperClient * ) >(
        this, g_l4d2.m_prediction_run_command )( this, player, cmd, move_helper );
}

bool CNetChannel::IsLoopback( )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_net_channel_is_loopback )( this );
}

float CNetChannel::GetLatency( i32 flow )
{
    return util::call_virtual< float( ATLAS_THISCALL * )( void *, i32 ) >(
        this, g_l4d2.m_net_channel_get_latency )( this, flow );
}

bool CNetChannel::IsTimingOut( )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_net_channel_is_timing_out )( this );
}

bool CNetChannel::SendNetMsg( CNetMessage &msg, bool force_reliable, bool voice )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void *, CNetMessage &, bool, bool ) >(
        this, g_l4d2.m_net_channel_send_net_msg )( this, msg, force_reliable, voice );
}

void CNetChannel::SetChoked( )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_net_channel_set_choked )( this );
}

int CNetChannel::SendDatagram( bf_write *data )
{
    return util::call_virtual< int( ATLAS_THISCALL * )( void *, bf_write * ) >(
        this, g_l4d2.m_net_channel_send_datagram )( this, data );
}

bool CNetChannel::CanPacket( )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_net_channel_can_packet )( this );
}

bool CClientState::IsPaused( )
{
    return g_l4d2.m_CClientState_IsPaused.get_as< bool( ATLAS_THISCALL * )( void * ) >( )( this );
}

void CClientState::ForceFullUpdate( const char *reason )
{
    g_l4d2.m_CClientState_ForceFullUpdate.get_as< void( ATLAS_THISCALL * )( void *, const char * ) >( )( this, reason );
}

int CSplitScreen::SetActiveSplitScreenPlayerSlot( int slot )
{
    return util::call_virtual< int( ATLAS_THISCALL * )( void *, int ) >(
        this, g_l4d2.m_split_screen_set_active_split_screen_player_slot )( this, slot );
}

int CSplitScreen::FirstValidSplitScreenSlot( )
{
    return util::call_virtual< int( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_split_screen_first_valid_split_screen_slot )( this );
}

int CSplitScreen::NextValidSplitScreenSlot( int slot )
{
    return util::call_virtual< int( ATLAS_THISCALL * )( void *, int ) >(
        this, g_l4d2.m_split_screen_next_valid_split_screen_slot )( this, slot );
}

bool CSplitScreen::IsDisconnecting( int slot )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void *, int ) >(
        this, g_l4d2.m_split_screen_is_disconnecting )( this, slot );
}

bool CSplitScreen::SetLocalPlayerIsResolvable( const char *file, int line, bool resolvable )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void *, const char *, int, bool ) >(
        this, g_l4d2.m_split_screen_set_local_player_is_resolvable )( this, file, line, resolvable );
}

bool IDemoPlayer::IsPlayingBack( )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_demo_player_is_playing_back )( this );
}

bool IDemoRecorder::IsRecording( )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_demo_recorder_is_recording )( this );
}

void IDemoRecorder::RecordUserInput( int command_number )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, int ) >(
        this, g_l4d2.m_demo_recorder_record_user_input )( this, command_number );
}

const model_t *IClientRenderable::GetModel( )
{
    return util::call_virtual< const model_t *( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_client_renderable_get_model )( this );
}

i32 IClientRenderable::DrawModel( i32 flags, const RenderableInstance_t &instance )
{
    return util::call_virtual< i32( ATLAS_THISCALL * )( void *, i32, const RenderableInstance_t & ) >(
        this, g_l4d2.m_client_renderable_draw_model )( this, flags, instance );
}

bool IClientRenderable::SetupBones( matrix3x4 *bone_to_world, i32 max_bones, i32 bone_mask, float current_time )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void *, matrix3x4 *, i32, i32, float ) >(
        this, g_l4d2.m_client_renderable_setup_bones )( this, bone_to_world, max_bones, bone_mask, current_time );
}

studiohdr_t *CModelInfo::GetStudiomodel( const model_t *model )
{
    return util::call_virtual< studiohdr_t *( ATLAS_THISCALL * )( void *, const model_t * ) >(
        this, g_l4d2.m_model_info_get_studiomodel )( this, model );
}

void CEngineTrace::TraceRay( const Ray_t &ray, u32 mask, ITraceFilter *filter, CGameTrace *trace )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, const Ray_t &, u32, ITraceFilter *, CGameTrace * ) >(
        this, g_l4d2.m_engine_trace_trace_ray )( this, ray, mask, filter, trace );
}

void IMaterial::IncrementReferenceCount( )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_material_increment_reference_count )( this );
}

void *IKeyValuesSystem::AllocKeyValuesMemory( i32 size )
{
    return util::call_virtual< void *( ATLAS_THISCALL * )( void *, i32 ) >(
        this, g_l4d2.m_key_values_system_alloc_key_values_memory )( this, size );
}

KeyValues::KeyValues( const char *name )
{
    g_l4d2.m_KeyValues_ctor.get_as< void( ATLAS_THISCALL * )( void *, const char *, IKeyValuesSystem *, bool ) >( )(
        this, name, nullptr, false );
}

void KeyValues::SetString( const char *key, const char *value )
{
    g_l4d2.m_KeyValues_SetString.get_as< void( ATLAS_THISCALL * )( void *, const char *, const char * ) >( )( this, key, value );
}

void KeyValues::SetInt( const char *key, i32 value )
{
    g_l4d2.m_KeyValues_SetInt.get_as< void( ATLAS_THISCALL * )( void *, const char *, i32 ) >( )( this, key, value );
}

void *KeyValues::operator new( usize size )
{
    return g_l4d2.m_kv_system->AllocKeyValuesMemory( static_cast< i32 >( size ) );
}

void IMatRenderContext::Release( )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_render_context_release )( this );
}

void IMatRenderContext::BeginRender( )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_render_context_begin_render )( this );
}

void IMatRenderContext::EndRender( )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_render_context_end_render )( this );
}

void IMatRenderContext::SetRenderTarget( ITexture *texture )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, ITexture * ) >(
        this, g_l4d2.m_render_context_set_render_target )( this, texture );
}

void IMatRenderContext::DepthRange( float z_near, float z_far )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, float, float ) >(
        this, g_l4d2.m_render_context_depth_range )( this, z_near, z_far );
}

void IMatRenderContext::ClearBuffers( bool clear_color, bool clear_depth, bool clear_stencil )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, bool, bool, bool ) >(
        this, g_l4d2.m_render_context_clear_buffers )( this, clear_color, clear_depth, clear_stencil );
}

void IMatRenderContext::Viewport( i32 x, i32 y, i32 width, i32 height )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, i32, i32, i32, i32 ) >(
        this, g_l4d2.m_render_context_viewport )( this, x, y, width, height );
}

void IMatRenderContext::ClearColor4ub( u8 r, u8 g, u8 b, u8 a )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, u8, u8, u8, u8 ) >(
        this, g_l4d2.m_render_context_clear_color_4ub )( this, r, g, b, a );
}

void IMatRenderContext::DrawScreenSpaceRectangle(
    IMaterial *material,
    i32        dest_x,
    i32        dest_y,
    i32        width,
    i32        height,
    float      src_x0,
    float      src_y0,
    float      src_x1,
    float      src_y1,
    i32        src_width,
    i32        src_height )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, IMaterial *, i32, i32, i32, i32, float, float, float, float, i32, i32, void *, i32, i32 ) >(
        this, g_l4d2.m_render_context_draw_screen_space_rectangle )(
        this, material, dest_x, dest_y, width, height, src_x0, src_y0, src_x1, src_y1, src_width, src_height, nullptr, 1, 1 );
}

void IMatRenderContext::PushRenderTargetAndViewport( )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_render_context_push_render_target_and_viewport )( this );
}

void IMatRenderContext::PopRenderTargetAndViewport( )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_render_context_pop_render_target_and_viewport )( this );
}

void IMatRenderContext::SetStencilState( const ShaderStencilState_t &state )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, const ShaderStencilState_t & ) >(
        this, g_l4d2.m_render_context_set_stencil_state )( this, state );
}

IMaterial *CMaterialSystem::CreateMaterial( const char *name, KeyValues *key_values )
{
    return util::call_virtual< IMaterial *( ATLAS_THISCALL * )( void *, const char *, KeyValues * ) >(
        this, g_l4d2.m_material_system_create_material )( this, name, key_values );
}

IMaterial *CMaterialSystem::FindMaterial( const char *name, const char *texture_group )
{
    return util::call_virtual< IMaterial *( ATLAS_THISCALL * )( void *, const char *, const char *, bool, const char * ) >(
        this, g_l4d2.m_material_system_find_material )( this, name, texture_group, true, nullptr );
}

ITexture *CMaterialSystem::FindTexture( const char *name, const char *texture_group )
{
    return util::call_virtual< ITexture *( ATLAS_THISCALL * )( void *, const char *, const char *, bool, i32 ) >(
        this, g_l4d2.m_material_system_find_texture )( this, name, texture_group, true, 0 );
}

IMatRenderContext *CMaterialSystem::GetRenderContext( )
{
    return util::call_virtual< IMatRenderContext *( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_material_system_get_render_context )( this );
}

void CVRenderView::SetBlend( float blend )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, float ) >(
        this, g_l4d2.m_render_view_set_blend )( this, blend );
}

float CVRenderView::GetBlend( )
{
    return util::call_virtual< float( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_render_view_get_blend )( this );
}

void CVRenderView::SetColorModulation( const float *color )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, const float * ) >(
        this, g_l4d2.m_render_view_set_color_modulation )( this, color );
}

void CVRenderView::GetColorModulation( float *color )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, float * ) >(
        this, g_l4d2.m_render_view_get_color_modulation )( this, color );
}
