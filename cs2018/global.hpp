#pragma once

#include "entities.hpp"

#define MAKE_VFUNC_IDX( idx_name ) \
    u16 m_##idx_name{ 1024 }

class csgo
{
public:
    CHLClient         *m_client{ };
    CClientEntityList *m_entity_list{ };
    CEngineClient     *m_engine{ };
    CEngineVGui       *m_vgui{ };
    CMatSystemSurface *m_surface{ };
    ClientModeShared  *m_client_mode{ };
    CInput            *m_input{ };
    CGlobalVarsBase   *m_globals{ };
    ICvar             *m_cvar{ };
    CEngineTrace      *m_trace{ };
    C_PlayerResource **m_player_resource{ };
    CPrediction       *m_prediction{ };
    CMoveHelperClient *m_move_helper{ };
    IPredictionSystem *m_prediction_systems{ };
    IMemAlloc         *m_alloc{ };
    datamap_t         *m_C_CSPlayer_datamap{ };
    CClientState     **m_client_states{ };
    CDemoPlayer       *m_demo_player{ };
    CDemoRecorder     *m_demo_recorder{ };

    CStudioRenderContext *m_studio_render{ };
    CMaterialSystem      *m_material_system{ };
    IMDLCache            *m_mdl_cache{ };

    CModelInfoClient     *m_model_info{ };
    IFileSystem          *m_filesystem{ };
    IPhysicsSurfaceProps *m_physics_surface_props{ };
    CGameEventManager    *m_game_events{ };
    CViewRenderBeams     *m_view_render_beams{ };

#if defined( ATLAS_DEV )
    IPlayerInfoManager      *m_player_info_manager{ };
    ILagCompensationManager *m_lag_compensation{ };
#endif // ATLAS_DEV

    ConVar *sv_airaccelerate{ };
    ConVar *sv_gravity{ };
    ConVar *view_recoil_tracking{ };
    ConVar *mat_postprocess_enable{ };
    ConVar *weapon_debug_spread_show{ };
    ConVar *ff_damage_reduction_bullets{ };
    ConVar *ff_damage_bullet_penetration{ };
    ConVar *sv_clip_penetration_traces_to_players{ };
    ConVar *mp_teammates_are_enemies{ };
    ConVar *mp_friendlyfire{ };
    ConVar *mp_damage_scale_ct_head{ };
    ConVar *mp_damage_scale_ct_body{ };
    ConVar *mp_damage_scale_t_head{ };
    ConVar *mp_damage_scale_t_body{ };
    ConVar *game_type{ };
    ConVar *weapon_accuracy_shotgun_spread_patterns{ };
    ConVar *sv_friction{ };
    ConVar *sv_stopspeed{ };
    ConVar *sv_accelerate{ };
    ConVar *sv_accelerate_use_weapon_speed{ };

    ConVar *host_limitlocal{ };
    ConVar *cl_cmdrate{ };
    ConVar *developer{ };
    ConVar *rate{ };
    ConVar *cl_updaterate{ };
    ConVar *cl_interp_ratio{ };
    ConVar *cl_interp{ };
    ConVar *cl_smooth{ };
    ConVar *cl_pred_optimize{ };
    ConVar *sv_lan{ };
    ConVar *sv_maxcmdrate{ };
    ConVar *sv_minupdaterate{ };
    ConVar *sv_maxupdaterate{ };
    ConVar *sv_client_min_interp_ratio{ };
    ConVar *sv_client_max_interp_ratio{ };
    ConVar *sv_maxrate{ };
    ConVar *sv_minrate{ };
    ConVar *mat_queue_mode{ };
    ConVar *r_threaded_particles{ };
    ConVar *r_hunkalloclightmaps{ };
    ConVar *mat_forcemanagedtextureintohardware{ };
    ConVar *r_drawbatchdecals{ };
    ConVar *r_drawtracers_firstperson{ };

    util::pe_module m_d3d9_dll{ };
    util::pe_module m_client_dll{ };
    util::pe_module m_engine_dll{ };
    util::pe_module m_vguimatsurface_dll{ };
    util::pe_module m_tier0_dll{ };
    util::pe_module m_vstdlib_dll{ };
    util::pe_module m_studiorender_dll{ };
    util::pe_module m_materialsystem_dll{ };
    util::pe_module m_datacache_dll{ };
    util::pe_module m_filesystem_stdio_dll{ };
    util::pe_module m_vphysics_dll{ };

