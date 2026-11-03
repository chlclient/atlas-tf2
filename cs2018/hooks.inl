#pragma once

#include "features/datamaps.hpp"
#include "features/features.hpp"
#include "features/prediction.hpp"

i32 ATLAS_STDCALL hkPresent(
    void       *device,
    const void *src_rect,
    const void *dest_rect,
    const void *hwnd_override,
    const void *dirty_region )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_present.stdcall< i32 >( device, src_rect, dest_rect, hwnd_override, dirty_region );
    }

    if ( !g_input.window( ) )
    {
        g_input.attach_device( device );
    }

    g_cl.on_directx_frame( device );
    g_skins.service_game_tables( );

    if ( g_render.start_frame( device ) )
    {
        g_gui.draw_background( );

        g_input.begin_frame( );

        sync_menu_theme( );
        g_skins.sync_editor( );

        g_draw.traverse( );

        draw_notifications( g_cl.m_screen_width, g_cl.m_screen_height );
        if ( g_cfg.m_misc.m_draw_watermark )
        {
            draw_watermark( g_cl.m_screen_width, g_cl.m_screen_height, watermark_values( ) );
        }

        g_gui.think( g_cl.m_screen_width, g_cl.m_screen_height );
        g_gui.draw( );
        g_render.end_frame( );
    }

    g_input.end_frame( );

    return g_hooks.m_present.stdcall< i32 >( device, src_rect, dest_rect, hwnd_override, dirty_region );
}

i32 ATLAS_STDCALL hkReset(
    void                                *device,
    render::d3d9::D3DPRESENT_PARAMETERS *present_params )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_reset.stdcall< i32 >( device, present_params );
    }

    g_model_render.on_device_lost( );
    g_render.shutdown( );

    const auto status{ g_hooks.m_reset.stdcall< i32 >( device, present_params ) };
    if ( status >= render::d3d9::D3D_OK )
    {
        g_render.init( );
    }

    return status;
}

void ATLAS_THISCALL hkFrameStageNotify( CHLClient *thisptr, ClientFrameStage_t stage )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_frame_stage_notify.thiscall< void >( thisptr, stage );
    }

    if ( stage == FRAME_NET_UPDATE_POSTDATAUPDATE_END )
    {
        g_compression.restore( );
    }

    g_hooks.m_frame_stage_notify.thiscall< void >( thisptr, stage );

    if ( stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START )
    {
        g_skins.on_post_data_update_start( );
    }

    if ( stage == FRAME_NET_UPDATE_POSTDATAUPDATE_END )
    {
        g_cl.update_local_data( );
        g_player_data.update( );
        g_esp.think( );
        g_misc.on_post_data_update( );
        g_shot_log.on_post_data_update( );
    }

    if ( stage == FRAME_NET_UPDATE_END )
    {
        g_anims.restore_local( );
        g_anims.update_others( );
        g_lagcomp.update( );
        g_shot_log.on_net_update_end( );
        g_hitmarker.on_net_update_end( );
#if defined( ATLAS_DEV )
        g_server_hitboxes.check_lag_records( );
#endif // ATLAS_DEV
    }

    if ( stage == FRAME_RENDER_START )
    {
        g_input.seal_batch( );
        g_misc.on_render_start( );
        g_model_render.begin_frame( );
        g_chams.begin_frame( );
        g_skins.on_render_start( );
    }

    if ( stage == FRAME_RENDER_END )
    {
        g_model_render.end_frame( );
    }
}

void ATLAS_THISCALL hkLevelInitPreEntity( CHLClient *thisptr, const char *map_name )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_level_init_pre_entity.thiscall< void >( thisptr, map_name );
    }

    g_compression.clear( );

    g_hooks.m_level_init_pre_entity.thiscall< void >( thisptr, map_name );
}

void ATLAS_VECTORCALL hkCL_Move( float accumulated_extra_samples, bool final_tick )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_cl_move.vectorcall< void >( accumulated_extra_samples, final_tick );
    }

    g_networking.cl_move( accumulated_extra_samples, final_tick );
}

