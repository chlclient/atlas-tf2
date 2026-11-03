#pragma once

#include "link.hpp"

class bone_setup
{
public:
    void store_pose( C_CSPlayer *player, float time );
    bool player_setup( C_CSPlayer *player, matrix3x4 *out, i32 max_bones );

    static void prepare_build( C_CSPlayer *player );

private:
    struct slot_t
    {
        matrix3x4           m_bones[ MAXSTUDIOBONES ]{ };
        C_AnimationLayer    m_layers[ ANIMATION_LAYER_COUNT ]{ };
        C_CSPlayer         *m_player{ };
        CStudioHdr         *m_studio{ };
        vec3                m_built_origin{ };
        float               m_time{ };
        i32                 m_version{ };
        i32                 m_built_version{ };
        i32                 m_bone_count{ };
        atl::atomic< bool > m_busy{ };
    };

    bool build( C_CSPlayer *player, slot_t &slot ) const;

    slot_t m_slots[ MAX_TRACKED_PLAYERS ]{ };
};

extern bone_setup g_bone_mgr;
