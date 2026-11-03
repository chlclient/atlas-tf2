#include "model_render.hpp"

#include "render/shaders/glow.hpp"
#include "render/shaders/model.hpp"

ATLAS_NO_DESTROY model_render g_model_render{ };

ATLAS_NO_DESTROY model_render::job_t model_render::s_jobs[ 4 ]{ };

constexpr u32 VIEW_PROJECTION_REGISTER{ 0 };
constexpr u32 WEIGHT_DECODE_REGISTER{ 4 };
constexpr u32 EYE_REGISTER{ 5 };
constexpr u32 PALETTE_REGISTER{ 8 };
constexpr u32 VERTEX_REGISTER_COUNT{ PALETTE_REGISTER + 3 * MAX_HARDWARE_BONES };

constexpr float CLEARED_PALETTE[ 3 * MAX_HARDWARE_BONES ][ 4 ]{ };

constexpr u32 PIXEL_REGISTER_COUNT{ 2 };
constexpr u32 TIME_REGISTER{ 1 };

constexpr float DECAL_SLOPE_SCALE_BIAS{ 1.0f / -0.5f };
constexpr float DECAL_DEPTH_BIAS{ 1.0f / -262144.0f };

constexpr f64 TIME_WRAP{ 1024.0 };

constexpr float GLOW_BLUR_SPACING{ 2.5f };

constexpr const u32 *PIXEL_SHADERS[ model_render::SHADE_COUNT ]{
    render::shaders::MODEL_FLAT_PIXEL_SHADER,
    render::shaders::MODEL_SHADED_PIXEL_SHADER,
    render::shaders::MODEL_FRESNEL_PIXEL_SHADER,
    render::shaders::MODEL_LAVA_PIXEL_SHADER,
    render::shaders::MODEL_PEARL_PIXEL_SHADER,
    render::shaders::MODEL_BUBBLE_PIXEL_SHADER,
    render::shaders::MODEL_LSD_PIXEL_SHADER,
    render::shaders::MODEL_NETWORK_PIXEL_SHADER,
    render::shaders::MODEL_SOAP_PIXEL_SHADER,
    render::shaders::MODEL_AURORA_PIXEL_SHADER,
    render::shaders::MODEL_AQUARIUM_PIXEL_SHADER,
    render::shaders::MODEL_SKY_PIXEL_SHADER,
};

ATLAS_FORCE_INLINE usize layout_index( u64 format )
{
    return ( ( format & VERTEX_FORMAT_COMPRESSED ) ? 1 : 0 ) | ( ( format & VERTEX_BONE_WEIGHT_MASK ) ? 2 : 0 ) |
           ( ( format & VERTEX_BONE_INDEX ) ? 4 : 0 ) | ( ( format & VERTEX_NORMAL ) ? 8 : 0 ) | ( ( format & VERTEX_FORMAT_PAD_POS_NORM ) ? 16 : 0 );
}

ATLAS_FORCE_INLINE void layout_elements( usize index, render::d3d9::D3DVERTEXELEMENT9 ( &elements )[ 5 ] )
{
    const bool compressed{ ( index & 1 ) != 0 };
    const bool padded{ ( index & 16 ) != 0 };
    usize      count{ };
    u16        offset{ static_cast< u16 >( padded ? 16 : 12 ) };

    elements[ count++ ] = { 0, 0, render::d3d9::D3DDECLTYPE_FLOAT3, 0, render::d3d9::D3DDECLUSAGE_POSITION, 0 };

    if ( index & 2 )
    {
        elements[ count++ ] = { 0, offset, compressed ? render::d3d9::D3DDECLTYPE_SHORT2 : render::d3d9::D3DDECLTYPE_FLOAT2, 0,
                                render::d3d9::D3DDECLUSAGE_BLENDWEIGHT, 0 };
        offset += compressed ? 4 : 8;
    }

    if ( index & 4 )
    {
        elements[ count++ ] = { 0, offset, render::d3d9::D3DDECLTYPE_D3DCOLOR, 0, render::d3d9::D3DDECLUSAGE_BLENDINDICES, 0 };
        offset += 4;
    }

    if ( index & 8 )
    {
        elements[ count++ ] = { 0, offset, compressed && !padded ? render::d3d9::D3DDECLTYPE_UBYTE4 : render::d3d9::D3DDECLTYPE_FLOAT3, 0,
                                render::d3d9::D3DDECLUSAGE_NORMAL, 0 };
    }

    elements[ count ] = render::d3d9::D3DDECL_END;
}

constexpr u32 RECORDED_STATES[]{
    render::d3d9::D3DRS_ZENABLE,
    render::d3d9::D3DRS_ZFUNC,
    render::d3d9::D3DRS_ZWRITEENABLE,
    render::d3d9::D3DRS_CULLMODE,
    render::d3d9::D3DRS_FILLMODE,
    render::d3d9::D3DRS_ALPHATESTENABLE,
    render::d3d9::D3DRS_ALPHABLENDENABLE,
    render::d3d9::D3DRS_SRCBLEND,
    render::d3d9::D3DRS_DESTBLEND,
    render::d3d9::D3DRS_BLENDOP,
    render::d3d9::D3DRS_FOGENABLE,
    render::d3d9::D3DRS_SCISSORTESTENABLE,
    render::d3d9::D3DRS_CLIPPLANEENABLE,
    render::d3d9::D3DRS_SRGBWRITEENABLE,
    render::d3d9::D3DRS_COLORWRITEENABLE,
    render::d3d9::D3DRS_DEPTHBIAS,
    render::d3d9::D3DRS_SLOPESCALEDEPTHBIAS,
    render::d3d9::D3DRS_STENCILENABLE,
    render::d3d9::D3DRS_STENCILFUNC,
    render::d3d9::D3DRS_STENCILREF,
    render::d3d9::D3DRS_STENCILPASS,
    render::d3d9::D3DRS_STENCILFAIL,
    render::d3d9::D3DRS_STENCILZFAIL,
    render::d3d9::D3DRS_STENCILMASK,
    render::d3d9::D3DRS_STENCILWRITEMASK,
};

