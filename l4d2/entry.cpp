#include "global.hpp"
#include "menu.hpp"

#define RAISE_ERROR( msg )                                                                                    \
    I( MessageBoxA )( NULL, ATLAS_XORSTR( msg ).get( ), ATLAS_XORSTR( "atlas-error" ).get( ), MB_ICONERROR ); \
    __debugbreak( )

#include "hooks.inl"

u32 ATLAS_STDCALL atlas_thread( void *module )
{
    if ( !g_api.startup( module ) )
    {
        RAISE_ERROR( "Failed to retrieve required startup info.\n"
                     "Please check to make sure your anti-virus is disabled, otherwise make a support ticket if this issue persists." );
        return 1;
    }

    // d3d9
    {
        const u64       started{ I( GetTickCount64 )( ) };
        util::pe_module d3d9_dll{ };

        do
        {
            d3d9_dll = util::find_module( ATLAS_XORWSTR( L"d3d9.dll" ).get( ) );

            if ( d3d9_dll.is_null( ) )
            {
                I( Sleep )( 50 );
            }
        } while ( d3d9_dll.is_null( ) && I( GetTickCount64 )( ) - started < 60000 ); // ~60s

        if ( d3d9_dll.is_null( ) )
        {
            RAISE_ERROR( "Early initialization failed.\n"
                         "Please check to make sure your anti-virus is disabled, otherwise make a support ticket if this issue persists." );
            return 1;
        }
    }

    // Render
    {
        void *present_addr{ };
        void *reset_addr{ };
        render::find_device_methods( present_addr, reset_addr );

        g_hooks.m_present = api_hook{ present_addr, reinterpret_cast< void * >( &hkPresent ) };
        g_hooks.m_reset   = api_hook{ reset_addr, reinterpret_cast< void * >( &hkReset ) };

        if ( !g_hooks.m_present.enable( ) ||
             !g_hooks.m_reset.enable( ) )
        {
            RAISE_ERROR( "Early initialization failed.\n"
                         "Please check to make sure your anti-virus is disabled, otherwise make a support ticket if this issue persists." );
            return 1;
        }

        if ( !g_render.init( ) )
        {
            RAISE_ERROR( "Early initialization failed.\n"
                         "Please check to make sure your anti-virus is disabled, otherwise make a support ticket if this issue persists." );
            return 1;
        }

        if ( !register_menu( ) )
        {
            RAISE_ERROR( "Early initialization failed.\n"
                         "Please check to make sure your anti-virus is disabled, otherwise make a support ticket if this issue persists." );
            return 1;
        }
    }

    // Modules
    {
        const u64 started{ I( GetTickCount64 )( ) };
        bool      modules_ready{ false };

        do
        {
            g_l4d2.m_client_dll         = util::find_module( ATLAS_XORWSTR( L"client.dll" ).get( ) );
            g_l4d2.m_engine_dll         = util::find_module( ATLAS_XORWSTR( L"engine.dll" ).get( ) );
            g_l4d2.m_vstdlib_dll        = util::find_module( ATLAS_XORWSTR( L"vstdlib.dll" ).get( ) );
            g_l4d2.m_vguimatsurface_dll = util::find_module( ATLAS_XORWSTR( L"vguimatsurface.dll" ).get( ) );
            g_l4d2.m_tier0_dll          = util::find_module( ATLAS_XORWSTR( L"tier0.dll" ).get( ) );
            g_l4d2.m_materialsystem_dll = util::find_module( ATLAS_XORWSTR( L"materialsystem.dll" ).get( ) );
            g_l4d2.m_studiorender_dll   = util::find_module( ATLAS_XORWSTR( L"studiorender.dll" ).get( ) );

            modules_ready = !g_l4d2.m_client_dll.is_null( ) &&
                            !g_l4d2.m_engine_dll.is_null( ) &&
                            !g_l4d2.m_vstdlib_dll.is_null( ) &&
                            !g_l4d2.m_vguimatsurface_dll.is_null( ) &&
                            !g_l4d2.m_tier0_dll.is_null( ) &&
                            !g_l4d2.m_materialsystem_dll.is_null( ) &&
                            !g_l4d2.m_studiorender_dll.is_null( );

            if ( !modules_ready )
            {
                I( Sleep )( 50 );
            }
        } while ( !modules_ready && I( GetTickCount64 )( ) - started < 60000 ); // ~60s

        if ( !g_l4d2.m_client_dll.is_valid( ) ||
             !g_l4d2.m_engine_dll.is_valid( ) ||
             !g_l4d2.m_vstdlib_dll.is_valid( ) ||
             !g_l4d2.m_vguimatsurface_dll.is_valid( ) ||
             !g_l4d2.m_tier0_dll.is_valid( ) ||
             !g_l4d2.m_materialsystem_dll.is_valid( ) ||
             !g_l4d2.m_studiorender_dll.is_valid( ) ||
             !g_api.cache_module( api_module::client ) ||
             !g_api.cache_module( api_module::engine ) )
        {
            RAISE_ERROR( "Failed to acquire required game memory.\nIf this error persists, please make a support ticket." );
            return 1;
        }
    }

    // Interfaces
    {
        g_l4d2.m_client          = g_l4d2.m_client_dll.find_interface( ATLAS_XORSTR( "VClient016" ).get( ) ).get_as< CHLClient * >( );
        g_l4d2.m_entity_list     = g_l4d2.m_client_dll.find_interface( ATLAS_XORSTR( "VClientEntityList003" ).get( ) ).get_as< CClientEntityList * >( );
        g_l4d2.m_engine          = g_l4d2.m_engine_dll.find_interface( ATLAS_XORSTR( "VEngineClient013" ).get( ) ).get_as< CEngineClient * >( );
        g_l4d2.m_vgui            = g_l4d2.m_engine_dll.find_interface( ATLAS_XORSTR( "VEngineVGui001" ).get( ) ).get_as< CEngineVGui * >( );
        g_l4d2.m_cvar            = g_l4d2.m_vstdlib_dll.find_interface( ATLAS_XORSTR( "VEngineCvar007" ).get( ) ).get_as< ICvar * >( );
        g_l4d2.m_surface         = g_l4d2.m_vguimatsurface_dll.find_interface( ATLAS_XORSTR( "VGUI_Surface031" ).get( ) ).get_as< CMatSystemSurface * >( );
        g_l4d2.m_prediction      = g_l4d2.m_client_dll.find_interface( ATLAS_XORSTR( "VClientPrediction001" ).get( ) ).get_as< CPrediction * >( );
        g_l4d2.m_material_system = g_l4d2.m_materialsystem_dll.find_interface( ATLAS_XORSTR( "VMaterialSystem080" ).get( ) ).get_as< CMaterialSystem * >( );
        g_l4d2.m_studio_render   = g_l4d2.m_studiorender_dll.find_interface( ATLAS_XORSTR( "VStudioRender026" ).get( ) ).get_as< CStudioRender * >( );
        g_l4d2.m_render_view     = g_l4d2.m_engine_dll.find_interface( ATLAS_XORSTR( "VEngineRenderView013" ).get( ) ).get_as< CVRenderView * >( );
        g_l4d2.m_model_render    = g_l4d2.m_engine_dll.find_interface( ATLAS_XORSTR( "VEngineModel016" ).get( ) ).get_as< CModelRender * >( );
        g_l4d2.m_engine_trace    = g_l4d2.m_engine_dll.find_interface( ATLAS_XORSTR( "EngineTraceClient003" ).get( ) ).get_as< CEngineTrace * >( );
        g_l4d2.m_model_info      = g_l4d2.m_engine_dll.find_interface( ATLAS_XORSTR( "VModelInfoClient004" ).get( ) ).get_as< CModelInfo * >( );

        if ( !g_l4d2.m_client ||
             !g_l4d2.m_entity_list ||
             !g_l4d2.m_engine ||
             !g_l4d2.m_vgui ||
             !g_l4d2.m_cvar ||
             !g_l4d2.m_surface ||
             !g_l4d2.m_prediction ||
             !g_l4d2.m_material_system ||
             !g_l4d2.m_studio_render ||
             !g_l4d2.m_render_view ||
             !g_l4d2.m_model_render ||
             !g_l4d2.m_engine_trace ||
             !g_l4d2.m_model_info )
        {
            RAISE_ERROR( "Failed to acquire required game memory.\nThe cheat may be out of date." );
            return 1;
        }
    }

    // Exports
    {
        g_l4d2.m_KeyValuesSystem_fn        = g_l4d2.m_vstdlib_dll.find_export( ATLAS_XORSTR( "KeyValuesSystem" ).get( ) );
        g_l4d2.m_Tier0_Warning_fn          = g_l4d2.m_tier0_dll.find_export( ATLAS_XORSTR( "Warning" ).get( ) );
        g_l4d2.m_CUniformRandomStream_ctor = g_l4d2.m_vstdlib_dll.find_export( ATLAS_XORSTR( "??0CUniformRandomStream@@QAE@XZ" ).get( ) );

        if ( g_l4d2.m_KeyValuesSystem_fn.is_null( ) ||
             g_l4d2.m_Tier0_Warning_fn.is_null( ) ||
             g_l4d2.m_CUniformRandomStream_ctor.is_null( ) )
        {
            RAISE_ERROR( "Failed to acquire required game memory.\nThe cheat may be out of date." );
            return 1;
        }

        g_l4d2.m_CUniformRandomStream_ctor.get_as< void( ATLAS_THISCALL * )( void * ) >( )( &g_l4d2.m_random );
        g_l4d2.m_alloc     = g_l4d2.m_tier0_dll.find_export( ATLAS_XORSTR( "g_pMemAlloc" ).get( ) ).deref( ).get_as< IMemAlloc * >( );
        g_l4d2.m_kv_system = g_l4d2.m_KeyValuesSystem_fn.call_as< IKeyValuesSystem * >( );

        if ( !g_l4d2.m_alloc ||
             !g_l4d2.m_kv_system )
        {
            RAISE_ERROR( "Failed to acquire required game memory.\nThe cheat may be out of date." );
            return 1;
        }
    }

    // Signatures
    {
        g_l4d2.m_input                              = g_api.get_signature_pkg( HASH_CT( "l4d2.client.input" ) ).get_as< CInput * >( );
        g_l4d2.m_globals                            = g_api.get_signature_pkg( HASH_CT( "l4d2.client.globals" ) ).get_as< CGlobalVarsBase * >( );
        g_l4d2.m_prediction_systems                 = g_api.get_signature_pkg( HASH_CT( "l4d2.client.prediction_systems" ) ).get_as< IPredictionSystem * >( );
        g_l4d2.m_move_helper                        = g_api.get_signature_pkg( HASH_CT( "l4d2.client.move_helper" ) ).get_as< CMoveHelperClient * >( );
        g_l4d2.m_CUserCmd_GetChecksum               = g_api.get_signature_pkg( HASH_CT( "l4d2.client.usercmd_getchecksum" ) );
        g_l4d2.m_CPredictionCopy_TransferData       = g_api.get_signature_pkg( HASH_CT( "l4d2.client.prediction_copy_transfer_data" ) );
        g_l4d2.m_CBaseAnimating_GetSequenceActivity = g_api.get_signature_pkg( HASH_CT( "l4d2.client.base_animating_get_sequence_activity" ) );
        g_l4d2.m_CL_RunPrediction                   = g_api.get_signature_pkg( HASH_CT( "l4d2.engine.cl_run_prediction" ) );
        g_l4d2.m_CClientState_IsPaused              = g_api.get_signature_pkg( HASH_CT( "l4d2.engine.client_state_is_paused" ) );
        g_l4d2.m_CClientState_ForceFullUpdate       = g_api.get_signature_pkg( HASH_CT( "l4d2.engine.client_state_force_full_update" ) );
        g_l4d2.m_Host_ShouldRun                     = g_api.get_signature_pkg( HASH_CT( "l4d2.engine.host_should_run" ) );
        g_l4d2.m_split_screen                       = g_api.get_signature_pkg( HASH_CT( "l4d2.engine.split_screen" ) ).get_as< CSplitScreen * >( );
        g_l4d2.m_demo_player                        = g_api.get_signature_pkg( HASH_CT( "l4d2.engine.demo_player" ) ).get_as< IDemoPlayer * >( );
        g_l4d2.m_demo_recorder                      = g_api.get_signature_pkg( HASH_CT( "l4d2.engine.demo_recorder" ) ).get_as< IDemoRecorder * >( );
        g_l4d2.m_net_time                           = g_api.get_signature_pkg( HASH_CT( "l4d2.engine.net_time" ) ).get_as< double * >( );
        g_l4d2.m_host_frametime_unbounded           = g_api.get_signature_pkg( HASH_CT( "l4d2.engine.host_frametime_unbounded" ) ).get_as< float * >( );
        g_l4d2.m_host_frametime_stddeviation        = g_api.get_signature_pkg( HASH_CT( "l4d2.engine.host_frametime_stddeviation" ) ).get_as< float * >( );
        g_l4d2.m_CLC_Move_vmt                       = g_api.get_signature_pkg( HASH_CT( "l4d2.engine.clc_move_vtable" ) ).get_as< void * >( );
        g_l4d2.m_NET_Tick_vmt                       = g_api.get_signature_pkg( HASH_CT( "l4d2.engine.net_tick_vtable" ) ).get_as< void * >( );
        g_l4d2.m_client_mode                        = g_api.get_signature_pkg( HASH_CT( "l4d2.client.get_client_mode" ) ).call_as< ClientModeShared * >( );
        g_l4d2.m_KeyValues_ctor                     = g_api.get_signature_pkg( HASH_CT( "l4d2.client.keyvalues_ctor" ) );
        g_l4d2.m_KeyValues_SetString                = g_api.get_signature_pkg( HASH_CT( "l4d2.client.keyvalues_set_string" ) );
        g_l4d2.m_KeyValues_SetInt                   = g_api.get_signature_pkg( HASH_CT( "l4d2.client.keyvalues_set_int" ) );
        g_l4d2.m_GetFileWeaponInfoFromHandle        = g_api.get_signature_pkg( HASH_CT( "l4d2.client.get_file_weapon_info_from_handle" ) );
        g_l4d2.m_CBaseAnimating_PushBoneAccess     = g_api.get_signature_pkg( HASH_CT( "l4d2.client.base_animating_push_allow_bone_access" ) );
        g_l4d2.m_CBaseAnimating_PopBoneAccess      = g_api.get_signature_pkg( HASH_CT( "l4d2.client.base_animating_pop_bone_access" ) );

        if ( !g_l4d2.m_input ||
             !g_l4d2.m_globals ||
             !g_l4d2.m_client_mode ||
             !g_l4d2.m_prediction_systems ||
             !g_l4d2.m_move_helper ||
             g_l4d2.m_CUserCmd_GetChecksum.is_null( ) ||
             !g_l4d2.m_split_screen ||
             !g_l4d2.m_demo_player ||
             !g_l4d2.m_demo_recorder ||
             !g_l4d2.m_net_time ||
             !g_l4d2.m_host_frametime_unbounded ||
             !g_l4d2.m_host_frametime_stddeviation ||
             !g_l4d2.m_CLC_Move_vmt ||
             !g_l4d2.m_NET_Tick_vmt ||
             g_l4d2.m_CPredictionCopy_TransferData.is_null( ) ||
             g_l4d2.m_CBaseAnimating_GetSequenceActivity.is_null( ) ||
             g_l4d2.m_CL_RunPrediction.is_null( ) ||
             g_l4d2.m_CClientState_IsPaused.is_null( ) ||
             g_l4d2.m_CClientState_ForceFullUpdate.is_null( ) ||
             g_l4d2.m_Host_ShouldRun.is_null( ) ||
             g_l4d2.m_KeyValues_ctor.is_null( ) ||
             g_l4d2.m_KeyValues_SetString.is_null( ) ||
             g_l4d2.m_KeyValues_SetInt.is_null( ) ||
             g_l4d2.m_GetFileWeaponInfoFromHandle.is_null( ) ||
             g_l4d2.m_CBaseAnimating_PushBoneAccess.is_null( ) ||
             g_l4d2.m_CBaseAnimating_PopBoneAccess.is_null( ) )
        {
            RAISE_ERROR( "Failed to acquire required game memory.\nThe cheat may be out of date." );
            return 1;
        }
    }

    // Indices
    {
        g_l4d2.m_chl_client_get_all_classes                       = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.chl_client.get_all_classes" ) );
        g_l4d2.m_client_entity_list_get_client_entity             = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.client_entity_list.get_client_entity" ) );
        g_l4d2.m_client_entity_list_get_client_entity_from_handle = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.client_entity_list.get_client_entity_from_handle" ) );
        g_l4d2.m_client_entity_list_get_highest_entity_index      = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.client_entity_list.get_highest_entity_index" ) );
        g_l4d2.m_client_networkable_get_client_class              = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.client_networkable.get_client_class" ) );
        g_l4d2.m_client_networkable_is_dormant                    = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.client_networkable.is_dormant" ) );
        g_l4d2.m_client_networkable_entindex                      = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.client_networkable.entindex" ) );
        g_l4d2.m_base_entity_get_pred_desc_map                    = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.base_entity.get_pred_desc_map" ) );
        g_l4d2.m_engine_client_get_screen_size                    = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.engine_client.get_screen_size" ) );
        g_l4d2.m_engine_client_get_local_player                   = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.engine_client.get_local_player" ) );
        g_l4d2.m_engine_client_is_in_game                         = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.engine_client.is_in_game" ) );
        g_l4d2.m_engine_client_world_to_screen_matrix             = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.engine_client.world_to_screen_matrix" ) );
        g_l4d2.m_engine_client_get_active_split_screen_player_slot = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.engine_client.get_active_split_screen_player_slot" ) );
        g_l4d2.m_cvar_find_var                                     = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.cvar.find_var" ) );
        g_l4d2.m_mat_system_surface_unlock_cursor                  = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.mat_system_surface.unlock_cursor" ) );
        g_l4d2.m_prediction_get_local_view_angles                  = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.prediction.get_local_view_angles" ) );
        g_l4d2.m_prediction_set_local_view_angles                  = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.prediction.set_local_view_angles" ) );
        g_l4d2.m_prediction_run_command                            = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.prediction.run_command" ) );
        g_l4d2.m_move_helper_client_set_host                       = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.move_helper_client.set_host" ) );
        g_l4d2.m_mem_alloc_alloc                                   = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.mem_alloc.alloc" ) );
        g_l4d2.m_mem_alloc_realloc                                 = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.mem_alloc.realloc" ) );
        g_l4d2.m_mem_alloc_free                                    = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.mem_alloc.free" ) );
        g_l4d2.m_base_player_item_post_frame                       = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.base_player.item_post_frame" ) );
        g_l4d2.m_chl_client_create_move                            = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.chl_client.create_move" ) );
        g_l4d2.m_chl_client_write_usercmd_delta_to_buffer          = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.chl_client.write_usercmd_delta_to_buffer" ) );
        g_l4d2.m_net_channel_is_loopback                           = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.net_channel.is_loopback" ) );
        g_l4d2.m_net_channel_is_timing_out                         = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.net_channel.is_timing_out" ) );
        g_l4d2.m_net_channel_get_latency                           = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.net_channel.get_latency" ) );
        g_l4d2.m_net_channel_send_net_msg                          = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.net_channel.send_net_msg" ) );
        g_l4d2.m_net_channel_set_choked                            = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.net_channel.set_choked" ) );
        g_l4d2.m_net_channel_send_datagram                         = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.net_channel.send_datagram" ) );
        g_l4d2.m_net_channel_can_packet                            = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.net_channel.can_packet" ) );
        g_l4d2.m_split_screen_set_active_split_screen_player_slot  = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.split_screen.set_active_split_screen_player_slot" ) );
        g_l4d2.m_split_screen_first_valid_split_screen_slot        = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.split_screen.first_valid_split_screen_slot" ) );
        g_l4d2.m_split_screen_next_valid_split_screen_slot         = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.split_screen.next_valid_split_screen_slot" ) );
        g_l4d2.m_split_screen_is_disconnecting                     = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.split_screen.is_disconnecting" ) );
        g_l4d2.m_split_screen_set_local_player_is_resolvable       = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.split_screen.set_local_player_is_resolvable" ) );
        g_l4d2.m_demo_player_is_playing_back                       = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.demo_player.is_playing_back" ) );
        g_l4d2.m_demo_recorder_is_recording                        = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.demo_recorder.is_recording" ) );
        g_l4d2.m_demo_recorder_record_user_input                   = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.demo_recorder.record_user_input" ) );
        g_l4d2.m_client_renderable_get_model                       = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.client_renderable.get_model" ) );
        g_l4d2.m_client_renderable_draw_model                      = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.client_renderable.draw_model" ) );
        g_l4d2.m_client_renderable_setup_bones                     = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.client_renderable.setup_bones" ) );
        g_l4d2.m_engine_trace_trace_ray                            = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.engine_trace.trace_ray" ) );
        g_l4d2.m_model_info_get_studiomodel                        = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.model_info.get_studiomodel" ) );
        g_l4d2.m_terror_weapon_is_fully_automatic                  = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.terror_weapon.is_fully_automatic" ) );
        g_l4d2.m_material_increment_reference_count                = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.material.increment_reference_count" ) );
        g_l4d2.m_key_values_system_alloc_key_values_memory         = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.key_values_system.alloc_key_values_memory" ) );
        g_l4d2.m_render_context_release                            = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.render_context.release" ) );
        g_l4d2.m_render_context_begin_render                       = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.render_context.begin_render" ) );
        g_l4d2.m_render_context_end_render                         = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.render_context.end_render" ) );
        g_l4d2.m_render_context_set_render_target                  = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.render_context.set_render_target" ) );
        g_l4d2.m_render_context_depth_range                        = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.render_context.depth_range" ) );
        g_l4d2.m_render_context_clear_buffers                      = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.render_context.clear_buffers" ) );
        g_l4d2.m_render_context_viewport                           = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.render_context.viewport" ) );
        g_l4d2.m_render_context_clear_color_4ub                    = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.render_context.clear_color_4ub" ) );
        g_l4d2.m_render_context_draw_screen_space_rectangle        = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.render_context.draw_screen_space_rectangle" ) );
        g_l4d2.m_render_context_push_render_target_and_viewport    = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.render_context.push_render_target_and_viewport" ) );
        g_l4d2.m_render_context_pop_render_target_and_viewport     = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.render_context.pop_render_target_and_viewport" ) );
        g_l4d2.m_render_context_set_stencil_state                  = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.render_context.set_stencil_state" ) );
        g_l4d2.m_material_system_create_material                   = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.material_system.create_material" ) );
        g_l4d2.m_material_system_find_material                     = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.material_system.find_material" ) );
        g_l4d2.m_material_system_find_texture                      = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.material_system.find_texture" ) );
        g_l4d2.m_material_system_get_render_context                = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.material_system.get_render_context" ) );
        g_l4d2.m_render_view_set_blend                             = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.render_view.set_blend" ) );
        g_l4d2.m_render_view_get_blend                             = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.render_view.get_blend" ) );
        g_l4d2.m_render_view_set_color_modulation                  = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.render_view.set_color_modulation" ) );
        g_l4d2.m_render_view_get_color_modulation                  = g_api.get_index_pkg( HASH_CT( "vfunc.l4d2.render_view.get_color_modulation" ) );
    }

    // ConVars
    {
        g_l4d2.sv_airaccelerate                        = g_l4d2.m_cvar->FindVar( ATLAS_XORSTR( "sv_airaccelerate" ).get( ) );
        g_l4d2.cl_cmdrate                              = g_l4d2.m_cvar->FindVar( ATLAS_XORSTR( "cl_cmdrate" ).get( ) );
        g_l4d2.host_limitlocal                         = g_l4d2.m_cvar->FindVar( ATLAS_XORSTR( "host_limitlocal" ).get( ) );
        g_l4d2.survivor_incapacitated_accuracy_penalty = g_l4d2.m_cvar->FindVar( ATLAS_XORSTR( "survivor_incapacitated_accuracy_penalty" ).get( ) );
        g_l4d2.tongue_victim_accuracy_penalty          = g_l4d2.m_cvar->FindVar( ATLAS_XORSTR( "tongue_victim_accuracy_penalty" ).get( ) );
        g_l4d2.upgrade_laser_sight_spread_factor       = g_l4d2.m_cvar->FindVar( ATLAS_XORSTR( "upgrade_laser_sight_spread_factor" ).get( ) );
        g_l4d2.cl_interpolate                          = g_l4d2.m_cvar->FindVar( ATLAS_XORSTR( "cl_interpolate" ).get( ) );
        g_l4d2.cl_interp                               = g_l4d2.m_cvar->FindVar( ATLAS_XORSTR( "cl_interp" ).get( ) );
        g_l4d2.cl_interp_ratio                         = g_l4d2.m_cvar->FindVar( ATLAS_XORSTR( "cl_interp_ratio" ).get( ) );
        g_l4d2.cl_updaterate                           = g_l4d2.m_cvar->FindVar( ATLAS_XORSTR( "cl_updaterate" ).get( ) );
        g_l4d2.sv_client_min_interp_ratio              = g_l4d2.m_cvar->FindVar( ATLAS_XORSTR( "sv_client_min_interp_ratio" ).get( ) );
        g_l4d2.sv_client_max_interp_ratio              = g_l4d2.m_cvar->FindVar( ATLAS_XORSTR( "sv_client_max_interp_ratio" ).get( ) );
        g_l4d2.sv_minupdaterate                        = g_l4d2.m_cvar->FindVar( ATLAS_XORSTR( "sv_minupdaterate" ).get( ) );
        g_l4d2.sv_maxupdaterate                        = g_l4d2.m_cvar->FindVar( ATLAS_XORSTR( "sv_maxupdaterate" ).get( ) );

        if ( !g_l4d2.sv_airaccelerate ||
             !g_l4d2.cl_cmdrate ||
             !g_l4d2.host_limitlocal ||
             !g_l4d2.survivor_incapacitated_accuracy_penalty ||
             !g_l4d2.tongue_victim_accuracy_penalty ||
             !g_l4d2.upgrade_laser_sight_spread_factor ||
             !g_l4d2.cl_interpolate ||
             !g_l4d2.cl_interp ||
             !g_l4d2.cl_interp_ratio ||
             !g_l4d2.cl_updaterate ||
             !g_l4d2.sv_client_min_interp_ratio ||
             !g_l4d2.sv_client_max_interp_ratio ||
             !g_l4d2.sv_minupdaterate ||
             !g_l4d2.sv_maxupdaterate )
        {
            RAISE_ERROR( "Failed to acquire required game memory.\nThe cheat may be out of date." );
            return 1;
        }
    }

    // Netvars
    {
        if ( !g_netvars.load( ) )
        {
            RAISE_ERROR( "Failed to acquire required game memory.\nThe cheat may be out of date." );
            return 1;
        }
    }

    // Datamaps
    {
        if ( !g_datamap.init( ) )
        {
            RAISE_ERROR( "Failed to acquire required game memory.\nThe cheat may be out of date." );
            return 1;
        }
    }

    // Hooks
    {
        g_hooks.m_frame_stage_notify   = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.client.frame_stage_notify" ), reinterpret_cast< void * >( &hkFrameStageNotify ), g_l4d2.m_client );
        g_hooks.m_level_shutdown       = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.client.level_shutdown" ), reinterpret_cast< void * >( &hkLevelShutdown ), g_l4d2.m_client );
        g_hooks.m_paint                = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.engine_vgui.paint" ), reinterpret_cast< void * >( &hkPaint ), g_l4d2.m_vgui );
        g_hooks.m_create_move          = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.client.create_move" ), reinterpret_cast< void * >( &hkCreateMove ), g_l4d2.m_client );
        g_hooks.m_get_usercmd          = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.client.get_usercmd" ), reinterpret_cast< void * >( &hkGetUserCmd ), g_l4d2.m_input );
        g_hooks.m_override_view        = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.client_mode.override_view" ), reinterpret_cast< void * >( &hkOverrideView ), g_l4d2.m_client_mode );
        g_hooks.m_set_cursor           = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.surface.set_cursor" ), reinterpret_cast< void * >( &hkSetCursor ), g_l4d2.m_surface );
        g_hooks.m_set_cursor_position  = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.surface.set_cursor_position" ), reinterpret_cast< void * >( &hkSetCursorPosition ), g_l4d2.m_surface );
        g_hooks.m_lock_cursor          = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.surface.lock_cursor" ), reinterpret_cast< void * >( &hkLockCursor ), g_l4d2.m_surface );
        g_hooks.m_calc_view_model_view = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.client.calc_view_model_view" ), reinterpret_cast< void * >( &hkCalcViewModelView ) );
        g_hooks.m_post_think           = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.client.post_think" ), reinterpret_cast< void * >( &hkPostThink ) );
        g_hooks.m_item_post_frame      = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.client.weapon_item_post_frame" ), reinterpret_cast< void * >( &hkWeaponItemPostFrame ) );
        g_hooks.m_burst_post_frame     = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.client.burst_item_post_frame" ), reinterpret_cast< void * >( &hkBurstItemPostFrame ) );
        g_hooks.m_cl_move              = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.engine.cl_move" ), reinterpret_cast< void * >( &hkCL_Move ) );
        g_hooks.m_cl_read_packets      = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.engine.cl_read_packets" ), reinterpret_cast< void * >( &hkCL_ReadPackets ) );

        g_hooks.m_do_post_screen_space_effects = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.client_mode.do_post_screen_space_effects" ), reinterpret_cast< void * >( &hkDoPostScreenSpaceEffects ), g_l4d2.m_client_mode );
        g_hooks.m_forced_material_override     = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.studio_render.forced_material_override" ), reinterpret_cast< void * >( &hkForcedMaterialOverride ), g_l4d2.m_studio_render );
        g_hooks.m_draw_model_execute           = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.model_render.draw_model_execute" ), reinterpret_cast< void * >( &hkDrawModelExecute ), g_l4d2.m_model_render );
        g_hooks.m_get_client_model_renderable  = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.client.get_client_model_renderable" ), reinterpret_cast< void * >( &hkGetClientModelRenderable ) );
        g_hooks.m_avoid_pushaway_props         = g_api.get_hook_pkg( HASH_CT( "hook.l4d2.client.avoid_pushaway_props" ), reinterpret_cast< void * >( &hkAvoidPushawayProps ) );

        if ( !g_hooks.m_frame_stage_notify.enable( ) ||
             !g_hooks.m_level_shutdown.enable( ) ||
             !g_hooks.m_paint.enable( ) ||
             !g_hooks.m_create_move.enable( ) ||
             !g_hooks.m_get_usercmd.enable( ) ||
             !g_hooks.m_override_view.enable( ) ||
             !g_hooks.m_set_cursor.enable( ) ||
             !g_hooks.m_set_cursor_position.enable( ) ||
             !g_hooks.m_lock_cursor.enable( ) ||
             !g_hooks.m_calc_view_model_view.enable( ) ||
             !g_hooks.m_post_think.enable( ) ||
             !g_hooks.m_item_post_frame.enable( ) ||
             !g_hooks.m_burst_post_frame.enable( ) ||
             !g_hooks.m_cl_move.enable( ) ||
             !g_hooks.m_cl_read_packets.enable( ) ||
             !g_hooks.m_do_post_screen_space_effects.enable( ) ||
             !g_hooks.m_forced_material_override.enable( ) ||
             !g_hooks.m_draw_model_execute.enable( ) ||
             !g_hooks.m_get_client_model_renderable.enable( ) ||
             !g_hooks.m_avoid_pushaway_props.enable( ) )
        {
            RAISE_ERROR( "Failed to acquire required game memory.\nThe cheat may be out of date." );
            return 1;
        }
    }

    g_cl.m_is_running = true;

    push_notification( notify_kind_t::success, ATLAS_XORSTR_STATIC( "Atlas is loaded!" ), ATLAS_XORSTR_STATIC( "Press F3 or insert to open the menu." ) );

    return 0;
}

extern "C" i32 ATLAS_STDCALL atlas_entry(
    HMODULE module,
    DWORD   reason,
    LPVOID  reserved )
{
    ATLAS_UNUSED_PARAM( reserved );

#if defined( ATLAS_DISABLE_CLOUD )
    if ( reason == DLL_PROCESS_ATTACH )
    {
        I( DisableThreadLibraryCalls )( module );

        HANDLE    thread{ };
        const i32 status{ I( RtlCreateUserThread )(
            util::get_current_process( ),
            nullptr, 0, 0,
            nullptr, nullptr,
            reinterpret_cast< void * >( atlas_thread ),
            module, &thread,
            nullptr ) };

        if ( status < 0 || !thread )
        {
            return FALSE;
        }

        I( CloseHandle )( thread );
    }
#endif

    return TRUE;
}
