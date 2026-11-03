#include "bone_setup.hpp"

ATLAS_NO_DESTROY bone_setup g_bone_mgr{ };

ATLAS_FORCE_INLINE void swap_layers( C_CSPlayer *player, C_AnimationLayer *layers )
{
    anim_overlay_t &overlays{ player->m_AnimOverlay( ) };
    for ( i32 index{ }; index < ANIMATION_LAYER_COUNT; ++index )
    {
        const C_AnimationLayer held{ overlays[ index ] };
        overlays[ index ] = layers[ index ];
        layers[ index ]   = held;
    }
}

void bone_setup::store_pose( C_CSPlayer *player, float time )
{
    slot_t &slot{ this->m_slots[ player->entindex( ) ] };

    while ( slot.m_busy.exchange( true ) )
    {
        _mm_pause( );
    }

    slot.m_player = player;
    slot.m_time   = time;
    ++slot.m_version;

    // the layers of the step, the local's emulated server layers among them, which the entity only holds during the step
    anim_overlay_t &overlays{ player->m_AnimOverlay( ) };
    for ( i32 index{ }; index < ANIMATION_LAYER_COUNT; ++index )
    {
        slot.m_layers[ index ] = overlays[ index ];
    }

    slot.m_layers[ ANIMATION_LAYER_LEAN ].m_flWeight = 0.0f;

    slot.m_busy.store( false );
}

void bone_setup::prepare_build( C_CSPlayer *player )
{
    // The bone root is the abs angles, which the animstate update sets to the feet like the server's; the local's are
    // set again to its view angles between our update and the frame's bone setup.
    player->SetAbsAngles( vec3{ 0.0f, player->m_PlayerAnimState( )->m_flFootYaw( ), 0.0f } );

    player->m_nAnimLODflags( )           = 0;
    player->m_nCustomBlendingRuleMask( ) = -1;
    player->m_nLastNonSkippedFrame( )    = 0;

    // C_BaseAnimating::InvalidateBoneCache, which the animstate update keeps disabled while it runs
    player->m_flLastBoneSetupTime( )         = -FLT_MAX;
    player->m_iMostRecentModelBoneCounter( ) = *g_csgo.m_model_bone_counter - 1;
}

bool bone_setup::build( C_CSPlayer *player, slot_t &slot ) const
{
    prepare_build( player );

    swap_layers( player, slot.m_layers );
    const bool built{ player->BaseSetupBones( nullptr, MAXSTUDIOBONES, BONE_USED_BY_ANYTHING, slot.m_time ) };
    swap_layers( player, slot.m_layers );

    if ( built )
    {
        slot.m_bone_count = player->m_CachedBoneCount( );
        atl::memcpy( slot.m_bones, player->m_CachedBoneData( ), sizeof( matrix3x4 ) * slot.m_bone_count );
    }

    return built;
}

bool bone_setup::player_setup( C_CSPlayer *player, matrix3x4 *out, i32 max_bones )
{
    if ( player->IsDormant( ) || !player->IsAlive( ) )
    {
        return false;
    }

    slot_t &slot{ this->m_slots[ player->entindex( ) ] };

    while ( slot.m_busy.exchange( true ) )
    {
        _mm_pause( );
    }

    bool result{ slot.m_player == player };
    if ( result )
    {
        // the pose only changes on a new animation step; the local's origin moves on every predicted tick under the
        // pose the server holds, which is where the server builds it as well
        if ( slot.m_built_version != slot.m_version || slot.m_studio != player->m_pStudioHdr( ) ||
             slot.m_built_origin != player->GetAbsOrigin( ) )
        {
            result = this->build( player, slot );
            if ( result )
            {
                slot.m_built_version = slot.m_version;
                slot.m_studio        = player->m_pStudioHdr( );
                slot.m_built_origin  = player->GetAbsOrigin( );
            }
        } else
        {
            // the game invalidates every bone cache once a frame; ours stays the one built from the last step
            player->m_iMostRecentModelBoneCounter( ) = *g_csgo.m_model_bone_counter;
        }

        if ( result && out )
        {
            result = max_bones >= slot.m_bone_count;
            if ( result )
            {
                atl::memcpy( out, slot.m_bones, sizeof( matrix3x4 ) * slot.m_bone_count );
            }
        }
    }

    slot.m_busy.store( false );

    return result;
}
