#include "entities.hpp"

#include "global.hpp"

datamap_t *C_BaseEntity::GetPredDescMap( )
{
    return util::call_virtual< datamap_t *( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_base_entity_get_pred_desc_map )( this );
}

ClientClass *C_BaseEntity::GetClientClass( )
{
    IClientNetworkable *networkable{ this->GetClientNetworkable( ) };
    return util::call_virtual< ClientClass *( ATLAS_THISCALL * )( void * ) >(
        networkable, g_l4d2.m_client_networkable_get_client_class )( networkable );
}

bool C_BaseEntity::IsDormant( )
{
    IClientNetworkable *networkable{ this->GetClientNetworkable( ) };
    return util::call_virtual< bool( ATLAS_THISCALL * )( void * ) >(
        networkable, g_l4d2.m_client_networkable_is_dormant )( networkable );
}

i32 C_BaseEntity::entindex( )
{
    IClientNetworkable *networkable{ this->GetClientNetworkable( ) };
    return util::call_virtual< i32( ATLAS_THISCALL * )( void * ) >(
        networkable, g_l4d2.m_client_networkable_entindex )( networkable );
}

i32 C_BaseAnimating::GetSequenceActivity( i32 sequence )
{
    return g_l4d2.m_CBaseAnimating_GetSequenceActivity.get_as< i32( ATLAS_THISCALL * )( void *, i32 ) >( )( this, sequence );
}

studiohdr_t *C_BaseAnimating::GetStudioHdr( )
{
    return g_l4d2.m_model_info->GetStudiomodel( GetClientRenderable( )->GetModel( ) );
}

bool C_BaseAnimating::SetupBones( matrix3x4 *bone_to_world, i32 bone_mask )
{
    return GetClientRenderable( )->SetupBones( bone_to_world, MAXSTUDIOBONES, bone_mask, g_l4d2.m_globals->curtime );
}

bool C_BasePlayer::IsAlive( )
{
    return m_lifeState( ) == LIFE_ALIVE;
}

bool C_BasePlayer::IsOnGround( )
{
    return m_fFlags( ) & FL_ONGROUND;
}

bool C_BasePlayer::IsDucked( )
{
    return m_fFlags( ) & FL_DUCKING;
}

void C_BasePlayer::ItemPostFrame( )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_base_player_item_post_frame )( this );
}

bool C_TerrorPlayer::IsPinned( )
{
    return m_tongueOwner( ).IsValid( ) ||
           m_pounceAttacker( ).IsValid( ) ||
           m_jockeyAttacker( ).IsValid( ) ||
           m_pummelAttacker( ).IsValid( ) ||
           m_carryAttacker( ).IsValid( );
}

bool C_TerrorPlayer::IsIncapacitated( )
{
    return IsAlive( ) && m_isIncapacitated( );
}

CTerrorWeaponInfo *C_BaseCombatWeapon::GetWeaponInfo( )
{
    return g_l4d2.m_GetFileWeaponInfoFromHandle.call_as< CTerrorWeaponInfo * >(
        static_cast< u32 >( m_hWeaponFileInfo( ) ) );
}

bool C_BaseCombatWeapon::IsTerrorGun( )
{
    switch ( GetClientClass( )->GetClassID( ) )
    {
        case EL4D2ClassID::CAssaultRifle:
        case EL4D2ClassID::CAutoShotgun:
        case EL4D2ClassID::CBaseAutoShotgun:
        case EL4D2ClassID::CBaseRifle:
        case EL4D2ClassID::CBaseShotgun:
        case EL4D2ClassID::CBaseSniperRifle:
        case EL4D2ClassID::CMagnumPistol:
        case EL4D2ClassID::CPistol:
        case EL4D2ClassID::CPumpShotgun:
        case EL4D2ClassID::CRifle_AK47:
        case EL4D2ClassID::CRifle_Desert:
        case EL4D2ClassID::CRifle_M60:
        case EL4D2ClassID::CRifle_SG552:
        case EL4D2ClassID::CShotgun_Chrome:
        case EL4D2ClassID::CShotgun_SPAS:
        case EL4D2ClassID::CSMG_MP5:
        case EL4D2ClassID::CSMG_Silenced:
        case EL4D2ClassID::CSniper_AWP:
        case EL4D2ClassID::CSniper_Military:
        case EL4D2ClassID::CSniper_Scout:
        case EL4D2ClassID::CSniperRifle:
        case EL4D2ClassID::CSubMachinegun:
        case EL4D2ClassID::CTerrorGun: return true;

        default: return false;
    }
}

bool C_TerrorGun::IsFullyAutomatic( )
{
    return util::call_virtual< bool( ATLAS_THISCALL * )( void * ) >(
        this, g_l4d2.m_terror_weapon_is_fully_automatic )( this );
}

bool C_Infected::IsAlive( )
{
    if ( m_bClientSideRagdoll( ) ||
         m_nSolidType( ) == SOLID_NONE ||
         ( m_usSolidFlags( ) & FSOLID_NOT_SOLID ) != 0 )
    {
        return false;
    }

    const i32 activity{ GetSequenceActivity( m_nSequence( ) ) };
    return activity < ACT_TERROR_DIE_FROM_STAND || activity > ACT_SLICING_DEATH_RIGHT;
}