constexpr u32 RECORDED_SAMPLER_STATES[]{
    render::d3d9::D3DSAMP_ADDRESSU,
    render::d3d9::D3DSAMP_ADDRESSV,
    render::d3d9::D3DSAMP_MAGFILTER,
    render::d3d9::D3DSAMP_MINFILTER,
    render::d3d9::D3DSAMP_MIPFILTER,
    render::d3d9::D3DSAMP_SRGBTEXTURE,
};

template< typename F >
ATLAS_FORCE_INLINE bool for_each_group( const studiohdr_t    *header,
                                        const studiohwdata_t *hardware,
                                        i32                   body,
                                        i32                   lod,
                                        F                   &&visit )
{
    const studioloddata_t &lod_data{ hardware->m_pLODs[ lod ] };

    for ( i32 part{ }; part < header->numbodyparts; ++part )
    {
        const mstudiobodyparts_t *bodypart{ header->pBodypart( part ) };
        const mstudiomodel_t     *model{ bodypart->pModel( body / bodypart->base % bodypart->nummodels ) };

        for ( i32 index{ }; index < model->nummeshes; ++index )
        {
            const studiomeshdata_t &mesh{ lod_data.m_pMeshData[ model->pMesh( index )->meshid ] };

            for ( i32 group{ }; group < mesh.m_NumGroup; ++group )
            {
                if ( !visit( mesh.m_pMeshGroup[ group ] ) )
                {
                    return false;
                }
            }
        }
    }

    return true;
}

ATLAS_FORCE_INLINE void view_projection( IMatRenderContext *render_context, VMatrix &out, float ( &eye )[ 4 ] )
{
    VMatrix view{ };
    VMatrix projection{ };
    render_context->GetMatrix( MATERIAL_VIEW, &view );
    render_context->GetMatrix( MATERIAL_PROJECTION, &projection );

    for ( i32 row{ }; row < 4; ++row )
    {
        for ( i32 column{ }; column < 4; ++column )
        {
            out[ row ][ column ] = projection[ row ][ 0 ] * view[ 0 ][ column ] + projection[ row ][ 1 ] * view[ 1 ][ column ] +
                                   projection[ row ][ 2 ] * view[ 2 ][ column ] + projection[ row ][ 3 ] * view[ 3 ][ column ];
        }
    }

    for ( i32 axis{ }; axis < 3; ++axis )
    {
        eye[ axis ] = -( view[ 0 ][ axis ] * view[ 0 ][ 3 ] + view[ 1 ][ axis ] * view[ 1 ][ 3 ] + view[ 2 ][ axis ] * view[ 2 ][ 3 ] );
    }

    eye[ 3 ] = 1.0f;
}

ATLAS_FORCE_INLINE void set_decal_bias( const render::d3d9::device9 &device, bool decal )
{
    device.SetRenderState( render::d3d9::D3DRS_SLOPESCALEDEPTHBIAS, decal ? __builtin_bit_cast( u32, DECAL_SLOPE_SCALE_BIAS ) : 0 );
    device.SetRenderState( render::d3d9::D3DRS_DEPTHBIAS, decal ? __builtin_bit_cast( u32, DECAL_DEPTH_BIAS ) : 0 );
}

ATLAS_FORCE_INLINE void set_stencil_state( const render::d3d9::device9 &device, bool occluded )
{
    set_decal_bias( device, !occluded );

    device.SetRenderState( render::d3d9::D3DRS_ZFUNC, occluded ? render::d3d9::D3DCMP_GREATER : render::d3d9::D3DCMP_LESSEQUAL );
    device.SetRenderState( render::d3d9::D3DRS_ZWRITEENABLE, 0 );
    device.SetRenderState( render::d3d9::D3DRS_STENCILFUNC, occluded ? render::d3d9::D3DCMP_EQUAL : render::d3d9::D3DCMP_ALWAYS );
    device.SetRenderState( render::d3d9::D3DRS_STENCILREF, occluded ? 0 : 1 );
    device.SetRenderState( render::d3d9::D3DRS_STENCILPASS, occluded ? render::d3d9::D3DSTENCILOP_KEEP : render::d3d9::D3DSTENCILOP_REPLACE );
    device.SetRenderState( render::d3d9::D3DRS_STENCILFAIL, render::d3d9::D3DSTENCILOP_KEEP );
    device.SetRenderState( render::d3d9::D3DRS_STENCILZFAIL, render::d3d9::D3DSTENCILOP_KEEP );
    device.SetRenderState( render::d3d9::D3DRS_STENCILWRITEMASK, occluded ? 0x00 : 0xFF );
}

i32 model_render::phase_t::AddRef( )
{
    return _InterlockedIncrement( &this->m_job->m_references );
}

i32 model_render::phase_t::Release( )
{
    return _InterlockedDecrement( &this->m_job->m_references );
}

void model_render::phase_t::operator( )( )
{
    if ( this->m_run )
    {
        ( this->m_owner->*this->m_run )( *this->m_job );
    }
}

void model_render::begin_frame( )
{
    if ( this->m_job )
    {
        this->end_frame( );
    }

    this->m_routing = true;

    usize index{ };
    while ( index < 4 && !s_jobs[ index ].acquire( ) )
    {
        ++index;
    }

    if ( index == 4 )
    {
        return;
    }

    const f64 seconds{ g_csgo.m_globals->realtime };

    job_t &job{ s_jobs[ index ] };
    job.m_clear_stencil  = false;
    job.m_time           = static_cast< float >( seconds - static_cast< f64 >( static_cast< i64 >( seconds / TIME_WRAP ) ) * TIME_WRAP );
    job.m_instance_count = 0;
    job.m_pose_count     = 0;
    job.m_glow_count     = 0;

    for ( usize &count : job.m_layer_counts )
    {
        count = 0;
    }

    this->m_layers_phases[ index ].m_owner = this;
    this->m_layers_phases[ index ].m_job   = &job;
    this->m_layers_phases[ index ].m_run   = &model_render::draw;
    this->m_glow_phases[ index ].m_owner   = this;
    this->m_glow_phases[ index ].m_job     = &job;
    this->m_glow_phases[ index ].m_run     = &model_render::draw_glow;
    this->m_end_phases[ index ].m_owner    = this;
    this->m_end_phases[ index ].m_job      = &job;
    this->m_job                            = &job;
}