void ATLAS_THISCALL hkPacketStart( void *thisptr, i32 incoming_sequence, i32 outgoing_acknowledged )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_packet_start.thiscall< void >( thisptr, incoming_sequence, outgoing_acknowledged );
    }

    const i32 acknowledged{ g_networking.acknowledged( outgoing_acknowledged ) };

    g_compression.on_packet_start( acknowledged );

    g_hooks.m_packet_start.thiscall< void >( thisptr, incoming_sequence, acknowledged );
}

void ATLAS_THISCALL hkDeathNoticePlayerDeath( void *thisptr, IGameEvent *event )
{
    if ( g_cl.m_is_running )
    {
        g_skins.on_player_death( event );
    }

    g_hooks.m_death_notice_player_death.thiscall< void >( thisptr, event );
}

i32 ATLAS_FASTCALL hkS_StartSound( const StartSoundParams_t &params )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_s_start_sound.fastcall< i32 >( &params );
    }

    if ( !params.staticsound )
    {
        g_player_data.on_start_sound( params );
    }

    return g_hooks.m_s_start_sound.fastcall< i32 >( &params );
}

void ATLAS_THISCALL hkPaint( CEngineVGui *thisptr, i32 mode )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_paint.thiscall< void >( thisptr, mode );
    }

    g_hooks.m_paint.thiscall< void >( thisptr, mode );

    if ( mode & PAINT_UIPANELS )
    {
        g_cl.update_w2s( );
        g_esp.paint( );
        g_hitmarker.paint( );
        g_misc.paint( );
#if defined( ATLAS_DEV )
        g_server_hitboxes.paint( );
        g_lagcomp.paint( );
        g_autowall.paint( );
#endif // ATLAS_DEV
        g_draw.send( );
    }
}

void ATLAS_THISCALL hkCreateMove(
    CInput *thisptr,
    i32     sequence_number,
    float   input_sample_frametime,
    bool    active )
{
    g_hooks.m_create_move.thiscall< void >( thisptr, sequence_number, input_sample_frametime, active );

    if ( !g_cl.m_is_running )
    {
        return;
    }

    g_anims.on_create_move( sequence_number );
    g_input.begin_command( );

    g_prediction.update( );

    if ( !g_csgo.m_engine->IsInGame( ) ||
         !g_cl.m_local ||
         !g_cl.m_local->IsAlive( ) )
    {
        return;
    }

    g_cl.m_cmd = thisptr->GetUserCmd( sequence_number );

    g_misc.on_create_move( );

    const vec3 view_angles{ g_cl.m_cmd->viewangles };

    g_ragebot.begin_command( );

    g_prediction.start( );
    g_fakelag.think( );
    g_anti_aim.think( );
#if defined( ATLAS_DEV )
    g_autowall.think( );
#endif // ATLAS_DEV
    g_prediction.end( );

    g_ragebot.end_command( );

    if ( g_cl.m_cmd->viewangles != view_angles )
    {
        g_cl.fix_movement( g_cl.m_cmd, view_angles );
    }

    g_misc.leg_movement( );

    CVerifiedUserCmd *verified{ thisptr->GetVerifiedUserCmd( sequence_number ) };
    verified->m_cmd = *g_cl.m_cmd;
    verified->m_crc = g_cl.m_cmd->GetChecksum( );
}

void ATLAS_THISCALL hkPostThink( C_BasePlayer *thisptr )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_post_think.thiscall< void >( thisptr );
    }

    if ( g_prediction.m_is_active )
    {
        g_ragebot.think( );
        g_prediction.post_think( );
        return;
    }

    g_hooks.m_post_think.thiscall< void >( thisptr );
    g_anims.update_local( thisptr->As< C_CSPlayer * >( ) );
}