    util::address m_Tier0_Warning_fn{ };
    util::address m_CUserCmd_GetChecksum{ };
    util::address m_CPredictionCopy_TransferData{ };
    util::address m_CL_RunPrediction{ };
    util::address m_C_BaseEntity_ShutdownPredictable{ };

    util::address m_Tier0_DevMsg_fn{ };
    util::address m_CUniformRandomStream_ctor{ };
    util::address m_CUniformRandomStream_SetSeed{ };
    util::address m_CUniformRandomStream_RandomFloat{ };
    util::address m_CUniformRandomStream_RandomInt{ };
    util::address m_Tier0_ConColorMsg_fn{ };
    util::address m_Host_ShouldRun_fn{ };
    util::address m_CClientState_IsPaused{ };
    util::address m_CBaseClientState_FreeEntityBaselines{ };
    util::address m_C_EconEntity_UpdateFallbackAttributes{ };
    util::address m_C_BaseViewModel_UpdateAllViewmodelAddons{ };
    util::address m_C_BaseViewModel_RemoveViewmodelArmModels{ };
    util::address m_CustomMaterialOwner_ClearMaterials{ };
    util::address m_C_WeaponCSBase_CheckCustomMaterial{ };
    util::address m_SFWeaponSelection_RefreshWeapon{ };
    util::address m_CCSGOPlayerAnimState_Update{ };
    util::address m_C_BaseAnimating_SetupBones{ };
    util::address m_C_BaseFlex_BuildTransformations{ };
    util::address m_C_BaseEntity_SetAbsOrigin{ };
    util::address m_C_BaseEntity_SetAbsAngles{ };
    util::address m_CCSGOPlayerAnimState_UpdateLayerOrderPreset{ };
    util::address m_CCSGOPlayerAnimState_GetWeaponPrefix{ };
    util::address m_C_BaseAnimating_LookupSequence{ };
    util::address m_C_BaseAnimating_LookupBone{ };
    util::address m_CStudioHdr_pSeqdesc{ };
    util::address m_GetSequenceActivity{ };
    util::address m_IsBreakableEntity{ };

    const i32    *m_model_bone_counter{ };
    const double *m_net_time{ };
    const float  *m_host_interval_per_tick{ };
    const float  *m_host_frameendtime_computationduration{ };
    const float  *m_host_frametime_stddeviation{ };
    const float  *m_host_framestarttime_stddeviation{ };

    void *m_CCLCMsg_Move_vmt{ };
    void *m_CCLCMsg_Move_message_vmt{ };
    void *m_CNETMsg_Tick_vmt{ };
    void *m_CNETMsg_Tick_message_vmt{ };
    void *m_CTraceFilterSkipTwoEntities_vmt{ };