void model_render::end_frame( )
{
    if ( !this->m_job )
    {
        return;
    }

    IMatRenderContext *render_context{ g_csgo.m_material_system->GetRenderContext( ) };
    queue( render_context, this->m_end_phases[ this->m_job - s_jobs ] );
    render_context->Release( );

    _InterlockedDecrement( &this->m_job->m_references );
    this->m_job = nullptr;
}

bool model_render::take_over( const DrawModelInfo_t &info, const matrix3x4 *bone_to_world, bool engine_base, i32 &flags, u32 &instance )
{
    if ( !this->m_job || !this->m_routing || !bone_to_world || !info.m_pStudioHdr || !info.m_pHardwareData ||
         !this->push_instance( info.m_pStudioHdr, info.m_pHardwareData, info.m_Body, bone_to_world, instance ) )
    {
        return false;
    }

    flags |= ( engine_base ? ROUTE_ENGINE_BASE : ROUTE_TAKEN_OVER ) | static_cast< i32 >( this->m_job - s_jobs ) << ROUTE_JOB_SHIFT |
             static_cast< i32 >( instance ) << ROUTE_INSTANCE_SHIFT;
    return true;
}

bool model_render::add_detached( const studiohdr_t *header, const studiohwdata_t *hardware, i32 body, const matrix3x4 *bone_to_world, u32 &instance )
{
    if ( !this->m_job || !this->push_instance( header, hardware, body, bone_to_world, instance ) )
    {
        return false;
    }

    instance_t &entry{ this->m_job->m_instances[ instance ] };
    entry.m_drawn    = true;
    entry.m_detached = true;
    return true;
}

bool model_render::push_instance( const studiohdr_t    *header,
                                  const studiohwdata_t *hardware,
                                  i32                   body,
                                  const matrix3x4      *bone_to_world,
                                  u32                  &instance )
{
    job_t      &job{ *this->m_job };
    const usize bone_count{ static_cast< usize >( header->numbones ) };

    if ( job.m_instance_count == MAX_INSTANCES || job.m_pose_count + bone_count > MAX_POSES || hardware->m_NumLODs <= 0 )
    {
        return false;
    }

    const bool hardware_skinned{ for_each_group( header, hardware, body, hardware->m_RootLOD, []( const studiomeshgroup_t &group )
    {
        return ( group.m_Flags & MESHGROUP_IS_HWSKINNED ) != 0;
    } ) };

    if ( !hardware_skinned )
    {
        return false;
    }

    const usize first_pose{ job.m_pose_count };

    for ( usize bone{ }; bone < bone_count; ++bone )
    {
        math::concat_transforms( bone_to_world[ bone ], header->pBone( static_cast< i32 >( bone ) )->poseToBone,
                                 job.m_poses[ first_pose + bone ].m_matrix );
    }

    if ( header->flags & STUDIOHDR_FLAGS_STATIC_PROP )
    {
        atl::memcpy( job.m_poses[ first_pose ].m_matrix, bone_to_world[ 0 ], sizeof( matrix3x4 ) );
    }

    job.m_pose_count += bone_count;

    instance_t entry{ };
    entry.m_header     = header;
    entry.m_hardware   = hardware;
    entry.m_body       = body;
    entry.m_lod        = hardware->m_RootLOD;
    entry.m_first_pose = static_cast< u32 >( first_pose );

    instance                    = static_cast< u32 >( job.m_instance_count++ );
    job.m_instances[ instance ] = entry;
    return true;
}

void model_render::add_in_place( u32 instance, const color_t &color, shade_t shade, bool wireframe )
{
    instance_t &entry{ this->m_job->m_instances[ instance ] };
    Assert( entry.m_surface_count < 2 );

    entry.m_surfaces[ entry.m_surface_count++ ] = make_surface( color, shade, wireframe );
    entry.m_lit |= shade != SHADE_FLAT;
}

void model_render::add_layer( usize layer, u32 instance, const color_t &color, shade_t shade, bool wireframe )
{
    Assert( this->m_job != nullptr && layer < LAYER_COUNT );

    job_t &job{ *this->m_job };
    usize &count{ job.m_layer_counts[ layer ] };

    if ( count == MAX_LAYERS )
    {
        return;
    }

    job.m_layers[ layer ][ count++ ] = layer_t{ instance, make_surface( color, shade, wireframe ) };
    job.m_instances[ instance ].m_lit |= shade != SHADE_FLAT;
}

void model_render::submit( IMatRenderContext *render_context )
{
    Assert( this->m_job != nullptr );

    this->m_job->m_clear_stencil = true;
    queue( render_context, this->m_layers_phases[ this->m_job - s_jobs ] );
}

void model_render::add_glow( u32 instance, const config::glow_profile_t &profile )
{
    Assert( this->m_job != nullptr );

    job_t &job{ *this->m_job };
    if ( job.m_glow_count == MAX_INSTANCES )
    {
        return;
    }

    const float alpha{ static_cast< float >( profile.m_color.a ) / 255.0f / 255.0f };

    job.m_glows[ job.m_glow_count++ ] = glow_t{
        instance,
        { static_cast< float >( profile.m_color.r ) * alpha, static_cast< float >( profile.m_color.g ) * alpha,
          static_cast< float >( profile.m_color.b ) * alpha, 1.0f },
        profile.m_stencil > 0 ? ( profile.m_stencil + 1 ) / 2 : 0,
        profile.m_blur > 0 ? static_cast< float >( profile.m_blur ) : 0.0f,
    };
}

void model_render::submit_glow( IMatRenderContext *render_context )
{
    Assert( this->m_job != nullptr );

    if ( !this->m_job->m_glow_count )
    {
        return;
    }

    queue( render_context, this->m_glow_phases[ this->m_job - s_jobs ] );
}

void model_render::end_scene( )
{
    this->m_routing = false;
}

void model_render::on_device_lost( )
{
    this->m_state_block.Release( );
    this->m_glow_targets.release( );
}