bool ATLAS_THISCALL hkCheckJumpButton( CGameMovement *thisptr )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_check_jump_button.thiscall< bool >( thisptr );
    }

    const bool jumped{ g_hooks.m_check_jump_button.thiscall< bool >( thisptr ) };
    if ( jumped && g_csgo.m_prediction->m_bFirstTimePredicted )
    {
        g_anims.on_jump( );
    }

    return jumped;
}

void ATLAS_THISCALL hkCheckInitPredictable( C_BaseEntity *thisptr, const char *context )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_check_init_predictable.thiscall< void >( thisptr, context );
    }

    g_datamap.on_check_init_predictable( );

    g_hooks.m_check_init_predictable.thiscall< void >( thisptr, context );
}

void ATLAS_VECTORCALL hkRunSimulation(
    CPrediction  *thisptr,
    i32           edx,
    float         xmm0,
    float         xmm1,
    float         curtime,
    i32           current_command,
    CUserCmd     *cmd,
    C_BasePlayer *local_player )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_run_simulation.vectorcall< void >( thisptr, edx, xmm0, xmm1, curtime, current_command, cmd, local_player );
    }

    local_player->m_vecPreviouslyPredictedOrigin( ) = local_player->m_vecOrigin( );

    C_BaseCombatWeapon *weapon{ local_player->m_hActiveWeapon( ).Get( ) };
    if ( !weapon )
    {
        g_hooks.m_run_simulation.vectorcall< void >( thisptr, edx, xmm0, xmm1, curtime, current_command, cmd, local_player );
        g_compression.on_run_simulation( local_player, current_command );
        return;
    }

    const i32   activity{ weapon->m_Activity( ) };
    const float last_shot{ weapon->m_fLastShotTime( ) };
    const i32   clip{ weapon->m_iClip1( ) };

    g_hooks.m_run_simulation.vectorcall< void >( thisptr, edx, xmm0, xmm1, curtime, current_command, cmd, local_player );
    g_compression.on_run_simulation( local_player, current_command );

    if ( local_player->m_hActiveWeapon( ).Get( ) != weapon )
    {
        return;
    }

    g_ragebot.on_simulated( weapon, activity, curtime );

    if ( thisptr->m_bFirstTimePredicted )
    {
        g_shot_log.on_command_simulated( current_command, curtime, local_player->As< C_CSPlayer * >( ), weapon, last_shot, clip );
    }
}

void ATLAS_THISCALL hkOverrideView( ClientModeShared *thisptr, CViewSetup *setup )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_override_view.thiscall< void >( thisptr, setup );
    }

    const vec3 eye{ setup->origin };
    g_hooks.m_override_view.thiscall< void >( thisptr, setup );

    g_misc.on_override_view( setup, eye );
}

void ATLAS_THISCALL hkLandingDip( C_CSPlayer *thisptr, vec3 &eye_origin )
{
    if ( g_cl.m_is_running && g_cfg.m_misc.m_removals[ config::REMOVAL_LANDING_BOB ] )
    {
        return;
    }

    g_hooks.m_landing_dip.thiscall< void >( thisptr, &eye_origin );
}

void ATLAS_THISCALL hkHudScopePaint( void *thisptr )
{
    if ( g_cl.m_is_running && g_cfg.m_misc.m_removals[ config::REMOVAL_SCOPE_OVERLAY ] )
    {
        return;
    }

    g_hooks.m_hud_scope_paint.thiscall< void >( thisptr );
}

void ATLAS_THISCALL hkCalcViewModelView( C_BaseEntity *thisptr, C_CSPlayer *owner, vec3 &eye_position, vec3 &eye_angles )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_calc_view_model_view.thiscall< void >( thisptr, owner, &eye_position, &eye_angles );
    }

    vec3 position{ eye_position };
    vec3 angles{ eye_angles };

    g_viewmodel.on_calc_view( owner, position, angles );

    g_hooks.m_calc_view_model_view.thiscall< void >( thisptr, owner, &position, &angles );
}

float ATLAS_THISCALL hkGetViewModelFOV( ClientModeShared *thisptr )
{
    const float original{ g_hooks.m_get_view_model_fov.thiscall< float >( thisptr ) };

    if ( !g_cl.m_is_running )
    {
        return original;
    }

    return g_viewmodel.view_model_fov( original );
}

