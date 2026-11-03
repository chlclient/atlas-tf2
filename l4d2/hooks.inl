#pragma once

#include "features/chams.hpp"
#include "features/datamaps.hpp"
#include "features/esp.hpp"
#include "features/glow.hpp"
#include "features/hitscan.hpp"
#include "features/lagcomp.hpp"
#include "features/misc.hpp"
#include "features/networking.hpp"
#include "features/nospread.hpp"
#include "features/prediction.hpp"
#include "features/viewmodel.hpp"

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

    render::d3d9::D3DVIEWPORT9 viewport{ };
    render::d3d9::device9{ device }.GetViewport( &viewport );

    const i32 screen_width{ static_cast< i32 >( viewport.width ) };
    const i32 screen_height{ static_cast< i32 >( viewport.height ) };

    if ( g_render.start_frame( device ) )
    {
        g_gui.draw_background( );

        g_input.begin_frame( );

        g_draw.traverse( );

        draw_notifications( screen_width, screen_height );

        g_gui.think( screen_width, screen_height );
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

    g_render.shutdown( );

    const auto status{ g_hooks.m_reset.stdcall< i32 >( device, present_params ) };
    if ( status >= render::d3d9::D3D_OK )
    {
        g_render.init( );
    }

    return status;
}

void ATLAS_THISCALL hkLevelShutdown( CHLClient *thisptr )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_level_shutdown.thiscall< void >( thisptr );
    }

    g_prediction.shutdown( );
    g_lagcomp.clear( );
    g_nospread.clear( );
    g_chams.on_level_shutdown( );

    g_cl.m_local  = nullptr;
    g_cl.m_weapon = nullptr;

    g_hooks.m_level_shutdown.thiscall< void >( thisptr );
}

void ATLAS_THISCALL hkFrameStageNotify( CHLClient *thisptr, ClientFrameStage_t stage )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_frame_stage_notify.thiscall< void >( thisptr, stage );
    }

    g_hooks.m_frame_stage_notify.thiscall< void >( thisptr, stage );

    switch ( stage )
    {
        case FRAME_NET_UPDATE_POSTDATAUPDATE_END:
        {
            g_cl.update_local_data( );
            g_lagcomp.update( );
            break;
        }

        default: break;
    }
}

void ATLAS_CDECL hkCL_Move( float accumulated_extra_samples, bool final_tick )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_cl_move.ccall< void >( accumulated_extra_samples, final_tick );
    }

    g_networking.remove_input_delay( final_tick );
    g_networking.cl_move( accumulated_extra_samples, final_tick );
}

void ATLAS_CDECL hkCL_ReadPackets( bool final_tick )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_cl_read_packets.ccall< void >( final_tick );
    }

    if ( !g_networking.should_read_packets( ) )
    {
        return;
    }

    g_hooks.m_cl_read_packets.ccall< void >( final_tick );
}

CUserCmd *ATLAS_THISCALL hkGetUserCmd(
    CInput *thisptr,
    i32     slot,
    i32     sequence_number )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_get_usercmd.thiscall< CUserCmd * >( thisptr, slot, sequence_number );
    }

    return thisptr->GetUserCmd( slot == -1 ? g_l4d2.m_engine->GetActiveSplitScreenPlayerSlot( ) : slot, sequence_number );
}

void ATLAS_THISCALL hkCreateMove(
    CHLClient *thisptr,
    i32        sequence_number,
    float      input_sample_frametime,
    bool       active )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_create_move.thiscall< void >( thisptr, sequence_number, input_sample_frametime, active );
    }

    g_hooks.m_create_move.thiscall< void >( thisptr, sequence_number, input_sample_frametime, active );

    g_prediction.update( );

    if ( !g_l4d2.m_engine->IsInGame( ) ||
         !g_cl.m_local ||
         !g_cl.m_local->IsAlive( ) )
    {
        return;
    }

    const i32 slot{ g_l4d2.m_engine->GetActiveSplitScreenPlayerSlot( ) };

    g_cl.m_cmd = g_l4d2.m_input->GetUserCmd( slot, sequence_number );

    g_misc.on_create_move( );

    g_nospread.pre_predict( );

    if ( g_prediction.start( ) )
    {
        g_nospread.post_predict( );
    }

    g_prediction.end( );

    CVerifiedUserCmd *verified{ g_l4d2.m_input->GetVerifiedUserCmd( slot, sequence_number ) };
    verified->m_cmd = *g_cl.m_cmd;
    verified->m_crc = g_cl.m_cmd->GetChecksum( );
}

void ATLAS_THISCALL hkOverrideView( ClientModeShared *thisptr, CViewSetup *setup )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_override_view.thiscall< void >( thisptr, setup );
    }

    g_hooks.m_override_view.thiscall< void >( thisptr, setup );

    g_misc.on_override_view( setup );

    g_cl.m_view_fov = setup->fov;
}

void ATLAS_THISCALL hkDoPostScreenSpaceEffects( ClientModeShared *thisptr, const CViewSetup *setup )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_do_post_screen_space_effects.thiscall< void >( thisptr, setup );
    }

    if ( g_l4d2.m_engine->IsInGame( ) && g_cl.m_local )
    {
        IMatRenderContext *render_context{ g_l4d2.m_material_system->GetRenderContext( ) };
        render_context->BeginRender( );

        g_chams.render( render_context );
        g_glow.render( render_context, setup->width, setup->height );

        render_context->EndRender( );
        render_context->Release( );
    }

    g_hooks.m_do_post_screen_space_effects.thiscall< void >( thisptr, setup );
}