bool model_render::draw_in_place( const DrawModelInfo_t &info, i32 flags )
{
    job_t      &job{ s_jobs[ ( flags >> ROUTE_JOB_SHIFT ) & 0x3 ] };
    const usize index{ static_cast< usize >( ( flags >> ROUTE_INSTANCE_SHIFT ) & ROUTE_INSTANCE_MASK ) };
    Assert( index < job.m_instance_count );

    instance_t &instance{ job.m_instances[ index ] };
    const bool  engine_base{ ( flags & ROUTE_ENGINE_BASE ) != 0 };

    instance.m_lod = info.m_Lod;

    IMatRenderContext *render_context{ g_csgo.m_material_system->GetRenderContext( ) };
    const bool         queued{ render_context->GetCallQueue( ) != nullptr };
    float              eye[ 4 ]{ };

    if ( !queued )
    {
        view_projection( render_context, instance.m_view_projection, eye );
    }

    render_context->Release( );

    if ( queued )
    {
        return false;
    }

    if ( !instance.m_surface_count || this->drawn_before( job, index, instance ) )
    {
        instance.m_drawn = true;
        return true;
    }

    const render::d3d9::device9 device{ g_render.device( ) };
    if ( !device || !this->begin_draw( device ) )
    {
        return false;
    }

    const float time[ 4 ]{ job.m_time, 0.0f, 0.0f, 0.0f };

    device.SetVertexShaderConstantF( VIEW_PROJECTION_REGISTER, instance.m_view_projection[ 0 ], 4 );
    device.SetVertexShaderConstantF( EYE_REGISTER, eye, 1 );
    device.SetPixelShaderConstantF( TIME_REGISTER, time, 1 );
    device.SetRenderState( render::d3d9::D3DRS_ZFUNC, render::d3d9::D3DCMP_LESSEQUAL );
    device.SetRenderState( render::d3d9::D3DRS_STENCILENABLE, 0 );

    instance.m_deferred = !engine_base && instance.m_surfaces[ 0 ].blends( );

    if ( instance.m_deferred )
    {
        device.SetRenderState( render::d3d9::D3DRS_ZWRITEENABLE, 1 );
        this->apply_surface( device, instance.m_surfaces[ 0 ] );
        device.SetRenderState( render::d3d9::D3DRS_COLORWRITEENABLE, 0 );
        this->draw_instance( device, job, instance );
    } else
    {
        for ( u32 surface{ }; surface < instance.m_surface_count; ++surface )
        {
            const bool base{ surface == 0 && !engine_base };

            device.SetRenderState( render::d3d9::D3DRS_ZWRITEENABLE, base );
            set_decal_bias( device, !base );
            this->apply_surface( device, instance.m_surfaces[ surface ] );
            this->draw_instance( device, job, instance );
        }
    }

    this->m_state_block.Apply( );
    instance.m_drawn = true;
    return true;
}

model_render::surface_t model_render::make_surface( const color_t &color, shade_t shade, bool wireframe )
{
    return surface_t{
        { static_cast< float >( color.r ) / 255.0f, static_cast< float >( color.g ) / 255.0f, static_cast< float >( color.b ) / 255.0f,
          static_cast< float >( color.a ) / 255.0f },
        shade,
        wireframe,
    };
}

bool model_render::drawn_before( const job_t &job, usize index, const instance_t &instance ) const
{
    const pose_t *poses{ job.m_poses };
    const usize   pose_bytes{ static_cast< usize >( instance.m_header->numbones ) * sizeof( pose_t ) };

    for ( usize earlier{ }; earlier < index; ++earlier )
    {
        const instance_t &other{ job.m_instances[ earlier ] };

        if ( !other.m_drawn || other.m_header != instance.m_header || other.m_body != instance.m_body || other.m_lod != instance.m_lod ||
             __builtin_memcmp( &other.m_view_projection, &instance.m_view_projection, sizeof( VMatrix ) ) != 0 )
        {
            continue;
        }

        if ( __builtin_memcmp( poses + other.m_first_pose, poses + instance.m_first_pose, pose_bytes ) == 0 )
        {
            return true;
        }
    }

    return false;
}

void model_render::queue( IMatRenderContext *render_context, phase_t &phase )
{
    if ( ICallQueue * queue{ render_context->GetCallQueue( ) } )
    {
        queue->QueueFunctor( &phase );
    } else
    {
        phase( );
    }
}

