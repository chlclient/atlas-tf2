#pragma once

#include "entities.hpp"

#define MAKE_VFUNC_IDX( idx_name ) \
    u16 m_##idx_name{ 1024 }

class l4d2
{
public:
    CHLClient         *m_client{ };
    CClientEntityList *m_entity_list{ };
    CEngineClient     *m_engine{ };
    CEngineVGui       *m_vgui{ };
    CMatSystemSurface *m_surface{ };
    CInput            *m_input{ };
    ICvar             *m_cvar{ };
    CGlobalVarsBase   *m_globals{ };
    ClientModeShared  *m_client_mode{ };
    CPrediction       *m_prediction{ };
    CMoveHelperClient *m_move_helper{ };
    IPredictionSystem *m_prediction_systems{ };
    IMemAlloc         *m_alloc{ };
    CSplitScreen      *m_split_screen{ };
    IDemoPlayer       *m_demo_player{ };
    IDemoRecorder     *m_demo_recorder{ };
    CMaterialSystem   *m_material_system{ };
    CStudioRender     *m_studio_render{ };
    CVRenderView      *m_render_view{ };
    CModelRender      *m_model_render{ };
    CEngineTrace      *m_engine_trace{ };
    CModelInfo        *m_model_info{ };
    IKeyValuesSystem  *m_kv_system{ };

    double *m_net_time{ };
    float  *m_host_frametime_unbounded{ };
    float  *m_host_frametime_stddeviation{ };
    void   *m_CLC_Move_vmt{ };
    void   *m_NET_Tick_vmt{ };

    ConVar *sv_airaccelerate{ };
    ConVar *cl_cmdrate{ };
    ConVar *host_limitlocal{ };
    ConVar *survivor_incapacitated_accuracy_penalty{ };
    ConVar *tongue_victim_accuracy_penalty{ };
    ConVar *upgrade_laser_sight_spread_factor{ };
    ConVar *cl_interpolate{ };
    ConVar *cl_interp{ };
    ConVar *cl_interp_ratio{ };
    ConVar *cl_updaterate{ };
    ConVar *sv_client_min_interp_ratio{ };
    ConVar *sv_client_max_interp_ratio{ };
    ConVar *sv_minupdaterate{ };
    ConVar *sv_maxupdaterate{ };

    CUniformRandomStream m_random{ };

    util::pe_module m_client_dll{ };
    util::pe_module m_engine_dll{ };
    util::pe_module m_vstdlib_dll{ };
    util::pe_module m_vguimatsurface_dll{ };
    util::pe_module m_tier0_dll{ };
    util::pe_module m_materialsystem_dll{ };
    util::pe_module m_studiorender_dll{ };

    util::address m_KeyValuesSystem_fn{ };
    util::address m_Tier0_Warning_fn{ };
    util::address m_KeyValues_ctor{ };
    util::address m_KeyValues_SetString{ };
    util::address m_KeyValues_SetInt{ };
    util::address m_CUserCmd_GetChecksum{ };
    util::address m_CPredictionCopy_TransferData{ };
    util::address m_CBaseAnimating_GetSequenceActivity{ };
    util::address m_CL_RunPrediction{ };
    util::address m_CClientState_IsPaused{ };
    util::address m_CClientState_ForceFullUpdate{ };
    util::address m_Host_ShouldRun{ };
    util::address m_GetFileWeaponInfoFromHandle{ };
    util::address m_CUniformRandomStream_ctor{ };
    util::address m_CBaseAnimating_PushBoneAccess{ };
    util::address m_CBaseAnimating_PopBoneAccess{ };