void ATLAS_THISCALL hkSetCursor( CMatSystemSurface *thisptr, vgui::HCursor hCursor )
{
    if ( !g_cl.m_is_running || !g_gui.open( ) )
    {
        return g_hooks.m_set_cursor.thiscall< void >( thisptr, hCursor );
    }

    switch ( g_gui.get_cursor_state( ) )
    {
        case gui::CURSOR_STATE_ARROW: hCursor = vgui::dc_arrow; break;
        case gui::CURSOR_STATE_CLICK_HAND: hCursor = vgui::dc_hand; break;
        case gui::CURSOR_STATE_TEXT_BEAM: hCursor = vgui::dc_ibeam; break;
        case gui::CURSOR_STATE_RESIZE_NESW: hCursor = vgui::dc_sizenesw; break;
        case gui::CURSOR_STATE_RESIZE_NWSE: hCursor = vgui::dc_sizenwse; break;
        default: hCursor = vgui::dc_arrow; break;
    }

    g_hooks.m_set_cursor.thiscall< void >( thisptr, hCursor );
}

void ATLAS_THISCALL hkLockCursor( CMatSystemSurface *thisptr )
{
    if ( !g_cl.m_is_running || !g_gui.open( ) )
    {
        return g_hooks.m_lock_cursor.thiscall< void >( thisptr );
    }

    g_csgo.m_surface->UnlockCursor( );
}

void ATLAS_THISCALL hkSetCursorPosition( CMatSystemSurface *thisptr, i32 x, i32 y )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_set_cursor_position.thiscall< void >( thisptr, x, y );
    }

    if ( g_gui.open( ) )
    {
        return;
    }

    g_hooks.m_set_cursor_position.thiscall< void >( thisptr, x, y );
}

void ATLAS_THISCALL hkDrawModel(
    CStudioRenderContext *thisptr,
    DrawModelResults_t   *results,
    DrawModelInfo_t      *info,
    matrix3x4            *bone_to_world,
    float                *flex_weights,
    float                *flex_delayed_weights,
    const vec3           *model_origin,
    i32                   flags )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_draw_model.thiscall< void >( thisptr, results, info, bone_to_world, flex_weights, flex_delayed_weights, model_origin, flags );
    }

    if ( !g_chams.on_draw_model( info, bone_to_world, flags ) )
    {
        return;
    }

    g_hooks.m_draw_model.thiscall< void >( thisptr, results, info, bone_to_world, flex_weights, flex_delayed_weights, model_origin, flags );
}

void ATLAS_THISCALL hkInternalDrawModel(
    CStudioRender               *thisptr,
    const DrawModelInfo_t       &info,
    const StudioRenderContext_t &context,
    matrix3x4                   *bone_to_world,
    const FlexWeights_t         &flex,
    i32                          flags )
{
    const i32 engine_flags{ flags & ~model_render::ROUTE_MASK };

    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_internal_draw_model.thiscall< void >( thisptr, &info, &context, bone_to_world, &flex, engine_flags );
    }

    if ( ( flags & model_render::ROUTE_TAKEN_OVER ) && g_model_render.draw_in_place( info, flags ) )
    {
        return;
    }

    g_hooks.m_internal_draw_model.thiscall< void >( thisptr, &info, &context, bone_to_world, &flex, engine_flags );

    if ( flags & model_render::ROUTE_ENGINE_BASE )
    {
        g_model_render.draw_in_place( info, flags );
    }
}

bool ATLAS_THISCALL hkDoPostScreenSpaceEffects( ClientModeShared *thisptr, const CViewSetup *setup )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_do_post_screen_space_effects.thiscall< bool >( thisptr, setup );
    }

    g_model_render.end_scene( );
    g_chams.render_main( );

    return g_hooks.m_do_post_screen_space_effects.thiscall< bool >( thisptr, setup );
}

