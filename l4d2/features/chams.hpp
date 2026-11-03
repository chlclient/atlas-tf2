#pragma once

#include "link.hpp"

class chams
{
public:
    enum : usize
    {
        CHAM_OFF = 0,
        CHAM_FLAT,
        CHAM_WIREFRAME,
        CHAM_COUNT,
        CHAM_ORIGINAL = CHAM_COUNT,
    };

    void render( IMatRenderContext *render_context );
    bool drawn( i32 index ) const;
    void on_level_shutdown( );

    bool m_driving{ };

private:
    struct draw_t
    {
        C_BaseEntity *m_entity{ };
        usize         m_target{ };
    };

    void       collect( );
    void       create_materials( );
    IMaterial *resolve_material( usize mode ) const;
    void       setup_pass( IMaterial *material, const color_t &color ) const;
    void       draw_layer( C_BaseEntity *entity, usize mode, const color_t &color );
    void       run_sweep( bool occluded );
    void       mark_drawn( i32 index );

    u64 m_drawn[ ( 1 << MAX_EDICT_BITS ) / 64 ]{ };

    draw_t m_draws[ 1 << MAX_EDICT_BITS ]{ };
    usize  m_draw_count{ };
    bool   m_has_occluded{ };

    IMaterial *m_flat{ };
    IMaterial *m_wireframe{ };
};

extern chams g_chams;
