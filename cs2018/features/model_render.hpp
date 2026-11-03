#pragma once

#include "link.hpp"

class model_render
{
public:
    enum : usize
    {
        LAYER_OCCLUDED_BASE = 0,
        LAYER_OCCLUDED_OVERLAY,
        LAYER_COUNT,
    };

    static constexpr i32 ROUTE_ENGINE_BASE{ static_cast< i32 >( 1u << 31 ) };
    static constexpr i32 ROUTE_TAKEN_OVER{ 1 << 30 };
    static constexpr i32 ROUTE_JOB_SHIFT{ 28 };
    static constexpr i32 ROUTE_INSTANCE_SHIFT{ 16 };
    static constexpr i32 ROUTE_INSTANCE_MASK{ 0xFFF };
    static constexpr i32 ROUTE_MASK{ ROUTE_ENGINE_BASE | ROUTE_TAKEN_OVER | ( 0x3 << ROUTE_JOB_SHIFT ) |
                                     ( ROUTE_INSTANCE_MASK << ROUTE_INSTANCE_SHIFT ) };
    static constexpr u32 INVALID_INSTANCE{ 0xFFFFFFFF };

    static constexpr usize MAX_INSTANCES{ 256 };
    static constexpr usize MAX_POSES{ 12288 };
    static constexpr usize MAX_LAYERS{ 512 };

    enum shade_t : u8
    {
        SHADE_FLAT = 0,
        SHADE_SHADED,
        SHADE_FRESNEL,
        SHADE_LAVA,
        SHADE_PEARL,
        SHADE_BUBBLE,
        SHADE_LSD,
        SHADE_NETWORK,
        SHADE_SOAP,
        SHADE_AURORA,
        SHADE_AQUARIUM,
        SHADE_SKY,
        SHADE_COUNT,
    };

    void begin_frame( );
    void end_frame( );

    bool active( ) const
    {
        return this->m_job != nullptr;
    }

    bool take_over( const DrawModelInfo_t &info, const matrix3x4 *bone_to_world, bool engine_base, i32 &flags, u32 &instance );
    bool add_detached( const studiohdr_t *header, const studiohwdata_t *hardware, i32 body, const matrix3x4 *bone_to_world, u32 &instance );
    void add_in_place( u32 instance, const color_t &color, shade_t shade, bool wireframe );
    void add_layer( usize layer, u32 instance, const color_t &color, shade_t shade, bool wireframe );
    void submit( IMatRenderContext *render_context );
    void add_glow( u32 instance, const config::glow_profile_t &profile );
    void submit_glow( IMatRenderContext *render_context );
    void end_scene( );
    void on_device_lost( );

    bool draw_in_place( const DrawModelInfo_t &info, i32 flags );

private:
    struct pose_t
    {
        matrix3x4 m_matrix{ };
    };

    struct surface_t
    {
        float   m_color[ 4 ]{ };
        shade_t m_shade{ };
        bool    m_wireframe{ };

        bool additive( ) const
        {
            return this->m_shade == SHADE_FRESNEL || this->m_shade == SHADE_LSD || this->m_shade == SHADE_SOAP || this->m_shade == SHADE_AURORA ||
                   this->m_shade == SHADE_AQUARIUM || this->m_shade == SHADE_SKY;
        }

        bool blends( ) const
        {
            return this->m_color[ 3 ] < 1.0f || this->additive( ) || this->m_shade == SHADE_NETWORK;
        }
    };

    struct instance_t
    {
        const studiohdr_t    *m_header{ };
        const studiohwdata_t *m_hardware{ };
        i32                   m_body{ };
        i32                   m_lod{ };
        u32                   m_first_pose{ };
        surface_t             m_surfaces[ 2 ]{ };
        u32                   m_surface_count{ };
        bool                  m_drawn{ };
        bool                  m_deferred{ };
        bool                  m_lit{ };
        bool                  m_detached{ };
        VMatrix               m_view_projection{ };
    };

    struct layer_t
    {
        u32       m_instance{ };
        surface_t m_surface{ };
    };