void *ATLAS_THISCALL hkGetClientModelRenderable( IClientRenderable *thisptr )
{
    void *renderable{ g_hooks.m_get_client_model_renderable.thiscall< void * >( thisptr ) };

    if ( !g_cl.m_is_running || !renderable )
    {
        return renderable;
    }

    return g_chams.wants( thisptr->GetIClientUnknown( )->GetBaseEntity( ) ) ? nullptr : renderable;
}

void ATLAS_THISCALL hkUpdateClientSideAnimation( C_CSPlayer *thisptr )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_update_client_side_animation.thiscall< void >( thisptr );
    }

    thisptr->m_PlayerAnimState( )->m_nLastUpdateFrame( ) = g_csgo.m_globals->framecount;
    g_hooks.m_update_client_side_animation.thiscall< void >( thisptr );
}

bool ATLAS_THISCALL hkInterpolate( C_CSPlayer *thisptr, float current_time )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_interpolate.thiscall< bool >( thisptr, current_time );
    }

    return true;
}

bool ATLAS_THISCALL hkSetupBones(
    IClientRenderable *thisptr,
    matrix3x4         *bone_to_world,
    i32                max_bones,
    i32                bone_mask,
    float              current_time )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_setup_bones.thiscall< bool >( thisptr, bone_to_world, max_bones, bone_mask, current_time );
    }

    if ( g_bone_mgr.player_setup( thisptr->GetIClientUnknown( )->GetBaseEntity( )->As< C_CSPlayer * >( ), bone_to_world, max_bones ) )
    {
        return true;
    }

    return g_hooks.m_setup_bones.thiscall< bool >( thisptr, bone_to_world, max_bones, bone_mask, current_time );
}

void ATLAS_THISCALL hkBuildTransformations(
    C_CSPlayer      *thisptr,
    CStudioHdr      *studio,
    vec3            *pos,
    quaternion      *q,
    const matrix3x4 *camera_transform,
    i32              bone_mask,
    void            *bone_computed )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_build_transformations.thiscall< void >( thisptr, studio, pos, q, camera_transform, bone_mask, bone_computed );
    }

    g_csgo.m_C_BaseFlex_BuildTransformations
        .get_as< void( ATLAS_THISCALL * )( C_CSPlayer *, CStudioHdr *, vec3 *, quaternion *, const matrix3x4 *, i32, void * ) >( )(
            thisptr, studio, pos, q, camera_transform, bone_mask, bone_computed );
}

void ATLAS_THISCALL hkDoExtraBoneProcessing(
    C_CSPlayer *thisptr,
    CStudioHdr *studio,
    vec3       *pos,
    quaternion *q,
    matrix3x4  *bone_to_world,
    void       *bone_computed,
    void       *ik_context )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_do_extra_bone_processing.thiscall< void >( thisptr, studio, pos, q, bone_to_world, bone_computed, ik_context );
    }
}

void ATLAS_VECTORCALL hkCheckForLayerChanges( C_BaseEntity *thisptr, i32 edx, float xmm0, float xmm1, float current_time, CStudioHdr *studio )
{
    if ( !g_cl.m_is_running || !thisptr->IsPlayer( ) )
    {
        return g_hooks.m_check_for_layer_changes.vectorcall< void >( thisptr, edx, xmm0, xmm1, current_time, studio );
    }
}

void ATLAS_THISCALL hkEstimateAbsVelocity( C_BaseEntity *thisptr, vec3 *velocity )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_estimate_abs_velocity.thiscall< void >( thisptr, velocity );
    }

    if ( g_anims.replay_velocity( thisptr, *velocity ) )
    {
        return;
    }

    g_hooks.m_estimate_abs_velocity.thiscall< void >( thisptr, velocity );
}

void ATLAS_THISCALL hkSetUpMovement( CCSGOPlayerAnimState *thisptr )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_set_up_movement.thiscall< void >( thisptr );
    }

    g_anims.set_up_movement( thisptr );
    g_hooks.m_set_up_movement.thiscall< void >( thisptr );
}