    MAKE_VFUNC_IDX( chl_client_get_all_classes );
    MAKE_VFUNC_IDX( client_entity_list_get_client_entity );
    MAKE_VFUNC_IDX( client_entity_list_get_client_entity_from_handle );
    MAKE_VFUNC_IDX( client_entity_list_get_highest_entity_index );
    MAKE_VFUNC_IDX( client_networkable_get_client_class );
    MAKE_VFUNC_IDX( client_networkable_is_dormant );
    MAKE_VFUNC_IDX( client_networkable_entindex );
    MAKE_VFUNC_IDX( base_entity_get_pred_desc_map );
    MAKE_VFUNC_IDX( engine_client_get_screen_size );
    MAKE_VFUNC_IDX( engine_client_get_local_player );
    MAKE_VFUNC_IDX( engine_client_is_in_game );
    MAKE_VFUNC_IDX( engine_client_world_to_screen_matrix );
    MAKE_VFUNC_IDX( engine_client_get_active_split_screen_player_slot );
    MAKE_VFUNC_IDX( cvar_find_var );
    MAKE_VFUNC_IDX( mat_system_surface_unlock_cursor );
    MAKE_VFUNC_IDX( prediction_get_local_view_angles );
    MAKE_VFUNC_IDX( prediction_set_local_view_angles );
    MAKE_VFUNC_IDX( prediction_run_command );
    MAKE_VFUNC_IDX( move_helper_client_set_host );
    MAKE_VFUNC_IDX( mem_alloc_alloc );
    MAKE_VFUNC_IDX( mem_alloc_realloc );
    MAKE_VFUNC_IDX( mem_alloc_free );
    MAKE_VFUNC_IDX( base_player_item_post_frame );
    MAKE_VFUNC_IDX( chl_client_create_move );
    MAKE_VFUNC_IDX( chl_client_write_usercmd_delta_to_buffer );
    MAKE_VFUNC_IDX( net_channel_is_loopback );
    MAKE_VFUNC_IDX( net_channel_is_timing_out );
    MAKE_VFUNC_IDX( net_channel_get_latency );
    MAKE_VFUNC_IDX( net_channel_send_net_msg );
    MAKE_VFUNC_IDX( net_channel_set_choked );
    MAKE_VFUNC_IDX( net_channel_send_datagram );
    MAKE_VFUNC_IDX( net_channel_can_packet );
    MAKE_VFUNC_IDX( split_screen_set_active_split_screen_player_slot );
    MAKE_VFUNC_IDX( split_screen_first_valid_split_screen_slot );
    MAKE_VFUNC_IDX( split_screen_next_valid_split_screen_slot );
    MAKE_VFUNC_IDX( split_screen_is_disconnecting );
    MAKE_VFUNC_IDX( split_screen_set_local_player_is_resolvable );
    MAKE_VFUNC_IDX( demo_player_is_playing_back );
    MAKE_VFUNC_IDX( demo_recorder_is_recording );
    MAKE_VFUNC_IDX( demo_recorder_record_user_input );
    MAKE_VFUNC_IDX( client_renderable_get_model );
    MAKE_VFUNC_IDX( client_renderable_draw_model );
    MAKE_VFUNC_IDX( client_renderable_setup_bones );
    MAKE_VFUNC_IDX( engine_trace_trace_ray );
    MAKE_VFUNC_IDX( model_info_get_studiomodel );
    MAKE_VFUNC_IDX( terror_weapon_is_fully_automatic );
    MAKE_VFUNC_IDX( material_increment_reference_count );
    MAKE_VFUNC_IDX( key_values_system_alloc_key_values_memory );
    MAKE_VFUNC_IDX( render_context_release );
    MAKE_VFUNC_IDX( render_context_begin_render );
    MAKE_VFUNC_IDX( render_context_end_render );
    MAKE_VFUNC_IDX( render_context_set_render_target );
    MAKE_VFUNC_IDX( render_context_depth_range );
    MAKE_VFUNC_IDX( render_context_clear_buffers );
    MAKE_VFUNC_IDX( render_context_viewport );
    MAKE_VFUNC_IDX( render_context_clear_color_4ub );
    MAKE_VFUNC_IDX( render_context_draw_screen_space_rectangle );
    MAKE_VFUNC_IDX( render_context_push_render_target_and_viewport );
    MAKE_VFUNC_IDX( render_context_pop_render_target_and_viewport );
    MAKE_VFUNC_IDX( render_context_set_stencil_state );
    MAKE_VFUNC_IDX( material_system_create_material );
    MAKE_VFUNC_IDX( material_system_find_material );
    MAKE_VFUNC_IDX( material_system_find_texture );
    MAKE_VFUNC_IDX( material_system_get_render_context );
    MAKE_VFUNC_IDX( render_view_set_blend );
    MAKE_VFUNC_IDX( render_view_get_blend );
    MAKE_VFUNC_IDX( render_view_set_color_modulation );
    MAKE_VFUNC_IDX( render_view_get_color_modulation );
};