void model_render::draw( job_t &job )
{
    IMatRenderContext *render_context{ g_csgo.m_material_system->GetRenderContext( ) };
    const bool         queued{ render_context->GetCallQueue( ) != nullptr };
    VMatrix            main_view{ };
    float              eye[ 4 ]{ };

    if ( !queued )
    {
        view_projection( render_context, main_view, eye );
    }

    render_context->Release( );

    const render::d3d9::device9 device{ g_render.device( ) };
    if ( queued || !device || !this->begin_draw( device ) )
    {
        return;
    }

    const float time[ 4 ]{ job.m_time, 0.0f, 0.0f, 0.0f };

    device.SetVertexShaderConstantF( VIEW_PROJECTION_REGISTER, main_view[ 0 ], 4 );
    device.SetVertexShaderConstantF( EYE_REGISTER, eye, 1 );
    device.SetPixelShaderConstantF( TIME_REGISTER, time, 1 );
    device.SetRenderState( render::d3d9::D3DRS_ZFUNC, render::d3d9::D3DCMP_LESSEQUAL );
    device.SetRenderState( render::d3d9::D3DRS_ZWRITEENABLE, 0 );
    device.SetRenderState( render::d3d9::D3DRS_STENCILENABLE, 0 );

    for ( usize index{ }; index < job.m_instance_count; ++index )
    {
        instance_t &instance{ job.m_instances[ index ] };

        if ( !instance.m_deferred || __builtin_memcmp( &instance.m_view_projection, &main_view, sizeof( VMatrix ) ) != 0 )
        {
            continue;
        }

        for ( u32 surface{ }; surface < instance.m_surface_count; ++surface )
        {
            this->apply_surface( device, instance.m_surfaces[ surface ] );
            this->draw_instance( device, job, instance );
        }

        instance.m_deferred = false;
    }

    if ( !job.m_layer_counts[ LAYER_OCCLUDED_BASE ] && !job.m_layer_counts[ LAYER_OCCLUDED_OVERLAY ] )
    {
        this->m_state_block.Apply( );
        return;
    }

    if ( job.m_clear_stencil )
    {
        device.Clear( render::d3d9::D3DCLEAR_STENCIL, 0, 1.0f, 0 );
    }

    device.SetRenderState( render::d3d9::D3DRS_STENCILENABLE, 1 );
    device.SetRenderState( render::d3d9::D3DRS_STENCILMASK, 0xFF );

    device.SetRenderState( render::d3d9::D3DRS_COLORWRITEENABLE, 0 );
    set_stencil_state( device, false );

    for ( usize index{ }; index < job.m_instance_count; ++index )
    {
        const instance_t &instance{ job.m_instances[ index ] };

        if ( instance.m_drawn && ( instance.m_detached || __builtin_memcmp( &instance.m_view_projection, &main_view, sizeof( VMatrix ) ) == 0 ) )
        {
            device.SetRenderState( render::d3d9::D3DRS_FILLMODE,
                                   instance.m_surfaces[ 0 ].m_wireframe ? render::d3d9::D3DFILL_WIREFRAME : render::d3d9::D3DFILL_SOLID );
            this->draw_instance( device, job, instance );
        }
    }

    device.SetRenderState( render::d3d9::D3DRS_COLORWRITEENABLE, 0x7 );
    set_stencil_state( device, true );

    for ( usize layer{ }; layer < LAYER_COUNT; ++layer )
    {
        for ( usize entry{ }; entry < job.m_layer_counts[ layer ]; ++entry )
        {
            const layer_t    &queued_layer{ job.m_layers[ layer ][ entry ] };
            const instance_t &instance{ job.m_instances[ queued_layer.m_instance ] };

            if ( !instance.m_drawn )
            {
                continue;
            }

            this->apply_surface( device, queued_layer.m_surface );
            this->draw_instance( device, job, instance );
        }
    }

    this->m_state_block.Apply( );
}

void model_render::draw_glow( job_t &job )
{
    IMatRenderContext *render_context{ g_csgo.m_material_system->GetRenderContext( ) };
    const bool         queued{ render_context->GetCallQueue( ) != nullptr };
    VMatrix            main_view{ };
    float              eye[ 4 ]{ };

    if ( !queued )
    {
        view_projection( render_context, main_view, eye );
    }

    render_context->Release( );

    const render::d3d9::device9 device{ g_render.device( ) };
    if ( queued || !device )
    {
        return;
    }

    render::d3d9::surface9 frame{ };
    render::d3d9::surface9 depth{ };
    device.GetRenderTarget( 0, &frame );
    device.GetDepthStencilSurface( &depth );

    render::d3d9::D3DSURFACE_DESC description{ };
    render::d3d9::D3DVIEWPORT9    viewport{ };

    if ( frame && depth && frame.GetDesc( &description ) >= render::d3d9::D3D_OK && device.GetViewport( &viewport ) >= render::d3d9::D3D_OK &&
         this->begin_draw( device ) )
    {
        if ( this->ensure_glow_targets( device, description.width, description.height ) )
        {
            device.SetVertexShaderConstantF( VIEW_PROJECTION_REGISTER, main_view[ 0 ], 4 );
            device.SetVertexShaderConstantF( EYE_REGISTER, eye, 1 );
            device.Clear( render::d3d9::D3DCLEAR_STENCIL, 0, 1.0f, 0 );

            device.SetRenderState( render::d3d9::D3DRS_COLORWRITEENABLE, 0 );
            device.SetRenderState( render::d3d9::D3DRS_ZFUNC, render::d3d9::D3DCMP_ALWAYS );
            device.SetRenderState( render::d3d9::D3DRS_ZWRITEENABLE, 0 );
            device.SetRenderState( render::d3d9::D3DRS_ALPHABLENDENABLE, 0 );
            device.SetRenderState( render::d3d9::D3DRS_FILLMODE, render::d3d9::D3DFILL_SOLID );
            device.SetRenderState( render::d3d9::D3DRS_STENCILENABLE, 1 );
            device.SetRenderState( render::d3d9::D3DRS_STENCILFUNC, render::d3d9::D3DCMP_ALWAYS );
            device.SetRenderState( render::d3d9::D3DRS_STENCILREF, 1 );
            device.SetRenderState( render::d3d9::D3DRS_STENCILPASS, render::d3d9::D3DSTENCILOP_REPLACE );
            device.SetRenderState( render::d3d9::D3DRS_STENCILFAIL, render::d3d9::D3DSTENCILOP_KEEP );
            device.SetRenderState( render::d3d9::D3DRS_STENCILZFAIL, render::d3d9::D3DSTENCILOP_KEEP );
            device.SetRenderState( render::d3d9::D3DRS_STENCILMASK, 0xFF );
            device.SetRenderState( render::d3d9::D3DRS_STENCILWRITEMASK, 0xFF );

            for ( usize index{ }; index < job.m_glow_count; ++index )
            {
                const instance_t &instance{ job.m_instances[ job.m_glows[ index ].m_instance ] };

                if ( instance.m_drawn )
                {
                    this->draw_instance( device, job, instance );
                }
            }

            device.SetRenderState( render::d3d9::D3DRS_COLORWRITEENABLE, 0x7 );

            for ( usize index{ }; index < job.m_glow_count; ++index )
            {
                const glow_t &settings{ job.m_glows[ index ] };
                bool          chained{ };

                for ( usize earlier{ }; earlier < index && !chained; ++earlier )
                {
                    chained = job.m_glows[ earlier ].m_radius == settings.m_radius && job.m_glows[ earlier ].m_blur == settings.m_blur;
                }

                if ( chained )
                {
                    continue;
                }

                device.SetDepthStencilSurface( nullptr );
                device.SetRenderTarget( 0, &this->m_glow_targets.m_surfaces[ 0 ] );
                device.Clear( render::d3d9::D3DCLEAR_TARGET, 0, 1.0f, 0 );
                device.SetRenderState( render::d3d9::D3DRS_ZENABLE, 0 );
                device.SetRenderState( render::d3d9::D3DRS_STENCILENABLE, 0 );
                device.SetRenderState( render::d3d9::D3DRS_ALPHABLENDENABLE, 0 );
                device.SetPixelShader( &this->m_pixel_shaders[ SHADE_FLAT ] );

                for ( usize member{ index }; member < job.m_glow_count; ++member )
                {
                    const glow_t     &glow{ job.m_glows[ member ] };
                    const instance_t &instance{ job.m_instances[ glow.m_instance ] };

                    if ( !instance.m_drawn || glow.m_radius != settings.m_radius || glow.m_blur != settings.m_blur )
                    {
                        continue;
                    }

                    device.SetPixelShaderConstantF( 0, glow.m_color, 1 );
                    this->draw_instance( device, job, instance );
                }

                this->draw_glow_chain( device, settings, frame, depth, viewport );
            }
        }

        device.SetRenderTarget( 0, &frame );
        device.SetDepthStencilSurface( &depth );
        this->m_state_block.Apply( );
    }

    frame.Release( );
    depth.Release( );
}