    MAKE_VFUNC_IDX( chl_client_get_all_classes );
    MAKE_VFUNC_IDX( client_entity_list_get_client_entity );
    MAKE_VFUNC_IDX( client_entity_list_get_client_entity_from_handle );
    MAKE_VFUNC_IDX( client_networkable_is_dormant );
    MAKE_VFUNC_IDX( base_entity_get_abs_origin );
    MAKE_VFUNC_IDX( base_entity_world_space_center );
    MAKE_VFUNC_IDX( base_entity_is_player );
    MAKE_VFUNC_IDX( base_entity_my_combat_character_pointer );
    MAKE_VFUNC_IDX( cs_player_get_layer_sequence_cycle_rate );
    MAKE_VFUNC_IDX( engine_client_get_player_info );
    MAKE_VFUNC_IDX( engine_client_get_player_for_user_id );
    MAKE_VFUNC_IDX( engine_client_get_local_player );
    MAKE_VFUNC_IDX( engine_client_get_max_clients );
    MAKE_VFUNC_IDX( engine_client_is_in_game );
    MAKE_VFUNC_IDX( engine_client_world_to_screen_matrix );
    MAKE_VFUNC_IDX( mat_system_surface_unlock_cursor );
    MAKE_VFUNC_IDX( mat_system_surface_play_sound );
    MAKE_VFUNC_IDX( engine_client_get_view_angles );
    MAKE_VFUNC_IDX( engine_client_get_net_channel_info );
    MAKE_VFUNC_IDX( net_channel_get_latency );
    MAKE_VFUNC_IDX( engine_trace_get_point_contents );
    MAKE_VFUNC_IDX( engine_trace_clip_ray_to_entity );
    MAKE_VFUNC_IDX( engine_trace_trace_ray );
    MAKE_VFUNC_IDX( physics_surface_props_get_surface_data );
    MAKE_VFUNC_IDX( weapon_cs_base_get_cs_wpn_data );
    MAKE_VFUNC_IDX( weapon_cs_base_get_spread );
    MAKE_VFUNC_IDX( weapon_cs_base_get_inaccuracy );
    MAKE_VFUNC_IDX( weapon_cs_base_update_accuracy_penalty );
    MAKE_VFUNC_IDX( weapon_cs_base_is_full_auto );
    MAKE_VFUNC_IDX( input_cam_is_third_person );
    MAKE_VFUNC_IDX( input_cam_to_third_person );
    MAKE_VFUNC_IDX( input_cam_to_first_person );
    MAKE_VFUNC_IDX( cvar_find_var );
    MAKE_VFUNC_IDX( convar_internal_set_int_value );
    MAKE_VFUNC_IDX( cs_player_get_aim_punch_angle_scaled );
    MAKE_VFUNC_IDX( prediction_get_local_view_angles );
    MAKE_VFUNC_IDX( prediction_set_local_view_angles );
    MAKE_VFUNC_IDX( prediction_run_command );
    MAKE_VFUNC_IDX( move_helper_client_set_host );
    MAKE_VFUNC_IDX( mem_alloc_alloc );
    MAKE_VFUNC_IDX( mem_alloc_realloc );
    MAKE_VFUNC_IDX( mem_alloc_free );
    MAKE_VFUNC_IDX( base_entity_get_pred_desc_map );
    MAKE_VFUNC_IDX( client_networkable_entindex );
    MAKE_VFUNC_IDX( base_player_item_post_frame );
    MAKE_VFUNC_IDX( material_system_get_render_context );
    MAKE_VFUNC_IDX( render_context_release );
    MAKE_VFUNC_IDX( render_context_get_call_queue );
    MAKE_VFUNC_IDX( render_context_get_matrix );
    MAKE_VFUNC_IDX( call_queue_queue_functor_internal );
    MAKE_VFUNC_IDX( mesh_handle_late_creation );
    MAKE_VFUNC_IDX( mdl_cache_get_studio_hdr );
    MAKE_VFUNC_IDX( mdl_cache_get_hardware_data );
    MAKE_VFUNC_IDX( client_renderable_get_model );
    MAKE_VFUNC_IDX( client_renderable_setup_bones );
    MAKE_VFUNC_IDX( chl_client_create_move );
    MAKE_VFUNC_IDX( chl_client_write_usercmd_delta_to_buffer );
    MAKE_VFUNC_IDX( demo_player_is_playing_back );
    MAKE_VFUNC_IDX( demo_recorder_record_user_input );
    MAKE_VFUNC_IDX( demo_recorder_is_recording );
    MAKE_VFUNC_IDX( net_channel_is_loopback );
    MAKE_VFUNC_IDX( net_channel_is_timing_out );
    MAKE_VFUNC_IDX( net_channel_send_net_msg );
    MAKE_VFUNC_IDX( net_channel_set_choked );
    MAKE_VFUNC_IDX( net_channel_send_datagram );
    MAKE_VFUNC_IDX( net_channel_can_packet );
    MAKE_VFUNC_IDX( model_info_get_model_index );
    MAKE_VFUNC_IDX( view_render_beams_create_beam_points );
    MAKE_VFUNC_IDX( filesystem_open );
    MAKE_VFUNC_IDX( filesystem_size );
    MAKE_VFUNC_IDX( filesystem_read );
    MAKE_VFUNC_IDX( filesystem_close );
    MAKE_VFUNC_IDX( base_entity_set_model_index );
    MAKE_VFUNC_IDX( client_networkable_release );
    MAKE_VFUNC_IDX( client_networkable_pre_data_update );
    MAKE_VFUNC_IDX( client_networkable_post_data_update );
    MAKE_VFUNC_IDX( game_event_get_name );
    MAKE_VFUNC_IDX( game_event_get_int );
    MAKE_VFUNC_IDX( game_event_get_float );
    MAKE_VFUNC_IDX( game_event_get_string );
    MAKE_VFUNC_IDX( game_event_set_string );
    MAKE_VFUNC_IDX( game_event_manager_add_listener );
    MAKE_VFUNC_IDX( client_networkable_on_data_changed );
    MAKE_VFUNC_IDX( econ_wearable_equip );
    MAKE_VFUNC_IDX( custom_material_owner_copy_to );

#if defined( ATLAS_DEV )
    MAKE_VFUNC_IDX( base_animating_setup_bones );
#endif // ATLAS_DEV
};

