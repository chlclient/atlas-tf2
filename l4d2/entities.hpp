#pragma once

#include "netvars.hpp"

class CTerrorWeaponInfo
{
public:
    CLASS_VAR( float, m_flRange, 0x9D0 )
    CLASS_VAR( i32, m_iBullets, 0x9D8 )
    CLASS_VAR( float, m_flSpreadPerShot, 0xC10 )
    CLASS_VAR( float, m_flMaxSpread, 0xC14 )
    CLASS_VAR( float, m_flSpreadDecay, 0xC18 )
    CLASS_VAR( float, m_flMinDuckingSpread, 0xC1C )
    CLASS_VAR( float, m_flMinStandingSpread, 0xC20 )
    CLASS_VAR( float, m_flMinInAirSpread, 0xC24 )
    CLASS_VAR( float, m_flMaxMovementSpread, 0xC28 )
    CLASS_VAR( float, m_flPelletScatterPitch, 0xC2C )
    CLASS_VAR( float, m_flPelletScatterYaw, 0xC30 )
};

class C_BaseEntity : public IClientEntity
{
public:
    NETVAR( vec3, m_vecOrigin, "DT_BaseEntity", "m_vecOrigin" )
    NETVAR( vec3, m_angRotation, "DT_BaseEntity", "m_angRotation" )
    NETVAR( i32, m_iTeamNum, "DT_BaseEntity", "m_iTeamNum" )
    NETVAR( i32, m_CollisionGroup, "DT_BaseEntity", "m_CollisionGroup" )
    NETVAR( float, m_flSimulationTime, "DT_BaseEntity", "m_flSimulationTime" )
    NETVAR( EHANDLE, m_hOwnerEntity, "DT_BaseEntity", "m_hOwnerEntity" )
    NETVAR( EHANDLE, m_hMoveParent, "DT_BaseEntity", "moveparent" )
    NETVAR( i16, m_nModelIndex, "DT_BaseEntity", "m_nModelIndex" )
    NETVAR( vec3, m_vecMins, "DT_BaseEntity", "m_vecMins" )
    NETVAR( vec3, m_vecMaxs, "DT_BaseEntity", "m_vecMaxs" )
    NETVAR( u8, m_nSolidType, "DT_BaseEntity", "m_nSolidType" )
    NETVAR( u16, m_usSolidFlags, "DT_BaseEntity", "m_usSolidFlags" )
    NETVAR( color_t, m_clrRender, "DT_BaseEntity", "m_clrRender" )
    NETVAR( i32, m_fEffects, "DT_BaseEntity", "m_fEffects" )

    CLASS_VAR( u8 *, m_pOriginalData, 0x5DC )
    u8 **m_pIntermediateData( )
    {
        return reinterpret_cast< u8 ** >( reinterpret_cast< u8 * >( this ) + 0x384u );
    }

    DATAMAP_VAR( vec3, m_vecAbsOrigin )
    DATAMAP_VAR( vec3, m_vecAbsVelocity )
    DATAMAP_VAR( vec3, m_vecAngVelocity )
    DATAMAP_VAR( i32, m_iEFlags )
    DATAMAP_VAR( u8, m_MoveType )
    DATAMAP_VAR( f32, m_flGravity )

    datamap_t   *GetPredDescMap( );
    ClientClass *GetClientClass( );
    bool         IsDormant( );
    i32          entindex( );
};

class C_BaseAnimating : public C_BaseEntity
{
public:
    NETVAR( i32, m_nSequence, "DT_BaseAnimating", "m_nSequence" )
    NETVAR( i32, m_nHitboxSet, "DT_BaseAnimating", "m_nHitboxSet" )
    NETVAR( float, m_flCycle, "DT_BaseAnimating", "m_flCycle" )
    NETVAR( float, m_flPlaybackRate, "DT_BaseAnimating", "m_flPlaybackRate" )
    NETVAR( float, m_flModelScale, "DT_BaseAnimating", "m_flModelScale" )
    NETVAR( i32, m_nSkin, "DT_BaseAnimating", "m_nSkin" )
    NETVAR( bool, m_bClientSideRagdoll, "DT_BaseAnimating", "m_bClientSideRagdoll" )
    NETVAR_PTR( float, m_flPoseParameter, "DT_BaseAnimating", "m_flPoseParameter" )
    NETVAR_PTR( float, m_flEncodedController, "DT_BaseAnimating", "m_flEncodedController" )

