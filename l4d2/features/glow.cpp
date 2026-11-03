#include "glow.hpp"

ATLAS_NO_DESTROY glow g_glow{ };

constexpr ShaderStencilState_t MARK_SILHOUETTE{ .m_bEnable         = true,
                                                .m_ZFailOp         = STENCILOPERATION_REPLACE,
                                                .m_PassOp          = STENCILOPERATION_REPLACE,
                                                .m_nReferenceValue = 1 };
constexpr ShaderStencilState_t OUTSIDE_SILHOUETTE{ .m_bEnable     = true,
                                                   .m_CompareFunc = STENCILCOMPARISONFUNCTION_EQUAL,
                                                   .m_nWriteMask  = 0 };

ATLAS_FORCE_INLINE const config::glow_profile_t &profile_for( usize target )
{
    if ( target >= config::TARGET_COMMON )
    {
        return g_cfg.m_glow.m_infected[ target - config::TARGET_COMMON ];
    }

    return g_cfg.m_glow.m_players[ target ];
}

ATLAS_FORCE_INLINE void draw_screen( IMatRenderContext *render_context,
                                     IMaterial         *material,
                                     i32                x,
                                     i32                y,
                                     i32                width,
                                     i32                height )
{
    render_context->DrawScreenSpaceRectangle( material, x, y, width, height,
                                              0.0f, 0.0f, static_cast< float >( width - 1 ), static_cast< float >( height - 1 ),
                                              width, height );
}

void glow::render( IMatRenderContext *render_context, i32 width, i32 height )
{
    this->collect( );

    if ( !this->m_draw_count )
    {
        return;
    }

    if ( !this->m_halo )
    {
        this->create_materials( );
    }

    const float old_blend{ g_l4d2.m_render_view->GetBlend( ) };
    float       old_color[ 3 ]{ };
    g_l4d2.m_render_view->GetColorModulation( old_color );

    this->m_driving = true;
    g_hooks.m_forced_material_override.thiscall< void >( g_l4d2.m_studio_render, this->m_glow_color, OVERRIDE_NORMAL );

    this->mask_pass( render_context );

    for ( usize target{ }; target < config::TARGET_COUNT; ++target )
    {
        if ( this->m_has_target[ target ] )
        {
            this->color_pass( render_context, target, width, height );
        }
    }

    g_hooks.m_forced_material_override.thiscall< void >( g_l4d2.m_studio_render, static_cast< IMaterial * >( nullptr ), OVERRIDE_NORMAL );
    this->m_driving = false;

    g_l4d2.m_render_view->SetBlend( old_blend );
    g_l4d2.m_render_view->SetColorModulation( old_color );
}

void glow::collect( )
{
    this->m_draw_count = 0;

    bool active{ };
    for ( usize target{ }; target < config::TARGET_COUNT; ++target )
    {
        this->m_has_target[ target ] = false;
        active |= !profile_for( target ).off( );
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
        if ( target == config::TARGET_COUNT || profile_for( target ).off( ) )
        {
            continue;
        }

        this->m_draws[ this->m_draw_count++ ] = draw_t{ entity, target };
        this->m_has_target[ target ]          = true;
    }
}

void glow::create_materials( )
{
    this->m_glow_color = g_l4d2.m_material_system->FindMaterial( ATLAS_XORSTR( "dev/glow_color" ).get( ), ATLAS_XORSTR( "Other textures" ).get( ) );
    this->m_glow_color->IncrementReferenceCount( );

    this->m_buffer1 = g_l4d2.m_material_system->FindTexture( ATLAS_XORSTR( "_rt_FullFrameFB" ).get( ), ATLAS_XORSTR( "RenderTargets" ).get( ) );
    this->m_buffer2 = g_l4d2.m_material_system->FindTexture( ATLAS_XORSTR( "_rt_FullScreen" ).get( ), ATLAS_XORSTR( "RenderTargets" ).get( ) );

    KeyValues *blur_x{ new KeyValues( ATLAS_XORSTR( "BlurFilterX" ).get( ) ) };
    blur_x->SetString( ATLAS_XORSTR( "$basetexture" ).get( ), ATLAS_XORSTR( "_rt_FullFrameFB" ).get( ) );
    blur_x->SetInt( ATLAS_XORSTR( "$ignorez" ).get( ), 1 );
    this->m_blur_x = g_l4d2.m_material_system->CreateMaterial( ATLAS_XORSTR( "atlas_glow_blur_x" ).get( ), blur_x );

    KeyValues *blur_y{ new KeyValues( ATLAS_XORSTR( "BlurFilterY" ).get( ) ) };
    blur_y->SetString( ATLAS_XORSTR( "$basetexture" ).get( ), ATLAS_XORSTR( "_rt_FullScreen" ).get( ) );
    blur_y->SetInt( ATLAS_XORSTR( "$bloomamount" ).get( ), 2 );
    blur_y->SetInt( ATLAS_XORSTR( "$ignorez" ).get( ), 1 );
    this->m_blur_y = g_l4d2.m_material_system->CreateMaterial( ATLAS_XORSTR( "atlas_glow_blur_y" ).get( ), blur_y );

    KeyValues *halo{ new KeyValues( ATLAS_XORSTR( "UnlitGeneric" ).get( ) ) };
    halo->SetString( ATLAS_XORSTR( "$basetexture" ).get( ), ATLAS_XORSTR( "_rt_FullFrameFB" ).get( ) );
    halo->SetInt( ATLAS_XORSTR( "$additive" ).get( ), 1 );
    halo->SetInt( ATLAS_XORSTR( "$ignorez" ).get( ), 1 );
    this->m_halo = g_l4d2.m_material_system->CreateMaterial( ATLAS_XORSTR( "atlas_glow_halo" ).get( ), halo );
}

