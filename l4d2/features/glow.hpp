#pragma once

#include "link.hpp"

class glow
{
public:
    void render( IMatRenderContext *render_context, i32 width, i32 height );

    bool m_driving{ };

private:
    struct draw_t
    {
        C_BaseEntity *m_entity{ };
        usize         m_target{ };
    };

    void collect( );
    void create_materials( );
    void set_modulation( const color_t &color ) const;
    void draw_target( usize target ) const;
    void mask_pass( IMatRenderContext *render_context ) const;
    void color_pass( IMatRenderContext *render_context, usize target, i32 width, i32 height ) const;

    draw_t m_draws[ 1 << MAX_EDICT_BITS ]{ };
    usize  m_draw_count{ };
    bool   m_has_target[ config::TARGET_COUNT ]{ };

    IMaterial *m_glow_color{ };
    IMaterial *m_halo{ };
    IMaterial *m_blur_x{ };
    IMaterial *m_blur_y{ };
    ITexture  *m_buffer1{ };
    ITexture  *m_buffer2{ };
};

extern glow g_glow;
