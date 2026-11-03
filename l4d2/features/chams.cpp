#include "chams.hpp"

ATLAS_NO_DESTROY chams g_chams{ };

constexpr ShaderStencilState_t MARK_VISIBLE{ .m_bEnable         = true,
                                             .m_PassOp          = STENCILOPERATION_REPLACE,
                                             .m_nReferenceValue = 1 };
constexpr ShaderStencilState_t UNMARKED_ONLY{ .m_bEnable     = true,
                                              .m_CompareFunc = STENCILCOMPARISONFUNCTION_EQUAL,
                                              .m_nWriteMask  = 0 };

ATLAS_FORCE_INLINE const config::cham_profile_t &profile_for( usize target )
{
    if ( target >= config::TARGET_COMMON )
    {
        return g_cfg.m_chams.m_infected[ target - config::TARGET_COMMON ];
    }

    return g_cfg.m_esp.m_profiles[ target ].m_chams;
}

ATLAS_FORCE_INLINE KeyValues *build_flat( )
{
    KeyValues *kv{ new KeyValues( ATLAS_XORSTR( "UnlitGeneric" ).get( ) ) };
    kv->SetString( ATLAS_XORSTR( "$basetexture" ).get( ), ATLAS_XORSTR( "vgui/white" ).get( ) );
    kv->SetInt( ATLAS_XORSTR( "$nodecal" ).get( ), 1 );
    return kv;
}

ATLAS_FORCE_INLINE KeyValues *build_wireframe( )
{
    KeyValues *kv{ new KeyValues( ATLAS_XORSTR( "UnlitGeneric" ).get( ) ) };
    kv->SetString( ATLAS_XORSTR( "$basetexture" ).get( ), ATLAS_XORSTR( "vgui/white" ).get( ) );
    kv->SetInt( ATLAS_XORSTR( "$wireframe" ).get( ), 1 );
    kv->SetInt( ATLAS_XORSTR( "$nodecal" ).get( ), 1 );
    return kv;
}

void chams::render( IMatRenderContext *render_context )
{
    this->collect( );

    if ( !this->m_draw_count )
    {
        return;
    }

    if ( !this->m_flat )
    {
        this->create_materials( );
    }

    const float old_blend{ g_l4d2.m_render_view->GetBlend( ) };
    float       old_color[ 3 ]{ };
    g_l4d2.m_render_view->GetColorModulation( old_color );

    this->m_driving = true;

    if ( this->m_has_occluded )
    {
        render_context->ClearBuffers( false, false, true );
        render_context->SetStencilState( MARK_VISIBLE );
    }

    this->run_sweep( false );

    if ( this->m_has_occluded )
    {
        render_context->SetStencilState( UNMARKED_ONLY );
        render_context->DepthRange( 0.0f, 0.2f );
        this->run_sweep( true );
        render_context->DepthRange( 0.0f, 1.0f );
        render_context->SetStencilState( ShaderStencilState_t{ } );
    }

    g_hooks.m_forced_material_override.thiscall< void >( g_l4d2.m_studio_render, static_cast< IMaterial * >( nullptr ), OVERRIDE_NORMAL );
    this->m_driving = false;

    g_l4d2.m_render_view->SetBlend( old_blend );
    g_l4d2.m_render_view->SetColorModulation( old_color );
}

bool chams::drawn( i32 index ) const
{
    if ( index <= 0 || index >= ( 1 << MAX_EDICT_BITS ) )
    {
        return false;
    }

    return ( this->m_drawn[ index / 64 ] & ( 1ull << ( index % 64 ) ) ) != 0;
}

void chams::on_level_shutdown( )
{
    this->m_draw_count = 0;

    for ( u64 &word : this->m_drawn )
    {
        word = 0;
    }
}

