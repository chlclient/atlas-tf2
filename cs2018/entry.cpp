#include "global.hpp"
#include "menu.hpp"
#include "runtime/thread_pool.hpp"

#define RAISE_ERROR( msg )                                                                                    \
    I( MessageBoxA )( NULL, ATLAS_XORSTR( msg ).get( ), ATLAS_XORSTR( "atlas-error" ).get( ), MB_ICONERROR ); \
    __debugbreak( )

#include "hooks.inl"

u32 ATLAS_STDCALL atlas_thread( void *parameter )
{
    if ( !g_api.startup( parameter ) )
    {
        RAISE_ERROR( "Failed to retrieve required startup info.\n"
                     "Please check to make sure your anti-virus is disabled, otherwise make a support ticket if this issue persists." );
        return 1;
    }

    {
        const u64 started{ I( GetTickCount64 )( ) };

        do
        {
            g_csgo.m_d3d9_dll = util::find_module( ATLAS_XORWSTR( L"d3d9.dll" ).get( ) );

            if ( g_csgo.m_d3d9_dll.is_null( ) )
            {
                I( Sleep )( 50 );
            }
        } while ( g_csgo.m_d3d9_dll.is_null( ) && I( GetTickCount64 )( ) - started < 60000 );

        if ( g_csgo.m_d3d9_dll.is_null( ) )
        {
            RAISE_ERROR( "Early initialization failed.\n"
                         "Please check to make sure your anti-virus is disabled, otherwise make a support ticket if this issue persists." );
            return 1;
        }
    }

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

    {
        const u64 started{ I( GetTickCount64 )( ) };
        bool      modules_ready{ false };

        do
        {
            g_csgo.m_client_dll           = util::find_module( ATLAS_XORWSTR( L"client.dll" ).get( ) );
            g_csgo.m_engine_dll           = util::find_module( ATLAS_XORWSTR( L"engine.dll" ).get( ) );
            g_csgo.m_studiorender_dll     = util::find_module( ATLAS_XORWSTR( L"studiorender.dll" ).get( ) );
            g_csgo.m_vguimatsurface_dll   = util::find_module( ATLAS_XORWSTR( L"vguimatsurface.dll" ).get( ) );
            g_csgo.m_tier0_dll            = util::find_module( ATLAS_XORWSTR( L"tier0.dll" ).get( ) );
            g_csgo.m_materialsystem_dll   = util::find_module( ATLAS_XORWSTR( L"materialsystem.dll" ).get( ) );
            g_csgo.m_vstdlib_dll          = util::find_module( ATLAS_XORWSTR( L"vstdlib.dll" ).get( ) );
            g_csgo.m_filesystem_stdio_dll = util::find_module( ATLAS_XORWSTR( L"filesystem_stdio.dll" ).get( ) );
            g_csgo.m_datacache_dll        = util::find_module( ATLAS_XORWSTR( L"datacache.dll" ).get( ) );
            g_csgo.m_vphysics_dll         = util::find_module( ATLAS_XORWSTR( L"vphysics.dll" ).get( ) );

            modules_ready = !g_csgo.m_client_dll.is_null( ) &&
                            !g_csgo.m_engine_dll.is_null( ) &&
                            !g_csgo.m_studiorender_dll.is_null( ) &&
                            !g_csgo.m_vguimatsurface_dll.is_null( ) &&
                            !g_csgo.m_tier0_dll.is_null( ) &&
                            !g_csgo.m_materialsystem_dll.is_null( ) &&
                            !g_csgo.m_vstdlib_dll.is_null( ) &&
                            !g_csgo.m_filesystem_stdio_dll.is_null( ) &&
                            !g_csgo.m_datacache_dll.is_null( ) &&
                            !g_csgo.m_vphysics_dll.is_null( );

            if ( !modules_ready )
            {
                I( Sleep )( 50 );
            }
        } while ( !modules_ready && I( GetTickCount64 )( ) - started < 60000 );

        if ( !modules_ready )
        {
            RAISE_ERROR( "Failed to acquire required game memory.\nIf this error persists, please make a support ticket." );
            return 1;
        }

        if ( !g_csgo.m_client_dll.is_valid( ) ||
             !g_csgo.m_engine_dll.is_valid( ) ||
             !g_csgo.m_studiorender_dll.is_valid( ) ||
             !g_csgo.m_vguimatsurface_dll.is_valid( ) ||
             !g_csgo.m_tier0_dll.is_valid( ) ||
             !g_csgo.m_materialsystem_dll.is_valid( ) ||
             !g_csgo.m_vstdlib_dll.is_valid( ) ||
             !g_csgo.m_filesystem_stdio_dll.is_valid( ) ||
             !g_csgo.m_datacache_dll.is_valid( ) ||
             !g_csgo.m_vphysics_dll.is_valid( ) ||
             !g_api.cache_module( api_module::client ) ||
             !g_api.cache_module( api_module::engine ) ||
             !g_api.cache_module( api_module::studiorender ) )
        {
            RAISE_ERROR( "Failed to acquire required game memory.\nIf this error persists, please make a support ticket." );
            return 1;
        }
    }

    {
        g_csgo.m_client                = g_csgo.m_client_dll.find_interface( ATLAS_XORSTR( "VClient018" ).get( ) ).get_as< CHLClient * >( );
        g_csgo.m_prediction            = g_csgo.m_client_dll.find_interface( ATLAS_XORSTR( "VClientPrediction001" ).get( ) ).get_as< CPrediction * >( );
        g_csgo.m_entity_list           = g_csgo.m_client_dll.find_interface( ATLAS_XORSTR( "VClientEntityList003" ).get( ) ).get_as< CClientEntityList * >( );
        g_csgo.m_engine                = g_csgo.m_engine_dll.find_interface( ATLAS_XORSTR( "VEngineClient014" ).get( ) ).get_as< CEngineClient * >( );
        g_csgo.m_vgui                  = g_csgo.m_engine_dll.find_interface( ATLAS_XORSTR( "VEngineVGui001" ).get( ) ).get_as< CEngineVGui * >( );
        g_csgo.m_model_info            = g_csgo.m_engine_dll.find_interface( ATLAS_XORSTR( "VModelInfoClient004" ).get( ) ).get_as< CModelInfoClient * >( );
        g_csgo.m_trace                 = g_csgo.m_engine_dll.find_interface( ATLAS_XORSTR( "EngineTraceClient004" ).get( ) ).get_as< CEngineTrace * >( );
        g_csgo.m_cvar                  = g_csgo.m_vstdlib_dll.find_interface( ATLAS_XORSTR( "VEngineCvar007" ).get( ) ).get_as< ICvar * >( );
        g_csgo.m_material_system       = g_csgo.m_materialsystem_dll.find_interface( ATLAS_XORSTR( "VMaterialSystem080" ).get( ) ).get_as< CMaterialSystem * >( );
        g_csgo.m_surface               = g_csgo.m_vguimatsurface_dll.find_interface( ATLAS_XORSTR( "VGUI_Surface031" ).get( ) ).get_as< CMatSystemSurface * >( );
        g_csgo.m_studio_render         = g_csgo.m_studiorender_dll.find_interface( ATLAS_XORSTR( "VStudioRender026" ).get( ) ).get_as< CStudioRenderContext * >( );
        g_csgo.m_filesystem            = g_csgo.m_filesystem_stdio_dll.find_interface( ATLAS_XORSTR( "VFileSystem017" ).get( ) ).get_as< IFileSystem * >( );
        g_csgo.m_mdl_cache             = g_csgo.m_datacache_dll.find_interface( ATLAS_XORSTR( "MDLCache004" ).get( ) ).get_as< IMDLCache * >( );
        g_csgo.m_physics_surface_props = g_csgo.m_vphysics_dll.find_interface( ATLAS_XORSTR( "VPhysicsSurfaceProps001" ).get( ) ).get_as< IPhysicsSurfaceProps * >( );
        g_csgo.m_game_events           = g_csgo.m_engine_dll.find_interface( ATLAS_XORSTR( "GAMEEVENTSMANAGER002" ).get( ) ).get_as< CGameEventManager * >( );

        if ( !g_csgo.m_client ||
             !g_csgo.m_prediction ||
             !g_csgo.m_entity_list ||
             !g_csgo.m_engine ||
             !g_csgo.m_vgui ||
             !g_csgo.m_model_info ||
             !g_csgo.m_trace ||
             !g_csgo.m_cvar ||
             !g_csgo.m_material_system ||
             !g_csgo.m_surface ||
             !g_csgo.m_studio_render ||
             !g_csgo.m_filesystem ||
             !g_csgo.m_mdl_cache ||
             !g_csgo.m_physics_surface_props ||
             !g_csgo.m_game_events )
        {
            RAISE_ERROR( "Failed to acquire required game memory.\nThe cheat may be out of date." );
            return 1;
        }

#if defined( ATLAS_DEV )
        const util::pe_module server_dll{ util::find_module( ATLAS_XORWSTR( L"server.dll" ).get( ) ) };

        g_csgo.m_player_info_manager = server_dll.find_interface( ATLAS_XORSTR( "PlayerInfoManager002" ).get( ) ).get_as< IPlayerInfoManager * >( );

        const util::address finish_call{ server_dll.find_pattern( ATLAS_XORSTR( "A1 ? ? ? ? B9 ? ? ? ? 57 FF 50 04 5F 5E 8B E5" ).get( ) ) };
        if ( !finish_call.is_null( ) )
        {
            g_csgo.m_lag_compensation = util::address{ finish_call.get_raw( ) + 6 }.deref( ).get_as< ILagCompensationManager * >( );
        }
#endif // ATLAS_DEV
    }

    {
        g_csgo.m_Tier0_Warning_fn                 = g_csgo.m_tier0_dll.find_export( ATLAS_XORSTR( "Warning" ).get( ) );
        g_csgo.m_Tier0_DevMsg_fn                  = g_csgo.m_tier0_dll.find_export( ATLAS_XORSTR( "?DevMsg@@YAXPBDZZ" ).get( ) );
        g_csgo.m_Tier0_ConColorMsg_fn             = g_csgo.m_tier0_dll.find_export( ATLAS_XORSTR( "?ConColorMsg@@YAXABVColor@@PBDZZ" ).get( ) );
        g_csgo.m_CUniformRandomStream_ctor        = g_csgo.m_vstdlib_dll.find_export( ATLAS_XORSTR( "??0CUniformRandomStream@@QAE@XZ" ).get( ) );
        g_csgo.m_CUniformRandomStream_SetSeed     = g_csgo.m_vstdlib_dll.find_export( ATLAS_XORSTR( "?SetSeed@CUniformRandomStream@@UAEXH@Z" ).get( ) );
        g_csgo.m_CUniformRandomStream_RandomFloat = g_csgo.m_vstdlib_dll.find_export( ATLAS_XORSTR( "?RandomFloat@CUniformRandomStream@@UAEMMM@Z" ).get( ) );
        g_csgo.m_CUniformRandomStream_RandomInt   = g_csgo.m_vstdlib_dll.find_export( ATLAS_XORSTR( "?RandomInt@CUniformRandomStream@@UAEHHH@Z" ).get( ) );

        if ( g_csgo.m_Tier0_Warning_fn.is_null( ) ||
             g_csgo.m_Tier0_DevMsg_fn.is_null( ) ||
             g_csgo.m_Tier0_ConColorMsg_fn.is_null( ) ||
             g_csgo.m_CUniformRandomStream_ctor.is_null( ) ||
             g_csgo.m_CUniformRandomStream_SetSeed.is_null( ) ||
             g_csgo.m_CUniformRandomStream_RandomFloat.is_null( ) ||
             g_csgo.m_CUniformRandomStream_RandomInt.is_null( ) )
        {
            RAISE_ERROR( "Failed to acquire required game memory.\nThe cheat may be out of date." );
            return 1;
        }

        g_csgo.m_alloc = g_csgo.m_tier0_dll.find_export( ATLAS_XORSTR( "g_pMemAlloc" ).get( ) ).deref( ).get_as< IMemAlloc * >( );

        if ( !g_csgo.m_alloc )
        {
            RAISE_ERROR( "Failed to acquire required game memory.\nThe cheat may be out of date." );
            return 1;
        }
    }

    {
        g_csgo.m_client_mode                              = g_api.get_signature_pkg( HASH_CT( "cs2018.client.client_mode" ) ).get_as< ClientModeShared * >( );
        g_csgo.m_input                                    = g_api.get_signature_pkg( HASH_CT( "cs2018.client.input" ) ).get_as< CInput * >( );
        g_csgo.m_globals                                  = g_api.get_signature_pkg( HASH_CT( "cs2018.client.globals" ) ).get_as< CGlobalVarsBase * >( );
        g_csgo.m_player_resource                          = g_api.get_signature_pkg( HASH_CT( "cs2018.client.player_resource" ) ).get_as< C_PlayerResource ** >( );
        g_csgo.m_move_helper                              = g_api.get_signature_pkg( HASH_CT( "cs2018.client.move_helper" ) ).get_as< CMoveHelperClient * >( );
        g_csgo.m_prediction_systems                       = g_api.get_signature_pkg( HASH_CT( "cs2018.client.prediction_systems" ) ).get_as< IPredictionSystem * >( );
        g_csgo.m_C_CSPlayer_datamap                       = g_api.get_signature_pkg( HASH_CT( "cs2018.client.cs_player_datamap" ) ).get_as< datamap_t * >( );
        g_csgo.m_CUserCmd_GetChecksum                     = g_api.get_signature_pkg( HASH_CT( "cs2018.client.usercmd_getchecksum" ) );
        g_csgo.m_CPredictionCopy_TransferData             = g_api.get_signature_pkg( HASH_CT( "cs2018.client.prediction_copy_transfer_data" ) );
        g_csgo.m_C_BaseEntity_ShutdownPredictable         = g_api.get_signature_pkg( HASH_CT( "cs2018.client.shutdown_predictable" ) );
        g_csgo.m_C_EconEntity_UpdateFallbackAttributes    = g_api.get_signature_pkg( HASH_CT( "cs2018.client.econ_entity_update_fallback_attributes" ) );
        g_csgo.m_C_BaseViewModel_UpdateAllViewmodelAddons = g_api.get_signature_pkg( HASH_CT( "cs2018.client.base_view_model_update_all_viewmodel_addons" ) );
        g_csgo.m_C_BaseViewModel_RemoveViewmodelArmModels = g_api.get_signature_pkg( HASH_CT( "cs2018.client.base_view_model_remove_viewmodel_arm_models" ) );
        g_csgo.m_CustomMaterialOwner_ClearMaterials       = g_api.get_signature_pkg( HASH_CT( "cs2018.client.custom_material_owner_clear_materials" ) );
        g_csgo.m_C_WeaponCSBase_CheckCustomMaterial       = g_api.get_signature_pkg( HASH_CT( "cs2018.client.weapon_cs_base_check_custom_material" ) );
        g_csgo.m_SFWeaponSelection_RefreshWeapon          = g_api.get_signature_pkg( HASH_CT( "cs2018.client.weapon_selection_refresh_weapon" ) );
        g_csgo.m_CCSGOPlayerAnimState_Update              = g_api.get_signature_pkg( HASH_CT( "cs2018.client.anim_state_update" ) );
        g_csgo.m_C_BaseAnimating_SetupBones               = g_api.get_signature_pkg( HASH_CT( "cs2018.client.base_animating_setup_bones" ) );
        g_csgo.m_C_BaseFlex_BuildTransformations          = g_api.get_signature_pkg( HASH_CT( "cs2018.client.base_flex_build_transformations" ) );
        g_csgo.m_C_BaseEntity_SetAbsOrigin                = g_api.get_signature_pkg( HASH_CT( "cs2018.client.base_entity_set_abs_origin" ) );
        g_csgo.m_C_BaseEntity_SetAbsAngles                = g_api.get_signature_pkg( HASH_CT( "cs2018.client.base_entity_set_abs_angles" ) );
        g_csgo.m_model_bone_counter                       = g_api.get_signature_pkg( HASH_CT( "cs2018.client.model_bone_counter" ) ).get_as< const i32 * >( );

        g_csgo.m_CCSGOPlayerAnimState_UpdateLayerOrderPreset = g_api.get_signature_pkg( HASH_CT( "cs2018.client.anim_state_update_layer_order_preset" ) );
        g_csgo.m_CCSGOPlayerAnimState_GetWeaponPrefix        = g_api.get_signature_pkg( HASH_CT( "cs2018.client.anim_state_get_weapon_prefix" ) );
        g_csgo.m_C_BaseAnimating_LookupSequence              = g_api.get_signature_pkg( HASH_CT( "cs2018.client.base_animating_lookup_sequence" ) );
        g_csgo.m_C_BaseAnimating_LookupBone                  = g_api.get_signature_pkg( HASH_CT( "cs2018.client.base_animating_lookup_bone" ) );
        g_csgo.m_CStudioHdr_pSeqdesc                         = g_api.get_signature_pkg( HASH_CT( "cs2018.client.studio_hdr_seqdesc" ) );
        g_csgo.m_GetSequenceActivity                         = g_api.get_signature_pkg( HASH_CT( "cs2018.client.get_sequence_activity" ) );
        g_csgo.m_IsBreakableEntity                           = g_api.get_signature_pkg( HASH_CT( "cs2018.client.is_breakable_entity" ) );
        g_csgo.m_CTraceFilterSkipTwoEntities_vmt             = g_api.get_signature_pkg( HASH_CT( "cs2018.client.trace_filter_skip_two_entities_vmt" ) ).get_as< void * >( );
        g_csgo.m_view_render_beams                           = g_api.get_signature_pkg( HASH_CT( "cs2018.client.view_render_beams" ) ).get_as< CViewRenderBeams * >( );

        g_csgo.m_client_states                         = g_api.get_signature_pkg( HASH_CT( "cs2018.engine.client_states" ) ).get_as< CClientState ** >( );
        g_csgo.m_demo_player                           = g_api.get_signature_pkg( HASH_CT( "cs2018.engine.demo_player" ) ).get_as< CDemoPlayer * >( );
        g_csgo.m_demo_recorder                         = g_api.get_signature_pkg( HASH_CT( "cs2018.engine.demo_recorder" ) ).get_as< CDemoRecorder * >( );
        g_csgo.m_net_time                              = g_api.get_signature_pkg( HASH_CT( "cs2018.engine.net_time" ) ).get_as< const double * >( );
        g_csgo.m_host_interval_per_tick                = g_api.get_signature_pkg( HASH_CT( "cs2018.engine.host_interval_per_tick" ) ).get_as< const float * >( );
        g_csgo.m_host_frameendtime_computationduration = g_api.get_signature_pkg( HASH_CT( "cs2018.engine.host_frameendtime_computationduration" ) ).get_as< const float * >( );
        g_csgo.m_host_frametime_stddeviation           = g_api.get_signature_pkg( HASH_CT( "cs2018.engine.host_frametime_stddeviation" ) ).get_as< const float * >( );
        g_csgo.m_host_framestarttime_stddeviation      = g_api.get_signature_pkg( HASH_CT( "cs2018.engine.host_framestarttime_stddeviation" ) ).get_as< const float * >( );
        g_csgo.m_CCLCMsg_Move_vmt                      = g_api.get_signature_pkg( HASH_CT( "cs2018.engine.clc_move_vmt" ) ).get_as< void * >( );
        g_csgo.m_CCLCMsg_Move_message_vmt              = g_api.get_signature_pkg( HASH_CT( "cs2018.engine.clc_move_message_vmt" ) ).get_as< void * >( );
        g_csgo.m_CNETMsg_Tick_vmt                      = g_api.get_signature_pkg( HASH_CT( "cs2018.engine.net_tick_vmt" ) ).get_as< void * >( );
        g_csgo.m_CNETMsg_Tick_message_vmt              = g_api.get_signature_pkg( HASH_CT( "cs2018.engine.net_tick_message_vmt" ) ).get_as< void * >( );
        g_csgo.m_CL_RunPrediction                      = g_api.get_signature_pkg( HASH_CT( "cs2018.engine.cl_run_prediction" ) );
        g_csgo.m_Host_ShouldRun_fn                     = g_api.get_signature_pkg( HASH_CT( "cs2018.engine.host_should_run" ) );
        g_csgo.m_CClientState_IsPaused                 = g_api.get_signature_pkg( HASH_CT( "cs2018.engine.client_state_is_paused" ) );
        g_csgo.m_CBaseClientState_FreeEntityBaselines  = g_api.get_signature_pkg( HASH_CT( "cs2018.engine.free_entity_baselines" ) );

        if ( !g_csgo.m_client_mode ||
             !g_csgo.m_input ||
             !g_csgo.m_globals ||
             !g_csgo.m_player_resource ||
             !g_csgo.m_move_helper ||
             !g_csgo.m_prediction_systems ||
             !g_csgo.m_C_CSPlayer_datamap ||
             g_csgo.m_CUserCmd_GetChecksum.is_null( ) ||
             g_csgo.m_CPredictionCopy_TransferData.is_null( ) ||
             g_csgo.m_C_BaseEntity_ShutdownPredictable.is_null( ) ||
             g_csgo.m_C_EconEntity_UpdateFallbackAttributes.is_null( ) ||
             g_csgo.m_C_BaseViewModel_UpdateAllViewmodelAddons.is_null( ) ||
             g_csgo.m_C_BaseViewModel_RemoveViewmodelArmModels.is_null( ) ||
             g_csgo.m_CustomMaterialOwner_ClearMaterials.is_null( ) ||
             g_csgo.m_C_WeaponCSBase_CheckCustomMaterial.is_null( ) ||
             g_csgo.m_SFWeaponSelection_RefreshWeapon.is_null( ) ||
             g_csgo.m_CCSGOPlayerAnimState_Update.is_null( ) ||
             g_csgo.m_C_BaseAnimating_SetupBones.is_null( ) ||
             g_csgo.m_C_BaseFlex_BuildTransformations.is_null( ) ||
             g_csgo.m_C_BaseEntity_SetAbsOrigin.is_null( ) ||
             g_csgo.m_C_BaseEntity_SetAbsAngles.is_null( ) ||
             g_csgo.m_CCSGOPlayerAnimState_UpdateLayerOrderPreset.is_null( ) ||
             g_csgo.m_CCSGOPlayerAnimState_GetWeaponPrefix.is_null( ) ||
             g_csgo.m_C_BaseAnimating_LookupSequence.is_null( ) ||
             g_csgo.m_C_BaseAnimating_LookupBone.is_null( ) ||
             g_csgo.m_CStudioHdr_pSeqdesc.is_null( ) ||
             g_csgo.m_GetSequenceActivity.is_null( ) ||
             g_csgo.m_IsBreakableEntity.is_null( ) ||
             !g_csgo.m_CTraceFilterSkipTwoEntities_vmt ||
             !g_csgo.m_view_render_beams ||
             !g_csgo.m_model_bone_counter ||
             !g_csgo.m_client_states ||
             !g_csgo.m_demo_player ||
             !g_csgo.m_demo_recorder ||
             !g_csgo.m_net_time ||
             !g_csgo.m_host_interval_per_tick ||
             !g_csgo.m_host_frameendtime_computationduration ||
             !g_csgo.m_host_frametime_stddeviation ||
             !g_csgo.m_host_framestarttime_stddeviation ||
             !g_csgo.m_CCLCMsg_Move_vmt ||
             !g_csgo.m_CCLCMsg_Move_message_vmt ||
             !g_csgo.m_CNETMsg_Tick_vmt ||
             !g_csgo.m_CNETMsg_Tick_message_vmt ||
             g_csgo.m_CL_RunPrediction.is_null( ) ||
             g_csgo.m_Host_ShouldRun_fn.is_null( ) ||
             g_csgo.m_CClientState_IsPaused.is_null( ) ||
             g_csgo.m_CBaseClientState_FreeEntityBaselines.is_null( ) )
        {
            RAISE_ERROR( "Failed to acquire required game memory.\nThe cheat may be out of date." );
            return 1;
        }
    }

    {
        g_csgo.m_chl_client_get_all_classes                       = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.chl_client.get_all_classes" ) );
        g_csgo.m_chl_client_create_move                           = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.chl_client.create_move" ) );
        g_csgo.m_chl_client_write_usercmd_delta_to_buffer         = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.chl_client.write_usercmd_delta_to_buffer" ) );
        g_csgo.m_client_entity_list_get_client_entity             = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.client_entity_list.get_client_entity" ) );
        g_csgo.m_client_entity_list_get_client_entity_from_handle = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.client_entity_list.get_client_entity_from_handle" ) );
        g_csgo.m_client_networkable_is_dormant                    = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.client_networkable.is_dormant" ) );
        g_csgo.m_client_networkable_entindex                      = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.client_networkable.entindex" ) );
        g_csgo.m_client_networkable_release                       = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.client_networkable.release" ) );
        g_csgo.m_client_networkable_pre_data_update               = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.client_networkable.pre_data_update" ) );
        g_csgo.m_client_networkable_post_data_update              = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.client_networkable.post_data_update" ) );
        g_csgo.m_base_entity_get_abs_origin                       = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.base_entity.get_abs_origin" ) );
        g_csgo.m_base_entity_world_space_center                   = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.base_entity.world_space_center" ) );
        g_csgo.m_base_entity_is_player                            = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.base_entity.is_player" ) );
        g_csgo.m_base_entity_my_combat_character_pointer          = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.base_entity.my_combat_character_pointer" ) );
        g_csgo.m_cs_player_get_layer_sequence_cycle_rate          = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.cs_player.get_layer_sequence_cycle_rate" ) );
        g_csgo.m_base_entity_get_pred_desc_map                    = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.base_entity.get_pred_desc_map" ) );
        g_csgo.m_base_entity_set_model_index                      = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.base_entity.set_model_index" ) );
        g_csgo.m_base_player_item_post_frame                      = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.base_player.item_post_frame" ) );
        g_csgo.m_cs_player_get_aim_punch_angle_scaled             = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.cs_player.get_aim_punch_angle_scaled" ) );
        g_csgo.m_weapon_cs_base_get_cs_wpn_data                   = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.weapon_cs_base.get_cs_wpn_data" ) );
        g_csgo.m_weapon_cs_base_get_spread                        = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.weapon_cs_base.get_spread" ) );
        g_csgo.m_weapon_cs_base_get_inaccuracy                    = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.weapon_cs_base.get_inaccuracy" ) );
        g_csgo.m_weapon_cs_base_update_accuracy_penalty           = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.weapon_cs_base.update_accuracy_penalty" ) );
        g_csgo.m_weapon_cs_base_is_full_auto                      = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.weapon_cs_base.is_full_auto" ) );
        g_csgo.m_engine_client_get_player_info                    = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.engine_client.get_player_info" ) );
        g_csgo.m_engine_client_get_player_for_user_id             = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.engine_client.get_player_for_user_id" ) );
        g_csgo.m_engine_client_get_local_player                   = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.engine_client.get_local_player" ) );
        g_csgo.m_engine_client_get_max_clients                    = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.engine_client.get_max_clients" ) );
        g_csgo.m_engine_client_is_in_game                         = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.engine_client.is_in_game" ) );
        g_csgo.m_engine_client_world_to_screen_matrix             = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.engine_client.world_to_screen_matrix" ) );
        g_csgo.m_engine_client_get_view_angles                    = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.engine_client.get_view_angles" ) );
        g_csgo.m_engine_client_get_net_channel_info               = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.engine_client.get_net_channel_info" ) );
        g_csgo.m_net_channel_get_latency                          = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.net_channel.get_latency" ) );
        g_csgo.m_net_channel_is_loopback                          = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.net_channel.is_loopback" ) );
        g_csgo.m_net_channel_is_timing_out                        = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.net_channel.is_timing_out" ) );
        g_csgo.m_net_channel_send_net_msg                         = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.net_channel.send_net_msg" ) );
        g_csgo.m_net_channel_set_choked                           = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.net_channel.set_choked" ) );
        g_csgo.m_net_channel_send_datagram                        = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.net_channel.send_datagram" ) );
        g_csgo.m_net_channel_can_packet                           = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.net_channel.can_packet" ) );
        g_csgo.m_demo_player_is_playing_back                      = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.demo_player.is_playing_back" ) );
        g_csgo.m_demo_recorder_record_user_input                  = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.demo_recorder.record_user_input" ) );
        g_csgo.m_demo_recorder_is_recording                       = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.demo_recorder.is_recording" ) );
        g_csgo.m_engine_trace_get_point_contents                  = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.engine_trace.get_point_contents" ) );
        g_csgo.m_engine_trace_clip_ray_to_entity                  = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.engine_trace.clip_ray_to_entity" ) );
        g_csgo.m_engine_trace_trace_ray                           = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.engine_trace.trace_ray" ) );
        g_csgo.m_physics_surface_props_get_surface_data           = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.physics_surface_props.get_surface_data" ) );
        g_csgo.m_model_info_get_model_index                       = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.model_info.get_model_index" ) );
        g_csgo.m_view_render_beams_create_beam_points             = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.view_render_beams.create_beam_points" ) );
        g_csgo.m_input_cam_is_third_person                        = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.input.cam_is_third_person" ) );
        g_csgo.m_input_cam_to_third_person                        = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.input.cam_to_third_person" ) );
        g_csgo.m_input_cam_to_first_person                        = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.input.cam_to_first_person" ) );
        g_csgo.m_prediction_get_local_view_angles                 = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.prediction.get_local_view_angles" ) );
        g_csgo.m_prediction_set_local_view_angles                 = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.prediction.set_local_view_angles" ) );
        g_csgo.m_prediction_run_command                           = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.prediction.run_command" ) );
        g_csgo.m_move_helper_client_set_host                      = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.move_helper_client.set_host" ) );
        g_csgo.m_cvar_find_var                                    = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.cvar.find_var" ) );
        g_csgo.m_convar_internal_set_int_value                    = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.convar.internal_set_int_value" ) );
        g_csgo.m_mat_system_surface_unlock_cursor                 = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.mat_system_surface.unlock_cursor" ) );
        g_csgo.m_mat_system_surface_play_sound                    = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.mat_system_surface.play_sound" ) );
        g_csgo.m_material_system_get_render_context               = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.material_system.get_render_context" ) );
        g_csgo.m_render_context_release                           = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.render_context.release" ) );
        g_csgo.m_render_context_get_call_queue                    = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.render_context.get_call_queue" ) );
        g_csgo.m_render_context_get_matrix                        = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.render_context.get_matrix" ) );
        g_csgo.m_call_queue_queue_functor_internal                = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.call_queue.queue_functor_internal" ) );
        g_csgo.m_mesh_handle_late_creation                        = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.mesh.handle_late_creation" ) );
        g_csgo.m_mdl_cache_get_studio_hdr                         = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.mdl_cache.get_studio_hdr" ) );
        g_csgo.m_mdl_cache_get_hardware_data                      = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.mdl_cache.get_hardware_data" ) );
        g_csgo.m_client_renderable_get_model                      = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.client_renderable.get_model" ) );
        g_csgo.m_client_renderable_setup_bones                    = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.client_renderable.setup_bones" ) );
        g_csgo.m_filesystem_open                                  = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.filesystem.open" ) );
        g_csgo.m_filesystem_size                                  = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.filesystem.size" ) );
        g_csgo.m_filesystem_read                                  = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.filesystem.read" ) );
        g_csgo.m_filesystem_close                                 = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.filesystem.close" ) );
        g_csgo.m_game_event_get_name                              = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.game_event.get_name" ) );
        g_csgo.m_game_event_get_int                               = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.game_event.get_int" ) );
        g_csgo.m_game_event_get_float                             = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.game_event.get_float" ) );
        g_csgo.m_game_event_get_string                            = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.game_event.get_string" ) );
        g_csgo.m_game_event_set_string                            = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.game_event.set_string" ) );
        g_csgo.m_game_event_manager_add_listener                  = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.game_event_manager.add_listener" ) );
        g_csgo.m_client_networkable_on_data_changed               = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.client_networkable.on_data_changed" ) );
        g_csgo.m_econ_wearable_equip                              = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.econ_wearable.equip" ) );
        g_csgo.m_custom_material_owner_copy_to                    = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.custom_material_owner.copy_to" ) );
        g_csgo.m_mem_alloc_alloc                                  = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.mem_alloc.alloc" ) );
        g_csgo.m_mem_alloc_realloc                                = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.mem_alloc.realloc" ) );
        g_csgo.m_mem_alloc_free                                   = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.mem_alloc.free" ) );

#if defined( ATLAS_DEV )
        g_csgo.m_base_animating_setup_bones = g_api.get_index_pkg( HASH_CT( "vfunc.cs2018.base_animating.setup_bones" ) );
#endif // ATLAS_DEV
    }

    {
        g_csgo.sv_airaccelerate                    = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "sv_airaccelerate" ).get( ) );
        g_csgo.sv_gravity                          = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "sv_gravity" ).get( ) );
        g_csgo.view_recoil_tracking                = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "view_recoil_tracking" ).get( ) );
        g_csgo.mat_postprocess_enable              = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "mat_postprocess_enable" ).get( ) );
        g_csgo.weapon_debug_spread_show            = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "weapon_debug_spread_show" ).get( ) );
        g_csgo.host_limitlocal                     = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "host_limitlocal" ).get( ) );
        g_csgo.developer                           = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "developer" ).get( ) );
        g_csgo.rate                                = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "rate" ).get( ) );
        g_csgo.cl_cmdrate                          = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "cl_cmdrate" ).get( ) );
        g_csgo.cl_updaterate                       = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "cl_updaterate" ).get( ) );
        g_csgo.cl_interp_ratio                     = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "cl_interp_ratio" ).get( ) );
        g_csgo.cl_interp                           = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "cl_interp" ).get( ) );
        g_csgo.cl_smooth                           = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "cl_smooth" ).get( ) );
        g_csgo.cl_pred_optimize                    = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "cl_pred_optimize" ).get( ) );
        g_csgo.sv_lan                              = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "sv_lan" ).get( ) );
        g_csgo.sv_maxcmdrate                       = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "sv_maxcmdrate" ).get( ) );
        g_csgo.sv_minupdaterate                    = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "sv_minupdaterate" ).get( ) );
        g_csgo.sv_maxupdaterate                    = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "sv_maxupdaterate" ).get( ) );
        g_csgo.sv_client_min_interp_ratio          = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "sv_client_min_interp_ratio" ).get( ) );
        g_csgo.sv_client_max_interp_ratio          = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "sv_client_max_interp_ratio" ).get( ) );
        g_csgo.sv_maxrate                          = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "sv_maxrate" ).get( ) );
        g_csgo.sv_minrate                          = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "sv_minrate" ).get( ) );
        g_csgo.mat_queue_mode                      = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "mat_queue_mode" ).get( ) );
        g_csgo.r_threaded_particles                = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "r_threaded_particles" ).get( ) );
        g_csgo.r_hunkalloclightmaps                = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "r_hunkalloclightmaps" ).get( ) );
        g_csgo.mat_forcemanagedtextureintohardware = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "mat_forcemanagedtextureintohardware" ).get( ) );
        g_csgo.r_drawbatchdecals                   = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "r_drawbatchdecals" ).get( ) );
        g_csgo.r_drawtracers_firstperson           = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "r_drawtracers_firstperson" ).get( ) );

        g_csgo.ff_damage_reduction_bullets           = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "ff_damage_reduction_bullets" ).get( ) );
        g_csgo.ff_damage_bullet_penetration          = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "ff_damage_bullet_penetration" ).get( ) );
        g_csgo.sv_clip_penetration_traces_to_players = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "sv_clip_penetration_traces_to_players" ).get( ) );
        g_csgo.mp_teammates_are_enemies              = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "mp_teammates_are_enemies" ).get( ) );
        g_csgo.mp_friendlyfire                       = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "mp_friendlyfire" ).get( ) );
        g_csgo.mp_damage_scale_ct_head               = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "mp_damage_scale_ct_head" ).get( ) );
        g_csgo.mp_damage_scale_ct_body               = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "mp_damage_scale_ct_body" ).get( ) );
        g_csgo.mp_damage_scale_t_head                = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "mp_damage_scale_t_head" ).get( ) );
        g_csgo.mp_damage_scale_t_body                = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "mp_damage_scale_t_body" ).get( ) );
        g_csgo.game_type                             = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "game_type" ).get( ) );

        g_csgo.weapon_accuracy_shotgun_spread_patterns = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "weapon_accuracy_shotgun_spread_patterns" ).get( ) );
        g_csgo.sv_friction                             = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "sv_friction" ).get( ) );
        g_csgo.sv_stopspeed                            = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "sv_stopspeed" ).get( ) );
        g_csgo.sv_accelerate                           = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "sv_accelerate" ).get( ) );
        g_csgo.sv_accelerate_use_weapon_speed          = g_csgo.m_cvar->FindVar( ATLAS_XORSTR( "sv_accelerate_use_weapon_speed" ).get( ) );

        if ( !g_csgo.sv_airaccelerate ||
             !g_csgo.sv_gravity ||
             !g_csgo.view_recoil_tracking ||
             !g_csgo.mat_postprocess_enable ||
             !g_csgo.weapon_debug_spread_show ||
             !g_csgo.host_limitlocal ||
             !g_csgo.developer ||
             !g_csgo.rate ||
             !g_csgo.cl_cmdrate ||
             !g_csgo.cl_updaterate ||
             !g_csgo.cl_interp_ratio ||
             !g_csgo.cl_interp ||
             !g_csgo.cl_smooth ||
             !g_csgo.cl_pred_optimize ||
             !g_csgo.sv_lan ||
             !g_csgo.sv_maxcmdrate ||
             !g_csgo.sv_minupdaterate ||
             !g_csgo.sv_maxupdaterate ||
             !g_csgo.sv_client_min_interp_ratio ||
             !g_csgo.sv_client_max_interp_ratio ||
             !g_csgo.sv_maxrate ||
             !g_csgo.sv_minrate ||
             !g_csgo.mat_queue_mode ||
             !g_csgo.r_threaded_particles ||
             !g_csgo.r_hunkalloclightmaps ||
             !g_csgo.mat_forcemanagedtextureintohardware ||
             !g_csgo.r_drawbatchdecals ||
             !g_csgo.r_drawtracers_firstperson ||
             !g_csgo.ff_damage_reduction_bullets ||
             !g_csgo.ff_damage_bullet_penetration ||
             !g_csgo.sv_clip_penetration_traces_to_players ||
             !g_csgo.mp_teammates_are_enemies ||
             !g_csgo.mp_friendlyfire ||
             !g_csgo.mp_damage_scale_ct_head ||
             !g_csgo.mp_damage_scale_ct_body ||
             !g_csgo.mp_damage_scale_t_head ||
             !g_csgo.mp_damage_scale_t_body ||
             !g_csgo.game_type ||
             !g_csgo.weapon_accuracy_shotgun_spread_patterns ||
             !g_csgo.sv_friction ||
             !g_csgo.sv_stopspeed ||
             !g_csgo.sv_accelerate ||
             !g_csgo.sv_accelerate_use_weapon_speed )
        {
            RAISE_ERROR( "Failed to acquire required game memory.\nThe cheat may be out of date." );
            return 1;
        }

        g_csgo.rate->SetValue( 786432 );
        g_csgo.cl_cmdrate->SetValue( 128 );
        g_csgo.cl_updaterate->SetValue( 128 );
        g_csgo.cl_interp_ratio->SetValue( 1 );
        g_csgo.cl_interp->SetValue( 0 );
        g_csgo.cl_smooth->SetValue( 0 );
        g_csgo.cl_pred_optimize->SetValue( 0 );

        g_csgo.sv_lan->SetValue( 1 );
        g_csgo.sv_maxcmdrate->SetValue( 128 );
        g_csgo.sv_maxupdaterate->SetValue( 128 );
        g_csgo.sv_client_min_interp_ratio->SetValue( -1 );
        g_csgo.sv_maxrate->SetValue( 0 );
        g_csgo.sv_minrate->SetValue( 80000 );

        g_csgo.mat_queue_mode->SetValue( 2 );
        g_csgo.r_threaded_particles->SetValue( 1 );

        g_csgo.r_hunkalloclightmaps->SetValue( 0 );

        g_csgo.mat_forcemanagedtextureintohardware->SetValue( 0 );
        g_csgo.r_drawbatchdecals->SetValue( 1 );

        g_csgo.r_drawtracers_firstperson->SetValue( 1 );