    i32          GetSequenceActivity( i32 sequence );
    studiohdr_t *GetStudioHdr( );
    bool         SetupBones( matrix3x4 *bone_to_world, i32 bone_mask );
};

class C_BaseAnimatingOverlay : public C_BaseAnimating
{
public:
};

class C_BaseFlex : public C_BaseAnimatingOverlay
{
public:
};

class C_BaseCombatCharacter : public C_BaseFlex
{
public:
    NETVAR( EHANDLE, m_hActiveWeapon, "DT_BaseCombatCharacter", "m_hActiveWeapon" )
    NETVAR( float, m_flNextAttack, "DT_BaseCombatCharacter", "m_flNextAttack" )
    NETVAR_PTR( EHANDLE, m_hMyWeapons, "DT_BaseCombatCharacter", "m_hMyWeapons" )
};

class C_BasePlayer : public C_BaseCombatCharacter
{
public:
    NETVAR( vec3, m_vecViewOffset, "DT_BasePlayer", "m_vecViewOffset[0]" )
    NETVAR( vec3, m_vecPunchAngle, "DT_BasePlayer", "m_vecPunchAngle" )
    NETVAR( vec3, m_vecVelocity, "DT_BasePlayer", "m_vecVelocity[0]" )
    NETVAR( vec3, m_vecBaseVelocity, "DT_BasePlayer", "m_vecBaseVelocity" )
    NETVAR( i32, m_iHealth, "DT_BasePlayer", "m_iHealth" )
    NETVAR( i32, m_iObserverMode, "DT_BasePlayer", "m_iObserverMode" )
    NETVAR( EHANDLE, m_hObserverTarget, "DT_BasePlayer", "m_hObserverTarget" )
    NETVAR( u8, m_lifeState, "DT_BasePlayer", "m_lifeState" )
    NETVAR( i32, m_fFlags, "DT_BasePlayer", "m_fFlags" )
    NETVAR( EHANDLE, m_hGroundEntity, "DT_BasePlayer", "m_hGroundEntity" )
    NETVAR( i32, m_nTickBase, "DT_BasePlayer", "m_nTickBase" )
    NETVAR( i32, m_iAmmo, "DT_BasePlayer", "m_iAmmo" )
    NETVAR( float, m_flMaxspeed, "DT_BasePlayer", "m_flMaxspeed" )
    NETVAR( i32, m_iFOV, "DT_BasePlayer", "m_iFOV" )
    NETVAR( i32, m_iDefaultFOV, "DT_BasePlayer", "m_iDefaultFOV" )
    NETVAR( EHANDLE, m_hViewModel, "DT_BasePlayer", "m_hViewModel[0]" )
    NETVAR( float, m_flStepSize, "DT_BasePlayer", "m_flStepSize" )
    NETVAR( u8, m_nWaterLevel, "DT_BasePlayer", "m_nWaterLevel" )

    DATAMAP_VAR( f32, m_surfaceFriction )
    DATAMAP_VAR( i32, m_nButtons )
    DATAMAP_VAR( i32, m_afButtonLast )
    DATAMAP_VAR( i32, m_afButtonPressed )
    DATAMAP_VAR( i32, m_afButtonReleased )

    bool IsAlive( );
    bool IsOnGround( );
    bool IsDucked( );
    void ItemPostFrame( );
};

class C_CSPlayer : public C_BasePlayer
{
public:
    NETVAR( vec3, m_angEyeAngles, "DT_CSPlayer", "m_angEyeAngles[0]" )
    DATAMAP_VAR( i32, m_iShotsFired )
};