void model_render::draw_glow_chain( const render::d3d9::device9      &device,
                                    const glow_t                     &settings,
                                    const render::d3d9::surface9     &frame,
                                    const render::d3d9::surface9     &depth,
                                    const render::d3d9::D3DVIEWPORT9 &viewport )
{
    struct pass_t
    {
        const render::d3d9::object9 *m_shader{ };
        float                        m_step_x{ };
        float                        m_step_y{ };
        float                        m_radius{ };
        u32                          m_filter{ };
    };

    const glow_targets_t &targets{ this->m_glow_targets };
    const float           texel_x{ 1.0f / static_cast< float >( targets.m_width ) };
    const float           texel_y{ 1.0f / static_cast< float >( targets.m_height ) };
    const float           radius{ static_cast< float >( settings.m_radius ) };

    pass_t passes[ 4 ]{ };
    usize  count{ };

    if ( settings.m_radius > 0 )
    {
        passes[ count++ ] = pass_t{ &this->m_dilate_shader, texel_x, 0.0f, radius, render::d3d9::D3DTEXF_POINT };
        passes[ count++ ] = pass_t{ &this->m_dilate_shader, 0.0f, texel_y, radius, render::d3d9::D3DTEXF_POINT };
    }

    if ( settings.m_blur > 0.0f )
    {
        passes[ count++ ] = pass_t{ &this->m_blur_shader, texel_x * GLOW_BLUR_SPACING, 0.0f, 0.0f, render::d3d9::D3DTEXF_LINEAR };
        passes[ count++ ] = pass_t{ &this->m_blur_shader, 0.0f, texel_y * GLOW_BLUR_SPACING, 0.0f, render::d3d9::D3DTEXF_LINEAR };
    }

    Assert( count >= 2 );

    device.SetVertexShader( nullptr );
    device.SetFVF( render::d3d9::D3DFVF_OVERLAY_VERTEX );
    device.SetRenderState( render::d3d9::D3DRS_CULLMODE, render::d3d9::D3DCULL_NONE );
    device.SetSamplerState( 0, render::d3d9::D3DSAMP_ADDRESSU, render::d3d9::D3DTADDRESS_CLAMP );
    device.SetSamplerState( 0, render::d3d9::D3DSAMP_ADDRESSV, render::d3d9::D3DTADDRESS_CLAMP );
    device.SetSamplerState( 0, render::d3d9::D3DSAMP_MIPFILTER, render::d3d9::D3DTEXF_NONE );
    device.SetSamplerState( 0, render::d3d9::D3DSAMP_SRGBTEXTURE, 0 );

    usize source{ };

    for ( usize index{ }; index < count; ++index )
    {
        const pass_t &pass{ passes[ index ] };
        const bool    last{ index + 1 == count };

        device.SetTexture( 0, &targets.m_textures[ source ] );

        if ( last )
        {
            device.SetRenderTarget( 0, &frame );
            device.SetDepthStencilSurface( &depth );
            device.SetViewport( &viewport );
            device.SetRenderState( render::d3d9::D3DRS_STENCILENABLE, 1 );
            device.SetRenderState( render::d3d9::D3DRS_STENCILFUNC, render::d3d9::D3DCMP_EQUAL );
            device.SetRenderState( render::d3d9::D3DRS_STENCILREF, 0 );
            device.SetRenderState( render::d3d9::D3DRS_STENCILPASS, render::d3d9::D3DSTENCILOP_KEEP );
            device.SetRenderState( render::d3d9::D3DRS_STENCILWRITEMASK, 0 );
            device.SetRenderState( render::d3d9::D3DRS_ALPHABLENDENABLE, 1 );
            device.SetRenderState( render::d3d9::D3DRS_SRCBLEND, render::d3d9::D3DBLEND_ONE );
            device.SetRenderState( render::d3d9::D3DRS_DESTBLEND, render::d3d9::D3DBLEND_ONE );
        } else
        {
            device.SetRenderTarget( 0, &targets.m_surfaces[ source ^ 1 ] );
        }

        const float scale{ last && settings.m_blur > 0.0f ? settings.m_blur : 1.0f };
        const float constants[ PIXEL_REGISTER_COUNT ][ 4 ]{
            { pass.m_step_x, pass.m_step_y, pass.m_radius, 0.0f },
            { scale, scale, scale, scale },
        };

        device.SetSamplerState( 0, render::d3d9::D3DSAMP_MAGFILTER, pass.m_filter );
        device.SetSamplerState( 0, render::d3d9::D3DSAMP_MINFILTER, pass.m_filter );
        device.SetPixelShader( pass.m_shader );
        device.SetPixelShaderConstantF( 0, constants[ 0 ], PIXEL_REGISTER_COUNT );

        const render::d3d9::D3DVIEWPORT9 target{ last ? viewport : render::d3d9::D3DVIEWPORT9{ 0, 0, targets.m_width, targets.m_height, 0.0f, 1.0f } };
        const float                      left{ static_cast< float >( target.x ) - 0.5f };
        const float                      top{ static_cast< float >( target.y ) - 0.5f };
        const float                      right{ left + static_cast< float >( target.width ) };
        const float                      bottom{ top + static_cast< float >( target.height ) };

        const render::d3d9::overlay_vertex vertices[ 4 ]{
            { left, top, 0.0f, 1.0f, 0xFFFFFFFFu, 0.0f, 0.0f },
            { right, top, 0.0f, 1.0f, 0xFFFFFFFFu, 1.0f, 0.0f },
            { left, bottom, 0.0f, 1.0f, 0xFFFFFFFFu, 0.0f, 1.0f },
            { right, bottom, 0.0f, 1.0f, 0xFFFFFFFFu, 1.0f, 1.0f },
        };

        device.DrawPrimitiveUP( render::d3d9::D3DPT_TRIANGLESTRIP, 2, vertices, sizeof( render::d3d9::overlay_vertex ) );

        source ^= 1;
    }

    device.SetTexture( 0, nullptr );
}