void glow::set_modulation( const color_t &color ) const
{
    const float modulation[ 3 ]{
        static_cast< float >( color.r ) / 255.0f,
        static_cast< float >( color.g ) / 255.0f,
        static_cast< float >( color.b ) / 255.0f,
    };

    g_l4d2.m_render_view->SetColorModulation( modulation );
    g_l4d2.m_render_view->SetBlend( static_cast< float >( color.a ) / 255.0f );
}

void glow::draw_target( usize target ) const
{
    for ( usize at{ }; at < this->m_draw_count; ++at )
    {
        if ( this->m_draws[ at ].m_target == target )
        {
            this->m_draws[ at ].m_entity->GetClientRenderable( )->DrawModel( STUDIO_RENDER | STUDIO_DONOTMODIFYSTENCILSTATE, RenderableInstance_t{ } );
        }
    }
}

void glow::mask_pass( IMatRenderContext *render_context ) const
{
    render_context->ClearBuffers( false, false, true );
    render_context->SetStencilState( MARK_SILHOUETTE );

    this->set_modulation( color_t{ 255, 255, 255, 0 } );

    for ( usize at{ }; at < this->m_draw_count; ++at )
    {
        this->m_draws[ at ].m_entity->GetClientRenderable( )->DrawModel( STUDIO_RENDER | STUDIO_DONOTMODIFYSTENCILSTATE, RenderableInstance_t{ } );
    }

    render_context->SetStencilState( ShaderStencilState_t{ } );
}

void glow::color_pass( IMatRenderContext *render_context, usize target, i32 width, i32 height ) const
{
    const config::glow_profile_t &profile{ profile_for( target ) };

    render_context->PushRenderTargetAndViewport( );
    render_context->SetRenderTarget( this->m_buffer1 );
    render_context->Viewport( 0, 0, width, height );
    render_context->ClearColor4ub( 0, 0, 0, 0 );
    render_context->ClearBuffers( true, false, false );

    this->set_modulation( profile.m_color );
    this->draw_target( target );

    render_context->PopRenderTargetAndViewport( );

    if ( profile.m_blur )
    {
        render_context->PushRenderTargetAndViewport( );
        render_context->Viewport( 0, 0, width, height );
        render_context->SetRenderTarget( this->m_buffer2 );
        draw_screen( render_context, this->m_blur_x, 0, 0, width, height );
        render_context->SetRenderTarget( this->m_buffer1 );
        draw_screen( render_context, this->m_blur_y, 0, 0, width, height );
        render_context->PopRenderTargetAndViewport( );
    }

    render_context->SetStencilState( OUTSIDE_SILHOUETTE );

    if ( profile.m_stencil > 0 )
    {
        const i32 side{ ( profile.m_stencil + 1 ) / 2 };

        draw_screen( render_context, this->m_halo, -side, 0, width, height );
        draw_screen( render_context, this->m_halo, 0, -side, width, height );
        draw_screen( render_context, this->m_halo, side, 0, width, height );
        draw_screen( render_context, this->m_halo, 0, side, width, height );

        if ( const i32 corner{ profile.m_stencil / 2 } )
        {
            draw_screen( render_context, this->m_halo, -corner, -corner, width, height );
            draw_screen( render_context, this->m_halo, corner, corner, width, height );
            draw_screen( render_context, this->m_halo, corner, -corner, width, height );
            draw_screen( render_context, this->m_halo, -corner, corner, width, height );
        }
    }

    if ( profile.m_blur )
    {
        draw_screen( render_context, this->m_halo, 0, 0, width, height );
    }

    render_context->SetStencilState( ShaderStencilState_t{ } );
}