class C_TerrorPlayer : public C_CSPlayer
{
public:
    NETVAR( i32, m_zombieClass, "DT_TerrorPlayer", "m_zombieClass" )
    NETVAR( i32, m_survivorCharacter, "DT_TerrorPlayer", "m_survivorCharacter" )
    NETVAR( i32, m_iMaxHealth, "DT_TerrorPlayer", "m_iMaxHealth" )
    NETVAR( float, m_healthBuffer, "DT_TerrorPlayer", "m_healthBuffer" )
    NETVAR( float, m_healthBufferTime, "DT_TerrorPlayer", "m_healthBufferTime" )
    NETVAR( i32, m_currentReviveCount, "DT_TerrorPlayer", "m_currentReviveCount" )
    NETVAR( bool, m_bIsOnThirdStrike, "DT_TerrorPlayer", "m_bIsOnThirdStrike" )
    NETVAR( bool, m_isIncapacitated, "DT_TerrorPlayer", "m_isIncapacitated" )
    NETVAR( bool, m_isHangingFromLedge, "DT_TerrorPlayer", "m_isHangingFromLedge" )
    NETVAR( bool, m_isGhost, "DT_TerrorPlayer", "m_isGhost" )
    NETVAR( bool, m_bAdrenalineActive, "DT_TerrorPlayer", "m_bAdrenalineActive" )
    NETVAR( EHANDLE, m_customAbility, "DT_TerrorPlayer", "m_customAbility" )
    NETVAR( EHANDLE, m_tongueOwner, "DT_TerrorPlayer", "m_tongueOwner" )
    NETVAR( EHANDLE, m_pounceAttacker, "DT_TerrorPlayer", "m_pounceAttacker" )
    NETVAR( EHANDLE, m_jockeyAttacker, "DT_TerrorPlayer", "m_jockeyAttacker" )
    NETVAR( EHANDLE, m_pummelAttacker, "DT_TerrorPlayer", "m_pummelAttacker" )
    NETVAR( EHANDLE, m_carryAttacker, "DT_TerrorPlayer", "m_carryAttacker" )

    bool IsPinned( );
    bool IsIncapacitated( );
};

class C_NextBotCombatCharacter : public C_BaseCombatCharacter
{
public:
};

class C_Infected : public C_NextBotCombatCharacter
{
public:
    NETVAR( bool, m_bIsBurning, "DT_Infected", "m_bIsBurning" )

    bool IsAlive( );
};

class C_Witch : public C_Infected
{
public:
};

class C_BaseCombatWeapon : public C_BaseAnimating
{
public:
    NETVAR( i32, m_iClip1, "DT_BaseCombatWeapon", "m_iClip1" )
    NETVAR( i32, m_iPrimaryAmmoType, "DT_BaseCombatWeapon", "m_iPrimaryAmmoType" )
    NETVAR( float, m_flNextPrimaryAttack, "DT_BaseCombatWeapon", "m_flNextPrimaryAttack" )
    NETVAR( float, m_flNextSecondaryAttack, "DT_BaseCombatWeapon", "m_flNextSecondaryAttack" )
    NETVAR( float, m_flTimeWeaponIdle, "DT_BaseCombatWeapon", "m_flTimeWeaponIdle" )
    NETVAR( bool, m_bInReload, "DT_BaseCombatWeapon", "m_bInReload" )
    NETVAR( EHANDLE, m_hOwner, "DT_BaseCombatWeapon", "m_hOwner" )

    NETVAR_OFFSET( u16, m_hWeaponFileInfo, "DT_BaseCombatWeapon", "m_bInReload", 0x1D )

    CTerrorWeaponInfo *GetWeaponInfo( );
    bool               IsTerrorGun( );
};

class C_TerrorGun : public C_BaseCombatWeapon
{
public:
    NETVAR( u32, m_upgradeBitVec, "DT_TerrorGun", "m_upgradeBitVec" )
    NETVAR_OFFSET( float, m_flSpread, "DT_TerrorGun", "m_upgradeBitVec", 0x1C )

    CLASS_VAR( bool, m_bFireAllowed, 0xA59 )
    CLASS_VAR( bool, m_bHoldingFire, 0xA5A )

    bool HasUpgrade( u32 bit )
    {
        return ( m_upgradeBitVec( ) & ( 1u << bit ) ) != 0;
    }

    bool IsFullyAutomatic( );
};

class C_Rifle_Desert : public C_TerrorGun
{
public:
    CLASS_VAR( float, m_flBurstShot1, 0xD40 )
    CLASS_VAR( float, m_flBurstShot2, 0xD44 )
    CLASS_VAR( float, m_flBurstResetTime, 0xD48 )
};