bool model_render::begin_draw( const render::d3d9::device9 &device )
{
    if ( !this->ensure_resources( device ) || this->m_state_block.Capture( ) < render::d3d9::D3D_OK )
    {
        return false;
    }

    device.SetPixelShader( &this->m_pixel_shaders[ SHADE_FLAT ] );
    device.SetRenderState( render::d3d9::D3DRS_ZENABLE, 1 );
    device.SetRenderState( render::d3d9::D3DRS_CULLMODE, render::d3d9::D3DCULL_CCW );
    device.SetRenderState( render::d3d9::D3DRS_DEPTHBIAS, 0 );
    device.SetRenderState( render::d3d9::D3DRS_SLOPESCALEDEPTHBIAS, 0 );
    device.SetRenderState( render::d3d9::D3DRS_ALPHATESTENABLE, 0 );
    device.SetRenderState( render::d3d9::D3DRS_FOGENABLE, 0 );
    device.SetRenderState( render::d3d9::D3DRS_SCISSORTESTENABLE, 0 );
    device.SetRenderState( render::d3d9::D3DRS_CLIPPLANEENABLE, 0 );
    device.SetRenderState( render::d3d9::D3DRS_SRGBWRITEENABLE, 0 );
    device.SetRenderState( render::d3d9::D3DRS_COLORWRITEENABLE, 0x7 );
    device.SetRenderState( render::d3d9::D3DRS_SRCBLEND, render::d3d9::D3DBLEND_SRCALPHA );
    device.SetRenderState( render::d3d9::D3DRS_DESTBLEND, render::d3d9::D3DBLEND_INVSRCALPHA );
    device.SetRenderState( render::d3d9::D3DRS_BLENDOP, render::d3d9::D3DBLENDOP_ADD );
    return true;
}

bool model_render::ensure_resources( const render::d3d9::device9 &device )
{
    if ( this->m_device != device.Get( ) )
    {
        this->release_resources( );
        this->m_device = device.Get( );
    }

    if ( !this->m_pixel_shaders[ SHADE_FLAT ] )
    {
        bool created{ device.CreateVertexShader( render::shaders::MODEL_SKINNED_VERTEX_SHADER, &this->m_skinned_shader ) >= render::d3d9::D3D_OK &&
                      device.CreateVertexShader( render::shaders::MODEL_RIGID_VERTEX_SHADER, &this->m_rigid_shader ) >= render::d3d9::D3D_OK &&
                      device.CreateVertexShader( render::shaders::MODEL_SKINNED_LIT_VERTEX_SHADER, &this->m_skinned_lit_shader ) >= render::d3d9::D3D_OK &&
                      device.CreateVertexShader( render::shaders::MODEL_RIGID_LIT_VERTEX_SHADER, &this->m_rigid_lit_shader ) >= render::d3d9::D3D_OK &&
                      device.CreatePixelShader( render::shaders::GLOW_DILATE_PIXEL_SHADER, &this->m_dilate_shader ) >= render::d3d9::D3D_OK &&
                      device.CreatePixelShader( render::shaders::GLOW_BLUR_PIXEL_SHADER, &this->m_blur_shader ) >= render::d3d9::D3D_OK };

        for ( usize index{ }; index < 32 && created; ++index )
        {
            render::d3d9::D3DVERTEXELEMENT9 elements[ 5 ]{ };
            layout_elements( index, elements );
            created = device.CreateVertexDeclaration( elements, &this->m_declarations[ index ] ) >= render::d3d9::D3D_OK;
        }

        for ( usize shade{ SHADE_COUNT }; shade-- > 0 && created; )
        {
            created = device.CreatePixelShader( PIXEL_SHADERS[ shade ], &this->m_pixel_shaders[ shade ] ) >= render::d3d9::D3D_OK;
        }

        if ( !created )
        {
            this->release_resources( );
            return false;
        }
    }

    if ( this->m_state_block )
    {
        return true;
    }

    if ( device.BeginStateBlock( ) < render::d3d9::D3D_OK )
    {
        return false;
    }

    const float                registers[ VERTEX_REGISTER_COUNT ][ 4 ]{ };
    render::d3d9::D3DVIEWPORT9 viewport{ };
    device.GetViewport( &viewport );

    device.SetVertexShader( nullptr );
    device.SetPixelShader( nullptr );
    device.SetVertexDeclaration( nullptr );
    device.SetStreamSource( 0, nullptr, 0, 0 );
    device.SetIndices( nullptr );
    device.SetVertexShaderConstantF( 0, registers[ 0 ], VERTEX_REGISTER_COUNT );
    device.SetPixelShaderConstantF( 0, registers[ 0 ], PIXEL_REGISTER_COUNT );
    device.SetTexture( 0, nullptr );
    device.SetViewport( &viewport );

    for ( const u32 state : RECORDED_STATES )
    {
        device.SetRenderState( state, 0 );
    }

    for ( const u32 state : RECORDED_SAMPLER_STATES )
    {
        device.SetSamplerState( 0, state, 0 );
    }

    return device.EndStateBlock( &this->m_state_block ) >= render::d3d9::D3D_OK;
}