class client
{
public:
    C_CSPlayer *m_local{ };
    CUserCmd   *m_cmd{ };

    bool m_send_packet{ true };
    bool m_final_tick{ };

    atl::atomic< bool > m_is_running{ false };

    VMatrix            m_w2s{ };
    atl::atomic< i32 > m_screen_width{ };
    atl::atomic< i32 > m_screen_height{ };

    void on_directx_frame( void *device );
    void update_local_data( );
    void update_w2s( );
    bool world_to_screen( const vec3 &world, vec2 &screen ) const;
    bool is_on_screen( const rect_t &bounds ) const;
    void fix_movement( CUserCmd *cmd, const vec3 &original_angles ) const;
    void trace_line( const vec3 &start, const vec3 &end, u32 mask, ITraceFilter *filter, trace_t &trace ) const;
    void clip_trace_to_players( const vec3 &start, const vec3 &end, u32 mask, ITraceFilter *filter, trace_t &trace,
                                float min_range ) const;
};

class hooks
{
public:
    api_hook m_present{ };
    api_hook m_reset{ };
    api_hook m_frame_stage_notify{ };
    api_hook m_level_init_pre_entity{ };
    api_hook m_paint{ };
    api_hook m_create_move{ };
    api_hook m_override_view{ };
    api_hook m_set_cursor{ };
    api_hook m_set_cursor_position{ };
    api_hook m_lock_cursor{ };
    api_hook m_s_start_sound{ };
    api_hook m_landing_dip{ };
    api_hook m_hud_scope_paint{ };
    api_hook m_calc_view_model_view{ };
    api_hook m_death_notice_player_death{ };
    api_hook m_get_view_model_fov{ };
    api_hook m_post_think{ };
    api_hook m_check_init_predictable{ };
    api_hook m_run_simulation{ };
    api_hook m_draw_model{ };
    api_hook m_internal_draw_model{ };
    api_hook m_do_post_screen_space_effects{ };
    api_hook m_get_client_model_renderable{ };
    api_hook m_cl_move{ };
    api_hook m_packet_start{ };
    api_hook m_update_client_side_animation{ };
    api_hook m_interpolate{ };
    api_hook m_setup_bones{ };
    api_hook m_build_transformations{ };
    api_hook m_do_extra_bone_processing{ };
    api_hook m_estimate_abs_velocity{ };
    api_hook m_check_for_layer_changes{ };
    api_hook m_set_up_movement{ };
    api_hook m_check_jump_button{ };
};

extern csgo   g_csgo;
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
        m_curtime   = g_csgo.m_globals->curtime;
        m_frametime = g_csgo.m_globals->frametime;
        m_tickcount = g_csgo.m_globals->tickcount;
    }

    void restore( ) const
    {
        g_csgo.m_globals->curtime   = m_curtime;
        g_csgo.m_globals->frametime = m_frametime;
        g_csgo.m_globals->tickcount = m_tickcount;
    }
};

#define TICK_INTERVAL       ( g_csgo.m_globals->interval_per_tick )
#define TIME_TO_TICKS( dt ) ( static_cast< i32 >( 0.5 + static_cast< double >( dt ) / static_cast< double >( TICK_INTERVAL ) ) )
#define TICKS_TO_TIME( t )  ( TICK_INTERVAL * ( t ) )

#if defined( ATLAS_DEV )
#define DBGOUT( fmtstr, ... ) g_csgo.m_Tier0_Warning_fn.call_variadic< void >( "[!] " fmtstr "\n", __VA_ARGS__ )
#else
#define DBGOUT( fmtstr, ... )
#endif
