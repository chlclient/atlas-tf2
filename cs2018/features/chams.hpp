#pragma once

#include "link.hpp"
#include "model_render.hpp"

class chams
{
public:
    enum : usize
    {
        CHAM_OFF = 0,
        CHAM_FLAT,
        CHAM_SHADED,
        CHAM_FRESNEL,
        CHAM_WIREFRAME,
        CHAM_LAVA,
        CHAM_PEARL,
        CHAM_BUBBLE,
        CHAM_LSD,
        CHAM_NETWORK,
        CHAM_SOAP,
        CHAM_AURORA,
        CHAM_AQUARIUM,
        CHAM_SKY,
        CHAM_COUNT,
    };

    void begin_frame( );
    bool wants( C_BaseEntity *entity ) const;
    bool on_draw_model( const DrawModelInfo_t *info, const matrix3x4 *bone_to_world, i32 &flags );
    void render_main( );

private:
    static constexpr usize TARGET_NONE{ config::ESP_TARGET_COUNT };

    struct routed_t
    {
        C_BaseEntity *m_entity{ };
        u32           m_instance{ };
        usize         m_target{ };
    };

    usize     target_for( C_BaseEntity *entity ) const;
    routed_t *find_routed( C_BaseEntity *entity );
    void      add_culled( i32 index );

    C_CSPlayer *m_players[ MAX_PLAYERS + 1 ]{ };
    usize       m_targets[ MAX_PLAYERS + 1 ]{ };
    bool        m_active{ };
    routed_t    m_routed[ model_render::MAX_INSTANCES ]{ };
    usize       m_routed_count{ };
    matrix3x4   m_bones[ MAXSTUDIOBONES ]{ };
};

extern chams g_chams;
