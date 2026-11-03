#include "entities.hpp"
#include "global.hpp"

const vec3 &C_BaseEntity::GetAbsOrigin( )
{
    return util::call_virtual< const vec3 &( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_base_entity_get_abs_origin )( this );
}

const vec3 &C_BaseEntity::WorldSpaceCenter( )
{
    return util::call_virtual< const vec3 &( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_base_entity_world_space_center )( this );
}

void C_BaseEntity::SetAbsOrigin( const vec3 &origin )
{
    g_csgo.m_C_BaseEntity_SetAbsOrigin.get_as< void( ATLAS_THISCALL * )( C_BaseEntity *, const vec3 * ) >( )( this, &origin );
}

void C_BaseEntity::SetAbsAngles( const vec3 &angles )
{
    g_csgo.m_C_BaseEntity_SetAbsAngles.get_as< void( ATLAS_THISCALL * )( C_BaseEntity *, const vec3 * ) >( )( this, &angles );
}

bool C_BaseEntity::IsPlayer( )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void * ) >( this, g_csgo.m_base_entity_is_player )( this );
}

C_BaseEntity *C_BaseEntity::MyCombatCharacterPointer( )
{
    return util::call_virtual< C_BaseEntity *( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_base_entity_my_combat_character_pointer )( this );
}

bool C_BaseEntity::IsDormant( )
{
    IClientNetworkable *networkable{ this->GetClientNetworkable( ) };
    return util::call_virtual< bool( ATLAS_THISCALL * )( void * ) >(
        networkable, g_csgo.m_client_networkable_is_dormant )( networkable );
}

datamap_t *C_BaseEntity::GetPredDescMap( )
{
    return util::call_virtual< datamap_t *( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_base_entity_get_pred_desc_map )( this );
}

void C_BaseEntity::ShutdownPredictable( )
{
    g_csgo.m_C_BaseEntity_ShutdownPredictable.get_as< void( ATLAS_THISCALL * )( C_BaseEntity * ) >( )( this );
}

i32 C_BaseEntity::entindex( )
{
    IClientNetworkable *networkable{ this->GetClientNetworkable( ) };
    return util::call_virtual< i32( ATLAS_THISCALL * )( void * ) >(
        networkable, g_csgo.m_client_networkable_entindex )( networkable );
}

const CCSWeaponInfo *C_BaseCombatWeapon::GetCSWpnData( )
{
    return util::call_virtual< const CCSWeaponInfo *( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_weapon_cs_base_get_cs_wpn_data )( this );
}

float C_BaseCombatWeapon::GetSpread( )
{
    return util::call_virtual< float( ATLAS_THISCALL * )( void * ) >( this, g_csgo.m_weapon_cs_base_get_spread )( this );
}

float C_BaseCombatWeapon::GetInaccuracy( )
{
    return util::call_virtual< float( ATLAS_THISCALL * )( void * ) >( this, g_csgo.m_weapon_cs_base_get_inaccuracy )( this );
}

void C_BaseCombatWeapon::UpdateAccuracyPenalty( )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void * ) >( this, g_csgo.m_weapon_cs_base_update_accuracy_penalty )( this );
}

bool C_BaseCombatWeapon::IsFullAuto( )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void * ) >( this, g_csgo.m_weapon_cs_base_is_full_auto )( this );
}

void C_BasePlayer::ItemPostFrame( )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void * ) >(
        this, g_csgo.m_base_player_item_post_frame )( this );
}

void C_BaseEntity::SetModelIndex( i32 index )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, i32 ) >(
        this, g_csgo.m_base_entity_set_model_index )( this, index );
}

void C_EconEntity::UpdateFallbackAttributes( )
{
    g_csgo.m_C_EconEntity_UpdateFallbackAttributes.get_as< void( ATLAS_THISCALL * )( C_EconEntity * ) >( )( this );
}

void C_BaseViewModel::UpdateAllViewmodelAddons( )
{
    g_csgo.m_C_BaseViewModel_UpdateAllViewmodelAddons.get_as< void( ATLAS_THISCALL * )( C_BaseViewModel * ) >( )( this );
}

void C_BaseViewModel::RemoveViewmodelArmModels( )
{
    g_csgo.m_C_BaseViewModel_RemoveViewmodelArmModels.get_as< void( ATLAS_THISCALL * )( C_BaseViewModel * ) >( )( this );
}

vec3 C_CSPlayer::GetAimPunchAngleScaled( )
{
    vec3 punch{ };
    util::call_virtual< vec3 *( ATLAS_THISCALL * )( void *, vec3 * ) >(
        this, g_csgo.m_cs_player_get_aim_punch_angle_scaled )( this, &punch );
    return punch;
}

