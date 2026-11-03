#include "chams.hpp"

ATLAS_NO_DESTROY chams g_chams{ };

ATLAS_FORCE_INLINE model_render::shade_t shade_of( usize mode )
{
    switch ( mode )
    {
        case chams::CHAM_SHADED: return model_render::SHADE_SHADED;
        case chams::CHAM_FRESNEL: return model_render::SHADE_FRESNEL;
        case chams::CHAM_LAVA: return model_render::SHADE_LAVA;
        case chams::CHAM_PEARL: return model_render::SHADE_PEARL;
        case chams::CHAM_BUBBLE: return model_render::SHADE_BUBBLE;
        case chams::CHAM_LSD: return model_render::SHADE_LSD;
        case chams::CHAM_NETWORK: return model_render::SHADE_NETWORK;
        case chams::CHAM_SOAP: return model_render::SHADE_SOAP;
        case chams::CHAM_AURORA: return model_render::SHADE_AURORA;
        case chams::CHAM_AQUARIUM: return model_render::SHADE_AQUARIUM;
        case chams::CHAM_SKY: return model_render::SHADE_SKY;
        default: return model_render::SHADE_FLAT;
    }
}

ATLAS_FORCE_INLINE bool is_wireframe( usize mode )
{
    return mode == chams::CHAM_WIREFRAME || mode == chams::CHAM_NETWORK;
}

void chams::begin_frame( )
{
    this->m_active       = false;
    this->m_routed_count = 0;

    for ( usize &target : this->m_targets )
    {
        target = TARGET_NONE;
    }

    for ( C_CSPlayer *&player : this->m_players )
    {
        player = nullptr;
    }

    if ( !g_cl.m_local || !g_csgo.m_engine->IsInGame( ) )
    {
        return;
    }

    const i32 max_clients{ math::smaller( g_csgo.m_engine->GetMaxClients( ), MAX_PLAYERS ) };
    const i32 local_team{ g_cl.m_local->m_iTeamNum( ) };

    for ( i32 index{ 1 }; index <= max_clients; ++index )
    {
        IClientEntity *entity{ g_csgo.m_entity_list->GetClientEntity( index ) };
        if ( !entity )
        {
            continue;
        }

        auto     *player{ entity->As< C_CSPlayer * >( ) };
        const i32 team{ player->m_iTeamNum( ) };

        if ( player == g_cl.m_local ||
             player->IsDormant( ) ||
             !player->IsAlive( ) ||
             team < TEAM_TERRORIST )
        {
            continue;
        }

        const usize target{ static_cast< usize >( team == local_team ? config::esp_target_t::team : config::esp_target_t::enemy ) };

        if ( g_cfg.m_chams.m_profiles[ target ].off( ) && g_cfg.m_glow.m_players[ target ].off( ) )
        {
            continue;
        }

        this->m_players[ index ] = player;
        this->m_targets[ index ] = target;
        this->m_active           = true;
    }
}

bool chams::wants( C_BaseEntity *entity ) const
{
    return this->m_active && this->target_for( entity ) != TARGET_NONE;
}

usize chams::target_for( C_BaseEntity *entity ) const
{
    for ( i32 depth{ }; depth < 4 && entity; ++depth )
    {
        const i32 index{ entity->entindex( ) };
        if ( index > 0 && index <= MAX_PLAYERS )
        {
            return this->m_targets[ index ];
        }

        entity = entity->m_pMoveParent( ).Get( );
    }

    return TARGET_NONE;
}

chams::routed_t *chams::find_routed( C_BaseEntity *entity )
{
    for ( usize at{ }; at < this->m_routed_count; ++at )
    {
        if ( this->m_routed[ at ].m_entity == entity )
        {
            return &this->m_routed[ at ];
        }
    }

    return nullptr;
}