    struct glow_t
    {
        u32   m_instance{ };
        float m_color[ 4 ]{ };
        i32   m_radius{ };
        float m_blur{ };
    };

    struct glow_targets_t
    {
        render::d3d9::texture9 m_textures[ 2 ]{ };
        render::d3d9::surface9 m_surfaces[ 2 ]{ };
        u32                    m_width{ };
        u32                    m_height{ };

        void release( )
        {
            for ( usize index{ }; index < 2; ++index )
            {
                this->m_surfaces[ index ].Release( );
                this->m_textures[ index ].Release( );
            }
        }
    };

    class job_t;

    class phase_t final : public CFunctor
    {
    public:
        i32  AddRef( ) override;
        i32  Release( ) override;
        void operator( )( ) override;

        model_render *m_owner{ };
        job_t        *m_job{ };
        void ( model_render::*m_run )( job_t & ){ };
    };

    class job_t
    {
    public:
        bool acquire( )
        {
            return _InterlockedCompareExchange( &this->m_references, 1, 0 ) == 0;
        }

        bool          m_clear_stencil{ };
        float         m_time{ };
        instance_t    m_instances[ MAX_INSTANCES ]{ };
        usize         m_instance_count{ };
        pose_t        m_poses[ MAX_POSES ]{ };
        usize         m_pose_count{ };
        layer_t       m_layers[ LAYER_COUNT ][ MAX_LAYERS ]{ };
        usize         m_layer_counts[ LAYER_COUNT ]{ };
        glow_t        m_glows[ MAX_INSTANCES ]{ };
        usize         m_glow_count{ };
        volatile long m_references{ };
    };

    static surface_t make_surface( const color_t &color, shade_t shade, bool wireframe );
    bool             push_instance( const studiohdr_t    *header,
                                    const studiohwdata_t *hardware,
                                    i32                   body,
                                    const matrix3x4      *bone_to_world,
                                    u32                  &instance );
    bool             drawn_before( const job_t &job, usize index, const instance_t &instance ) const;

    static void queue( IMatRenderContext *render_context, phase_t &phase );
    void        draw( job_t &job );
    void        draw_glow( job_t &job );
    void        draw_glow_chain( const render::d3d9::device9      &device,
                                 const glow_t                     &settings,
                                 const render::d3d9::surface9     &frame,
                                 const render::d3d9::surface9     &depth,
                                 const render::d3d9::D3DVIEWPORT9 &viewport );
    bool        begin_draw( const render::d3d9::device9 &device );
    bool        ensure_resources( const render::d3d9::device9 &device );
    bool        ensure_glow_targets( const render::d3d9::device9 &device, u32 width, u32 height );
    void        release_resources( );
    void        apply_surface( const render::d3d9::device9 &device, const surface_t &surface ) const;
    void        draw_instance( const render::d3d9::device9 &device, const job_t &job, const instance_t &instance ) const;
    void        draw_group( const render::d3d9::device9 &device, const studiomeshgroup_t &group, const pose_t *poses, bool lit ) const;

    static job_t s_jobs[ 4 ];

    phase_t m_layers_phases[ 4 ]{ };
    phase_t m_glow_phases[ 4 ]{ };
    phase_t m_end_phases[ 4 ]{ };
    job_t  *m_job{ };
    bool    m_routing{ };

    void                      *m_device{ };
    render::d3d9::object9      m_skinned_shader{ };
    render::d3d9::object9      m_rigid_shader{ };
    render::d3d9::object9      m_skinned_lit_shader{ };
    render::d3d9::object9      m_rigid_lit_shader{ };
    render::d3d9::object9      m_pixel_shaders[ SHADE_COUNT ]{ };
    render::d3d9::object9      m_declarations[ 32 ]{ };
    render::d3d9::object9      m_dilate_shader{ };
    render::d3d9::object9      m_blur_shader{ };
    render::d3d9::state_block9 m_state_block{ };

    glow_targets_t m_glow_targets{ };
};

extern model_render g_model_render;