vec3 C_CSPlayer::Weapon_ShootPosition( )
{
    vec3 position{ this->GetAbsOrigin( ) + this->m_vecViewOffset( ) };

    CCSGOPlayerAnimState *anim_state{ this->m_PlayerAnimState( ) };
    if ( !anim_state->m_bLanding( ) && anim_state->m_flAnimDuckAmount( ) == 0.0f && this->IsOnGround( ) )
    {
        return position;
    }

    const i32 head{ this->LookupBone( ATLAS_XORSTR( "head_0" ).get( ) ) };
    if ( head == -1 )
    {
        return position;
    }

    if ( !this->SetupBones( nullptr, -1, BONE_USED_BY_HITBOX, g_csgo.m_globals->curtime ) )
    {
        return position;
    }

    const float head_z{ this->m_CachedBoneData( )[ head ][ 2 ][ 3 ] + 1.7f };
    if ( position.z > head_z )
    {
        position.z += ( head_z - position.z ) * math::smooth_step( math::remap_clamped( position.z - head_z, 4.0f, 10.0f, 0.0f, 1.0f ) );
    }

    return position;
}

float C_CSPlayer::GetMaxSpeed( C_BaseCombatWeapon *weapon )
{
    const bool resuming{ this->m_bResumeZoom( ) && weapon->m_zoomLevel( ) > 0 };
    const bool alternate{ resuming || weapon->m_weaponMode( ) != 0 };

    return weapon->GetCSWpnData( )->m_flMaxSpeed[ alternate ? 1 : 0 ];
}

bool C_CSPlayer::IsOtherEnemy( C_CSPlayer *other )
{
    if ( this == other )
    {
        return false;
    }

    if ( g_csgo.mp_teammates_are_enemies->GetInt( ) )
    {
        return true;
    }

    return this->m_iTeamNum( ) != other->m_iTeamNum( );
}

float C_CSPlayer::GetLayerSequenceCycleRate( C_AnimationLayer *layer, i32 sequence )
{
    return util::call_virtual< float( ATLAS_THISCALL * )( void *, C_AnimationLayer *, i32 ) >(
        this, g_csgo.m_cs_player_get_layer_sequence_cycle_rate )( this, layer, sequence );
}

const model_t *C_BaseAnimating::GetModel( )
{
    IClientRenderable *renderable{ this->GetClientRenderable( ) };
    return util::call_virtual< const model_t *( ATLAS_THISCALL * )( void * ) >(
        renderable, g_csgo.m_client_renderable_get_model )( renderable );
}

bool C_BaseAnimating::SetupBones( matrix3x4 *bone_to_world, i32 max_bones, i32 bone_mask, float current_time )
{
    IClientRenderable *renderable{ this->GetClientRenderable( ) };
    return util::call_virtual< bool( ATLAS_THISCALL * )( void *, matrix3x4 *, i32, i32, float ) >(
        renderable, g_csgo.m_client_renderable_setup_bones )( renderable, bone_to_world, max_bones, bone_mask, current_time );
}

bool C_BaseAnimating::BaseSetupBones( matrix3x4 *bone_to_world, i32 max_bones, i32 bone_mask, float current_time )
{
    return g_csgo.m_C_BaseAnimating_SetupBones.get_as< bool( ATLAS_THISCALL * )( IClientRenderable *, matrix3x4 *, i32, i32, float ) >( )(
        this->GetClientRenderable( ), bone_to_world, max_bones, bone_mask, current_time );
}

i32 C_BaseAnimating::LookupSequence( const char *label )
{
    return g_csgo.m_C_BaseAnimating_LookupSequence.get_as< i32( ATLAS_THISCALL * )( C_BaseAnimating *, const char * ) >( )( this, label );
}

i32 C_BaseAnimating::LookupBone( const char *name )
{
    return g_csgo.m_C_BaseAnimating_LookupBone.get_as< i32( ATLAS_THISCALL * )( C_BaseAnimating *, const char * ) >( )( this, name );
}

void CCSGOPlayerAnimState::Update( float eye_yaw, float eye_pitch )
{
    g_csgo.m_CCSGOPlayerAnimState_Update.get_as< void( ATLAS_VECTORCALL * )( CCSGOPlayerAnimState *, i32, float, float, float, i32 ) >( )(
        this, 0, 0.0f, eye_yaw, eye_pitch, 0 );
}

const char *CCSGOPlayerAnimState::GetWeaponPrefix( )
{
    return g_csgo.m_CCSGOPlayerAnimState_GetWeaponPrefix.get_as< const char *( ATLAS_THISCALL * )( CCSGOPlayerAnimState * ) >( )( this );
}

void CCSGOPlayerAnimState::UpdateLayerOrderPreset( i32 layer, i32 sequence )
{
    g_csgo.m_CCSGOPlayerAnimState_UpdateLayerOrderPreset.get_as< void( ATLAS_THISCALL * )( CCSGOPlayerAnimState *, i32, i32 ) >( )( this, layer, sequence );
}