bool chams::on_draw_model( const DrawModelInfo_t *info, const matrix3x4 *bone_to_world, i32 &flags )
{
    if ( !this->m_active || !info || !info->m_pClientEntity ||
         ( flags & ( STUDIORENDER_SHADOWDEPTHTEXTURE | STUDIORENDER_SSAODEPTHTEXTURE ) ) )
    {
        return true;
    }

    C_BaseEntity *entity{ static_cast< IClientRenderable * >( info->m_pClientEntity )->GetIClientUnknown( )->GetBaseEntity( ) };
    if ( !entity )
    {
        return true;
    }

    const usize target{ this->target_for( entity ) };
    if ( target == TARGET_NONE )
    {
        return true;
    }

    const config::cham_profile_t &profile{ g_cfg.m_chams.m_profiles[ target ] };
    routed_t                     *routed{ this->find_routed( entity ) };

    if ( ( flags & STUDIORENDER_DRAW_GROUP_MASK ) == STUDIORENDER_DRAW_TRANSLUCENT_ONLY )
    {
        return profile.m_visible == CHAM_OFF || !routed;
    }

    const bool engine_base{ profile.m_visible == CHAM_OFF };

    u32 instance{ };
    if ( !g_model_render.take_over( *info, bone_to_world, engine_base, flags, instance ) )
    {
        return true;
    }

    if ( !engine_base )
    {
        g_model_render.add_in_place( instance, profile.m_visible_color, shade_of( profile.m_visible ), is_wireframe( profile.m_visible ) );
    }

    if ( profile.m_visible_overlay != CHAM_OFF )
    {
        g_model_render.add_in_place( instance, profile.m_visible_overlay_color, shade_of( profile.m_visible_overlay ),
                                     is_wireframe( profile.m_visible_overlay ) );
    }

    if ( routed )
    {
        routed->m_instance = instance;
    } else if ( this->m_routed_count < model_render::MAX_INSTANCES )
    {
        this->m_routed[ this->m_routed_count++ ] = routed_t{ entity, instance, target };
    }

    return true;
}

void chams::add_culled( i32 index )
{
    const usize                   target{ this->m_targets[ index ] };
    const config::cham_profile_t &profile{ g_cfg.m_chams.m_profiles[ target ] };

    if ( ( profile.m_invisible == CHAM_OFF && profile.m_invisible_overlay == CHAM_OFF && g_cfg.m_glow.m_players[ target ].off( ) ) ||
         this->m_routed_count == model_render::MAX_INSTANCES )
    {
        return;
    }

    C_CSPlayer    *player{ this->m_players[ index ] };
    const model_t *model{ player->GetModel( ) };
    if ( !model || model->type != MOD_STUDIO )
    {
        return;
    }

    const studiohdr_t    *header{ g_csgo.m_mdl_cache->GetStudioHdr( model->studio ) };
    const studiohwdata_t *hardware{ g_csgo.m_mdl_cache->GetHardwareData( model->studio ) };

    u32 instance{ };
    if ( !header || !hardware || header->numbones > MAXSTUDIOBONES ||
         !player->SetupBones( this->m_bones, MAXSTUDIOBONES, BONE_USED_BY_VERTEX_LOD0 << hardware->m_RootLOD, g_csgo.m_globals->curtime ) ||
         !g_model_render.add_detached( header, hardware, player->m_nBody( ), this->m_bones, instance ) )
    {
        return;
    }

    this->m_routed[ this->m_routed_count++ ] = routed_t{ player, instance, target };
}

void chams::render_main( )
{
    if ( !this->m_active || !g_model_render.active( ) )
    {
        return;
    }

    for ( i32 index{ 1 }; index <= MAX_PLAYERS; ++index )
    {
        if ( this->m_targets[ index ] != TARGET_NONE && !this->find_routed( this->m_players[ index ] ) )
        {
            this->add_culled( index );
        }
    }

    if ( !this->m_routed_count )
    {
        return;
    }

    for ( usize at{ }; at < this->m_routed_count; ++at )
    {
        const routed_t               &routed{ this->m_routed[ at ] };
        const config::cham_profile_t &profile{ g_cfg.m_chams.m_profiles[ routed.m_target ] };
        const config::glow_profile_t &glow{ g_cfg.m_glow.m_players[ routed.m_target ] };

        if ( profile.m_invisible != CHAM_OFF )
        {
            g_model_render.add_layer( model_render::LAYER_OCCLUDED_BASE, routed.m_instance, profile.m_occluded_color, shade_of( profile.m_invisible ),
                                      is_wireframe( profile.m_invisible ) );
        }

        if ( profile.m_invisible_overlay != CHAM_OFF )
        {
            g_model_render.add_layer( model_render::LAYER_OCCLUDED_OVERLAY, routed.m_instance, profile.m_occluded_overlay_color,
                                      shade_of( profile.m_invisible_overlay ), is_wireframe( profile.m_invisible_overlay ) );
        }

        if ( !glow.off( ) )
        {
            g_model_render.add_glow( routed.m_instance, glow );
        }
    }

    IMatRenderContext *render_context{ g_csgo.m_material_system->GetRenderContext( ) };
    g_model_render.submit( render_context );
    g_model_render.submit_glow( render_context );
    render_context->Release( );
}