void chams::collect( )
{
    this->m_draw_count   = 0;
    this->m_has_occluded = false;

    for ( u64 &word : this->m_drawn )
    {
        word = 0;
    }

    bool active{ };
    for ( const config::esp_profile_t &profile : g_cfg.m_esp.m_profiles )
    {
        active |= !profile.m_chams.off( );
    }

    for ( const config::cham_profile_t &profile : g_cfg.m_chams.m_infected )
    {
        active |= !profile.off( );
    }

    if ( !active )
    {
        return;
    }

    const i32 highest{ g_l4d2.m_entity_list->GetHighestEntityIndex( ) };
    for ( i32 index{ 1 }; index <= highest; ++index )
    {
        IClientEntity *client_entity{ g_l4d2.m_entity_list->GetClientEntity( index ) };
        if ( !client_entity )
        {
            continue;
        }

        auto *entity{ client_entity->As< C_BaseEntity * >( ) };
        if ( entity->IsDormant( ) )
        {
            continue;
        }

        const usize target{ g_cl.render_target( entity ) };
        if ( target == config::TARGET_COUNT )
        {
            continue;
        }

        const config::cham_profile_t &style{ profile_for( target ) };
        if ( style.off( ) )
        {
            continue;
        }

        this->m_draws[ this->m_draw_count++ ] = draw_t{ entity, target };
        this->m_has_occluded |= style.m_invisible != CHAM_OFF || style.m_invisible_overlay != CHAM_OFF;
    }
}

void chams::create_materials( )
{
    this->m_flat      = g_l4d2.m_material_system->CreateMaterial( ATLAS_XORSTR( "atlas_cham_flat" ).get( ), build_flat( ) );
    this->m_wireframe = g_l4d2.m_material_system->CreateMaterial( ATLAS_XORSTR( "atlas_cham_wireframe" ).get( ), build_wireframe( ) );
}

IMaterial *chams::resolve_material( usize mode ) const
{
    switch ( mode )
    {
        case CHAM_FLAT: return this->m_flat;
        case CHAM_WIREFRAME: return this->m_wireframe;
        default: return nullptr;
    }
}

void chams::setup_pass( IMaterial *material, const color_t &color ) const
{
    const float modulation[ 3 ]{
        static_cast< float >( color.r ) / 255.0f,
        static_cast< float >( color.g ) / 255.0f,
        static_cast< float >( color.b ) / 255.0f,
    };

    g_l4d2.m_render_view->SetColorModulation( modulation );
    g_l4d2.m_render_view->SetBlend( static_cast< float >( color.a ) / 255.0f );
    g_hooks.m_forced_material_override.thiscall< void >( g_l4d2.m_studio_render, material, OVERRIDE_NORMAL );
}

void chams::draw_layer( C_BaseEntity *entity, usize mode, const color_t &color )
{
    if ( mode == CHAM_OFF )
    {
        return;
    }

    if ( mode == CHAM_ORIGINAL )
    {
        this->setup_pass( nullptr, color_t{ 255, 255, 255, 255 } );
    } else
    {
        this->setup_pass( this->resolve_material( mode ), color );
    }

    if ( entity->GetClientRenderable( )->DrawModel( STUDIO_RENDER | STUDIO_DONOTMODIFYSTENCILSTATE, RenderableInstance_t{ } ) != 0 )
    {
        this->mark_drawn( entity->entindex( ) );
    }
}

void chams::run_sweep( bool occluded )
{
    for ( usize at{ }; at < this->m_draw_count; ++at )
    {
        const draw_t                 &draw{ this->m_draws[ at ] };
        const config::cham_profile_t &style{ profile_for( draw.m_target ) };
        usize                         base{ occluded ? style.m_invisible : style.m_visible };
        const usize                   overlay{ occluded ? style.m_invisible_overlay : style.m_visible_overlay };

        if ( !occluded && base == CHAM_OFF )
        {
            base = CHAM_ORIGINAL;
        }

        if ( base == CHAM_OFF && overlay == CHAM_OFF )
        {
            continue;
        }

        this->draw_layer( draw.m_entity, base, occluded ? style.m_occluded_color : style.m_visible_color );
        this->draw_layer( draw.m_entity, overlay, occluded ? style.m_occluded_overlay_color : style.m_visible_overlay_color );
    }
}

void chams::mark_drawn( i32 index )
{
    if ( index <= 0 || index >= ( 1 << MAX_EDICT_BITS ) )
    {
        return;
    }

    this->m_drawn[ index / 64 ] |= 1ull << ( index % 64 );
}