void ATLAS_THISCALL hkDrawModelExecute(
    CModelRender            *thisptr,
    const DrawModelState_t  &state,
    const ModelRenderInfo_t &info,
    matrix3x4               *bone_to_world )
{
    if ( !g_cl.m_is_running ||
         g_chams.m_driving ||
         g_glow.m_driving ||
         ( info.flags & STUDIO_SHADOWDEPTHTEXTURE ) ||
         !g_chams.drawn( info.entity_index ) )
    {
        return g_hooks.m_draw_model_execute.thiscall< void >( thisptr, &state, &info, bone_to_world );
    }
}

void *ATLAS_THISCALL hkGetClientModelRenderable( C_BaseEntity *thisptr )
{
    if ( !g_cl.m_is_running || !g_chams.drawn( thisptr->entindex( ) ) )
    {
        return g_hooks.m_get_client_model_renderable.thiscall< void * >( thisptr );
    }

    return nullptr;
}

void ATLAS_THISCALL hkForcedMaterialOverride( CStudioRender *thisptr, IMaterial *material, OverrideType_t override_type )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_forced_material_override.thiscall< void >( thisptr, material, override_type );
    }

    if ( g_chams.m_driving || g_glow.m_driving )
    {
        return;
    }

    g_hooks.m_forced_material_override.thiscall< void >( thisptr, material, override_type );
}

void ATLAS_THISCALL hkCalcViewModelView(
    C_BaseEntity *thisptr,
    C_BaseEntity *owner,
    vec3         &eye_position,
    vec3         &eye_angles )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_calc_view_model_view.thiscall< void >( thisptr, owner, &eye_position, &eye_angles );
    }

    vec3 position{ eye_position };
    vec3 angles{ eye_angles };

    g_viewmodel.on_calc_view( position, angles );

    g_hooks.m_calc_view_model_view.thiscall< void >( thisptr, owner, &position, &angles );
}

void ATLAS_CDECL hkAvoidPushawayProps( C_BasePlayer *player, CUserCmd *cmd )
{
    if ( !g_cl.m_is_running || !g_cfg.m_misc.m_removals[ config::REMOVAL_PUSH ] )
    {
        return g_hooks.m_avoid_pushaway_props.ccall< void >( player, cmd );
    }
}

void ATLAS_THISCALL hkPostThink( C_BasePlayer *thisptr )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_post_think.thiscall< void >( thisptr );
    }

    if ( g_prediction.m_is_active )
    {
        g_prediction.post_think( );
        return;
    }

    g_hooks.m_post_think.thiscall< void >( thisptr );
}

ATLAS_FORCE_INLINE void gun_item_post_frame( api_hook &original, C_TerrorGun *gun )
{
    if ( !g_cl.m_is_running ||
         !g_prediction.m_is_active ||
         gun != g_cl.m_weapon ||
         !g_prediction.begin_weapon( ) )
    {
        return original.thiscall< void >( gun );
    }

    const i32  clip{ gun->m_iClip1( ) };
    const bool aimed{ g_hitscan.think( gun ) };

    original.thiscall< void >( gun );

    if ( aimed )
    {
        g_hitscan.commit( gun->m_iClip1( ) < clip );
    }
}

void ATLAS_THISCALL hkWeaponItemPostFrame( C_TerrorGun *thisptr )
{
    gun_item_post_frame( g_hooks.m_item_post_frame, thisptr );
}

void ATLAS_THISCALL hkBurstItemPostFrame( C_TerrorGun *thisptr )
{
    gun_item_post_frame( g_hooks.m_burst_post_frame, thisptr );
}

void ATLAS_THISCALL hkPaint( CEngineVGui *thisptr, i32 mode )
{
    if ( !g_cl.m_is_running )
    {
        return g_hooks.m_paint.thiscall< void >( thisptr, mode );
    }

    g_hooks.m_paint.thiscall< void >( thisptr, mode );

    if ( mode == PAINT_INGAMEPANELS )
    {
        g_cl.update_w2s( );
        g_esp.paint( );
        g_hitscan.paint( );
        g_draw.send( );
        return;
    }

    // out of game V_RenderView only paints UI panels, so an empty list is sent here
    // to clear what the last map left on screen..
    if ( !g_l4d2.m_engine->IsInGame( ) )
    {
        g_draw.send( );
    }
}

void ATLAS_THISCALL hkSetCursor( CMatSystemSurface *thisptr, vgui::HCursor hCursor )
{
    if ( !g_cl.m_is_running || !g_gui.open( ) )
    {
        return g_hooks.m_set_cursor.thiscall< void >( thisptr, hCursor );
    }

    g_hooks.m_set_cursor.thiscall< void >( thisptr, vgui::dc_arrow );
}

void ATLAS_THISCALL hkLockCursor( CMatSystemSurface *thisptr )
{
    if ( !g_cl.m_is_running || !g_gui.open( ) )
    {
        return g_hooks.m_lock_cursor.thiscall< void >( thisptr );
    }

    g_l4d2.m_surface->UnlockCursor( );
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