class client
{
public:
    atl::atomic< bool > m_is_running{ false };

    C_TerrorPlayer     *m_local{ };
    C_BaseCombatWeapon *m_weapon{ };
    CUserCmd           *m_cmd{ };
    bool                m_send_packet{ true };

    VMatrix m_w2s{ };
    i32     m_screen_width{ };
    i32     m_screen_height{ };
    float   m_view_fov{ };

    void  update_local_data( );
    void  fix_movement( CUserCmd *cmd, const vec3 &original_angles );
    void  update_w2s( );
    bool  world_to_screen( const vec3 &world, vec2 &screen ) const;
    bool  clip_segment_to_screen( const vec3 &start, const vec3 &end, vec2 &screen_start, vec2 &screen_end ) const;
    void  draw_3d_line( const vec3 &start, const vec3 &end, const color_t &color ) const;
    usize render_target( C_BaseEntity *entity ) const;
    usize aim_target( C_BaseEntity *entity ) const;
};

class hooks
{
public:
    api_hook m_present{ };
    api_hook m_reset{ };
    api_hook m_frame_stage_notify{ };
    api_hook m_level_shutdown{ };
    api_hook m_paint{ };
    api_hook m_create_move{ };
    api_hook m_get_usercmd{ };
    api_hook m_override_view{ };
    api_hook m_lock_cursor{ };
    api_hook m_set_cursor_position{ };
    api_hook m_set_cursor{ };
    api_hook m_calc_view_model_view{ };
    api_hook m_post_think{ };
    api_hook m_item_post_frame{ };
    api_hook m_burst_post_frame{ };
    api_hook m_cl_move{ };
    api_hook m_cl_read_packets{ };
    api_hook m_do_post_screen_space_effects{ };
    api_hook m_forced_material_override{ };
    api_hook m_draw_model_execute{ };
    api_hook m_get_client_model_renderable{ };
    api_hook m_avoid_pushaway_props{ };
};

extern l4d2   g_l4d2;
extern client g_cl;
extern hooks  g_hooks;
extern api    g_api;

struct globals_backup_t
{
    float m_curtime{ };
    float m_frametime{ };
    i32   m_tickcount{ };

    void store( )
    {
        m_curtime   = g_l4d2.m_globals->curtime;
        m_frametime = g_l4d2.m_globals->frametime;
        m_tickcount = g_l4d2.m_globals->tickcount;
    }

    void restore( ) const
    {
        g_l4d2.m_globals->curtime   = m_curtime;
        g_l4d2.m_globals->frametime = m_frametime;
        g_l4d2.m_globals->tickcount = m_tickcount;
    }
};

#define TICK_INTERVAL ( g_l4d2.m_globals->interval_per_tick )

#define TIME_TO_TICKS( dt ) ( static_cast< i32 >( 0.5f + static_cast< float >( dt ) / TICK_INTERVAL ) )

#if defined( ATLAS_DEV )
#define DBGOUT( fmtstr, ... ) g_l4d2.m_Tier0_Warning_fn.call_variadic< void >( "[!] " fmtstr "\n", __VA_ARGS__ )
#else
#define DBGOUT( fmtstr, ... )
#endif