#if defined( ATLAS_DEV )
        g_csgo.developer->SetValue( 2 );
#endif // ATLAS_DEV
    }

    {
        if ( !g_netvars.load( ) )
        {
            RAISE_ERROR( "Failed to acquire required game memory.\nThe cheat may be out of date." );
            return 1;
        }

        if ( !g_datamap.init( ) )
        {
            RAISE_ERROR( "Failed to allocate required game memory.\nIf this error persists, please make a support ticket." );
            return 1;
        }

        if ( !g_skins.install_proxies( ) || !g_prediction.install_proxies( ) || !g_shot_log.install( ) || !g_compression.init( ) )
        {
            RAISE_ERROR( "Failed to acquire required game memory.\nThe cheat may be out of date." );
            return 1;
        }

        g_skins.request_game_tables( );
        g_nospread.init( );
    }

    {
        g_hooks.m_create_move                  = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.input.create_move" ), reinterpret_cast< void * >( &hkCreateMove ), g_csgo.m_input );
        g_hooks.m_frame_stage_notify           = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client.frame_stage_notify" ), reinterpret_cast< void * >( &hkFrameStageNotify ), g_csgo.m_client );
        g_hooks.m_level_init_pre_entity        = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client.level_init_pre_entity" ), reinterpret_cast< void * >( &hkLevelInitPreEntity ), g_csgo.m_client );
        g_hooks.m_paint                        = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.engine_vgui.paint" ), reinterpret_cast< void * >( &hkPaint ), g_csgo.m_vgui );
        g_hooks.m_override_view                = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client_mode.override_view" ), reinterpret_cast< void * >( &hkOverrideView ), g_csgo.m_client_mode );
        g_hooks.m_get_view_model_fov           = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client_mode.get_view_model_fov" ), reinterpret_cast< void * >( &hkGetViewModelFOV ), g_csgo.m_client_mode );
        g_hooks.m_do_post_screen_space_effects = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client_mode.do_post_screen_space_effects" ), reinterpret_cast< void * >( &hkDoPostScreenSpaceEffects ), g_csgo.m_client_mode );
        g_hooks.m_draw_model                   = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.studio_render.draw_model" ), reinterpret_cast< void * >( &hkDrawModel ), g_csgo.m_studio_render );
        g_hooks.m_set_cursor                   = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.surface.set_cursor" ), reinterpret_cast< void * >( &hkSetCursor ), g_csgo.m_surface );
        g_hooks.m_set_cursor_position          = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.surface.set_cursor_position" ), reinterpret_cast< void * >( &hkSetCursorPosition ), g_csgo.m_surface );
        g_hooks.m_lock_cursor                  = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.surface.lock_cursor" ), reinterpret_cast< void * >( &hkLockCursor ), g_csgo.m_surface );
        g_hooks.m_cl_move                      = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.engine.cl_move" ), reinterpret_cast< void * >( &hkCL_Move ) );
        g_hooks.m_packet_start                 = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.engine.packet_start" ), reinterpret_cast< void * >( &hkPacketStart ) );
        g_hooks.m_s_start_sound                = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.engine.s_start_sound" ), reinterpret_cast< void * >( &hkS_StartSound ) );
        g_hooks.m_post_think                   = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client.post_think" ), reinterpret_cast< void * >( &hkPostThink ) );
        g_hooks.m_check_init_predictable       = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client.check_init_predictable" ), reinterpret_cast< void * >( &hkCheckInitPredictable ) );
        g_hooks.m_run_simulation               = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client.run_simulation" ), reinterpret_cast< void * >( &hkRunSimulation ) );
        g_hooks.m_landing_dip                  = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client.landing_dip" ), reinterpret_cast< void * >( &hkLandingDip ) );
        g_hooks.m_hud_scope_paint              = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client.hud_scope_paint" ), reinterpret_cast< void * >( &hkHudScopePaint ) );
        g_hooks.m_calc_view_model_view         = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client.calc_view_model_view" ), reinterpret_cast< void * >( &hkCalcViewModelView ) );
        g_hooks.m_death_notice_player_death    = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client.death_notice_player_death" ), reinterpret_cast< void * >( &hkDeathNoticePlayerDeath ) );
        g_hooks.m_internal_draw_model          = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.studiorender.internal_draw_model" ), reinterpret_cast< void * >( &hkInternalDrawModel ) );
        g_hooks.m_get_client_model_renderable  = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client.get_client_model_renderable" ), reinterpret_cast< void * >( &hkGetClientModelRenderable ) );
        g_hooks.m_update_client_side_animation = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client.update_client_side_animation" ), reinterpret_cast< void * >( &hkUpdateClientSideAnimation ) );
        g_hooks.m_interpolate                  = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client.interpolate" ), reinterpret_cast< void * >( &hkInterpolate ) );
        g_hooks.m_setup_bones                  = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client.setup_bones" ), reinterpret_cast< void * >( &hkSetupBones ) );
        g_hooks.m_build_transformations        = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client.build_transformations" ), reinterpret_cast< void * >( &hkBuildTransformations ) );
        g_hooks.m_do_extra_bone_processing     = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client.do_extra_bone_processing" ), reinterpret_cast< void * >( &hkDoExtraBoneProcessing ) );
        g_hooks.m_estimate_abs_velocity        = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client.estimate_abs_velocity" ), reinterpret_cast< void * >( &hkEstimateAbsVelocity ) );
        g_hooks.m_check_for_layer_changes      = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client.check_for_layer_changes" ), reinterpret_cast< void * >( &hkCheckForLayerChanges ) );
        g_hooks.m_set_up_movement              = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client.anim_state_set_up_movement" ), reinterpret_cast< void * >( &hkSetUpMovement ) );
        g_hooks.m_check_jump_button            = g_api.get_hook_pkg( HASH_CT( "hook.cs2018.client.game_movement_check_jump_button" ), reinterpret_cast< void * >( &hkCheckJumpButton ) );

        if ( !g_hooks.m_create_move.enable( ) ||
             !g_hooks.m_frame_stage_notify.enable( ) ||
             !g_hooks.m_level_init_pre_entity.enable( ) ||
             !g_hooks.m_paint.enable( ) ||
             !g_hooks.m_override_view.enable( ) ||
             !g_hooks.m_get_view_model_fov.enable( ) ||
             !g_hooks.m_do_post_screen_space_effects.enable( ) ||
             !g_hooks.m_draw_model.enable( ) ||
             !g_hooks.m_set_cursor.enable( ) ||
             !g_hooks.m_set_cursor_position.enable( ) ||
             !g_hooks.m_lock_cursor.enable( ) ||
             !g_hooks.m_cl_move.enable( ) ||
             !g_hooks.m_packet_start.enable( ) ||
             !g_hooks.m_s_start_sound.enable( ) ||
             !g_hooks.m_post_think.enable( ) ||
             !g_hooks.m_check_init_predictable.enable( ) ||
             !g_hooks.m_run_simulation.enable( ) ||
             !g_hooks.m_landing_dip.enable( ) ||
             !g_hooks.m_hud_scope_paint.enable( ) ||
             !g_hooks.m_calc_view_model_view.enable( ) ||
             !g_hooks.m_death_notice_player_death.enable( ) ||
             !g_hooks.m_internal_draw_model.enable( ) ||
             !g_hooks.m_get_client_model_renderable.enable( ) ||
             !g_hooks.m_update_client_side_animation.enable( ) ||
             !g_hooks.m_interpolate.enable( ) ||
             !g_hooks.m_setup_bones.enable( ) ||
             !g_hooks.m_build_transformations.enable( ) ||
             !g_hooks.m_do_extra_bone_processing.enable( ) ||
             !g_hooks.m_estimate_abs_velocity.enable( ) ||
             !g_hooks.m_check_for_layer_changes.enable( ) ||
             !g_hooks.m_set_up_movement.enable( ) ||
             !g_hooks.m_check_jump_button.enable( ) )
        {
            RAISE_ERROR( "Failed to acquire required game memory.\nThe cheat may be out of date." );
            return 1;
        }
    }

    const bool pooled{ g_thread_pool.startup( atl::thread_pool::default_workers( ) ) };
    if ( !pooled && g_thread_pool.workers( ) != 0 )
    {
        RAISE_ERROR( "Failed to allocate required game memory." );
        return 1;
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