bool model_render::ensure_glow_targets( const render::d3d9::device9 &device, u32 width, u32 height )
{
    glow_targets_t &targets{ this->m_glow_targets };

    if ( targets.m_textures[ 1 ] && targets.m_width == width && targets.m_height == height )
    {
        return true;
    }

    targets.release( );

    for ( usize index{ }; index < 2; ++index )
    {
        if ( device.CreateTexture( width, height, 1, render::d3d9::D3DUSAGE_RENDERTARGET, render::d3d9::D3DFMT_A8R8G8B8,
                                   render::d3d9::D3DPOOL_DEFAULT, &targets.m_textures[ index ] ) < render::d3d9::D3D_OK ||
             targets.m_textures[ index ].GetSurfaceLevel( 0, &targets.m_surfaces[ index ] ) < render::d3d9::D3D_OK )
        {
            targets.release( );
            return false;
        }
    }

    targets.m_width  = width;
    targets.m_height = height;
    return true;
}

void model_render::release_resources( )
{
    this->m_glow_targets.release( );
    this->m_state_block.Release( );
    this->m_dilate_shader.Release( );
    this->m_blur_shader.Release( );
    this->m_skinned_shader.Release( );
    this->m_rigid_shader.Release( );
    this->m_skinned_lit_shader.Release( );
    this->m_rigid_lit_shader.Release( );

    for ( render::d3d9::object9 &declaration : this->m_declarations )
    {
        declaration.Release( );
    }

    for ( render::d3d9::object9 &shader : this->m_pixel_shaders )
    {
        shader.Release( );
    }
}

void model_render::apply_surface( const render::d3d9::device9 &device, const surface_t &surface ) const
{
    const bool additive{ surface.additive( ) };

    device.SetPixelShader( &this->m_pixel_shaders[ surface.m_shade ] );
    device.SetPixelShaderConstantF( 0, surface.m_color, 1 );

    device.SetRenderState( render::d3d9::D3DRS_ALPHABLENDENABLE, surface.blends( ) );
    device.SetRenderState( render::d3d9::D3DRS_DESTBLEND, additive ? render::d3d9::D3DBLEND_ONE : render::d3d9::D3DBLEND_INVSRCALPHA );
    device.SetRenderState( render::d3d9::D3DRS_FILLMODE, surface.m_wireframe ? render::d3d9::D3DFILL_WIREFRAME : render::d3d9::D3DFILL_SOLID );
}

void model_render::draw_instance( const render::d3d9::device9 &device, const job_t &job, const instance_t &instance ) const
{
    const pose_t *poses{ job.m_poses + instance.m_first_pose };

    device.SetVertexShaderConstantF( PALETTE_REGISTER, CLEARED_PALETTE[ 0 ], 3 * MAX_HARDWARE_BONES );

    if ( instance.m_header->numbones == 1 )
    {
        device.SetVertexShaderConstantF( PALETTE_REGISTER, poses[ 0 ].m_matrix[ 0 ], 3 );
    }

    for_each_group( instance.m_header, instance.m_hardware, instance.m_body, instance.m_lod,
                    [ & ]( const studiomeshgroup_t &group )
    {
        this->draw_group( device, group, poses, instance.m_lit );
        return true;
    } );
}

void model_render::draw_group( const render::d3d9::device9 &device, const studiomeshgroup_t &group, const pose_t *poses, bool lit ) const
{
    if ( group.m_MeshNeedsRestore )
    {
        return;
    }

    group.m_pMesh->HandleLateCreation( );

    const CMeshDX8         *mesh{ group.m_pMesh };
    const CVertexBufferDx8 *vertices{ mesh->m_vertex_buffer };
    const u64               format{ mesh->m_vertex_format };
    const bool              skinned{ ( format & VERTEX_BONE_INDEX ) && ( format & VERTEX_BONE_WEIGHT_MASK ) };
    const bool              compressed{ ( format & VERTEX_FORMAT_COMPRESSED ) != 0 };

    const float decode[ 4 ]{ 0.0f, compressed ? 1.0f / 32768.0f : 1.0f, compressed ? 1.0f / 32768.0f : 0.0f, compressed ? 1.0f : 0.0f };
    device.SetVertexShaderConstantF( WEIGHT_DECODE_REGISTER, decode, 1 );

    const bool shaded{ lit && ( format & VERTEX_NORMAL ) };

    device.SetVertexDeclaration( &this->m_declarations[ layout_index( format ) ] );
    device.SetVertexShader( skinned ? ( shaded ? &this->m_skinned_lit_shader : &this->m_skinned_shader )
                                    : ( shaded ? &this->m_rigid_lit_shader : &this->m_rigid_shader ) );
    device.SetStreamSource( 0, vertices->m_vertex_buffer, 0, static_cast< u32 >( vertices->m_vertex_size ) );
    device.SetIndices( mesh->m_index_buffer->m_index_buffer );

    for ( i32 index{ }; index < group.m_NumStrips; ++index )
    {
        const StripHeader_t &strip{ group.m_pStripData[ index ] };

        for ( i32 change{ }; change < strip.numBoneStateChanges; ++change )
        {
            const BoneStateChangeHeader_t *state{ strip.pBoneStateChange( change ) };

            if ( state->newBoneID < 0 )
            {
                break;
            }

            Assert( state->hardwareID >= 0 && state->hardwareID < MAX_HARDWARE_BONES );

            device.SetVertexShaderConstantF( PALETTE_REGISTER + 3 * static_cast< u32 >( state->hardwareID ),
                                             poses[ state->newBoneID ].m_matrix[ 0 ], 3 );
        }

        if ( strip.flags & ( STRIP_IS_QUADLIST_REG | STRIP_IS_QUADLIST_EXTRA ) )
        {
            continue;
        }

        device.DrawIndexedPrimitive( render::d3d9::D3DPT_TRIANGLELIST,
                                     0,
                                     0,
                                     static_cast< u32 >( vertices->m_vertex_count ),
                                     static_cast< u32 >( strip.indexOffset ),
                                     static_cast< u32 >( strip.numIndices / 3 ) );
    }
}
