#pragma once

#include "netvars.hpp"

class C_BaseEntity : public IClientEntity
{
public:
    NETVAR( i32, m_iTeamNum, "DT_BaseEntity", "m_iTeamNum" )
    NETVAR( vec3, m_vecMins, "DT_BaseEntity", "m_vecMins" )
    NETVAR( vec3, m_vecMaxs, "DT_BaseEntity", "m_vecMaxs" )
    CLASS_VAR( EHANDLE, m_pMoveParent, 0x300 )
    NETVAR( vec3, m_vecOrigin, "DT_BaseEntity", "m_vecOrigin" )
    NETVAR( i16, m_nModelIndex, "DT_BaseEntity", "m_nModelIndex" )
    NETVAR( EHANDLE, m_hOwnerEntity, "DT_BaseEntity", "m_hOwnerEntity" )
    NETVAR( float, m_flSimulationTime, "DT_BaseEntity", "m_flSimulationTime" )

    CLASS_VAR( i32, m_index, 0x64 )

    void SetModelIndex( i32 index );

    const vec3   &GetAbsOrigin( );
    const vec3   &WorldSpaceCenter( );
    void          SetAbsOrigin( const vec3 &origin );
    void          SetAbsAngles( const vec3 &angles );
    bool          IsPlayer( );
    C_BaseEntity *MyCombatCharacterPointer( );
    bool          IsDormant( );
    datamap_t    *GetPredDescMap( );
    i32           entindex( );
    void          ShutdownPredictable( );

    CLASS_VAR( u8 *, m_pOriginalData, 0x928 )
};

class C_BaseCombatWeapon : public C_BaseEntity
{
public:
    NETVAR( i16, m_iItemDefinitionIndex, "DT_BaseAttributableItem", "m_iItemDefinitionIndex" )
    NETVAR( i32, m_iViewModelIndex, "DT_BaseCombatWeapon", "m_iViewModelIndex" )
    NETVAR( i32, m_iWorldModelIndex, "DT_BaseCombatWeapon", "m_iWorldModelIndex" )
    NETVAR( EHANDLE, m_hWeaponWorldModel, "DT_BaseCombatWeapon", "m_hWeaponWorldModel" )
    NETVAR( float, m_flNextPrimaryAttack, "DT_BaseCombatWeapon", "m_flNextPrimaryAttack" )
    NETVAR( float, m_flNextSecondaryAttack, "DT_BaseCombatWeapon", "m_flNextSecondaryAttack" )
    NETVAR( i32, m_iClip1, "DT_BaseCombatWeapon", "m_iClip1" )
    NETVAR( i32, m_iPrimaryReserveAmmoCount, "DT_BaseCombatWeapon", "m_iPrimaryReserveAmmoCount" )
    NETVAR( float, m_flRecoilIndex, "DT_WeaponCSBase", "m_flRecoilIndex" )
    NETVAR( i32, m_weaponMode, "DT_WeaponCSBase", "m_weaponMode" )
    NETVAR( float, m_fAccuracyPenalty, "DT_WeaponCSBase", "m_fAccuracyPenalty" )
    NETVAR( float, m_fLastShotTime, "DT_WeaponCSBase", "m_fLastShotTime" )
    NETVAR( float, m_flPostponeFireReadyTime, "DT_WeaponCSBase", "m_flPostponeFireReadyTime" )
    NETVAR( i32, m_zoomLevel, "DT_WeaponCSBaseGun", "m_zoomLevel" )
    NETVAR( float, m_fThrowTime, "DT_BaseCSGrenade", "m_fThrowTime" )

    CLASS_VAR( i32, m_Activity, 0x3234 )
    CLASS_VAR( float, m_flLastClientFireBulletTime, 0x3328 )

    const CCSWeaponInfo *GetCSWpnData( );
    float                GetSpread( );
    float                GetInaccuracy( );
    void                 UpdateAccuracyPenalty( );
    bool                 IsFullAuto( );
};

class C_BaseAnimating : public C_BaseEntity
{
public:
    NETVAR( i32, m_nBody, "DT_BaseAnimating", "m_nBody" )
    NETVAR( i32, m_nHitboxSet, "DT_BaseAnimating", "m_nHitboxSet" )
    NETVAR_PTR( float, m_flPoseParameter, "DT_BaseAnimating", "m_flPoseParameter" )

    CLASS_VAR( i32, m_nCustomBlendingRuleMask, 0xA24 )
    CLASS_VAR( i32, m_nAnimLODflags, 0xA28 )
    CLASS_VAR( i32, m_nLastNonSkippedFrame, 0xA5C )
    CLASS_VAR( i32, m_iMostRecentModelBoneCounter, 0x2680 )
    CLASS_VAR( matrix3x4 *, m_CachedBoneData, 0x2900 )
    CLASS_VAR( i32, m_CachedBoneCount, 0x290C )
    CLASS_VAR( float, m_flLastBoneSetupTime, 0x2914 )
    CLASS_VAR( CStudioHdr *, m_pStudioHdr, 0x293C )
    CLASS_VAR( anim_overlay_t, m_AnimOverlay, 0x2970 )

    const model_t *GetModel( );
    bool           SetupBones( matrix3x4 *bone_to_world, i32 max_bones, i32 bone_mask, float current_time );
    bool           BaseSetupBones( matrix3x4 *bone_to_world, i32 max_bones, i32 bone_mask, float current_time );
    i32            LookupSequence( const char *label );
    i32            LookupBone( const char *name );
};

class C_BasePlayer : public C_BaseAnimating
{
public:
    NETVAR( i32, m_iHealth, "DT_BasePlayer", "m_iHealth" )
    NETVAR( u8, m_lifeState, "DT_BasePlayer", "m_lifeState" )
    NETVAR( CHandle< C_BaseCombatWeapon >, m_hActiveWeapon, "DT_BaseCombatCharacter", "m_hActiveWeapon" )
    NETVAR( i32, m_fFlags, "DT_BasePlayer", "m_fFlags" )
    NETVAR( vec3, m_vecVelocity, "DT_BasePlayer", "m_vecVelocity[0]" )
    NETVAR( vec3, m_vecViewOffset, "DT_BasePlayer", "m_vecViewOffset[0]" )
    NETVAR( float, m_flMaxspeed, "DT_BasePlayer", "m_flMaxspeed" )
    NETVAR( u8, m_nWaterLevel, "DT_BasePlayer", "m_nWaterLevel" )
    NETVAR( vec3, m_viewPunchAngle, "DT_BasePlayer", "m_viewPunchAngle" )
    NETVAR( vec3, m_aimPunchAngle, "DT_BasePlayer", "m_aimPunchAngle" )
    NETVAR( i32, m_iDefaultFOV, "DT_BasePlayer", "m_iDefaultFOV" )
    NETVAR( i32, m_iFOV, "DT_BasePlayer", "m_iFOV" )
    NETVAR( i32, m_iFOVStart, "DT_BasePlayer", "m_iFOVStart" )
    NETVAR( EHANDLE, m_hViewModel, "DT_BasePlayer", "m_hViewModel[0]" )
    NETVAR( i32, m_nTickBase, "DT_BasePlayer", "m_nTickBase" )
    NETVAR( float, m_flLaggedMovementValue, "DT_BasePlayer", "m_flLaggedMovementValue" )
    NETVAR( float, m_flNextAttack, "DT_BaseCombatCharacter", "m_flNextAttack" )
    NETVAR_PTR( EHANDLE, m_hMyWeapons, "DT_BaseCombatCharacter", "m_hMyWeapons" )
    NETVAR_PTR( EHANDLE, m_hMyWearables, "DT_BaseCombatCharacter", "m_hMyWearables" )

    CLASS_VAR( u8, m_MoveType, 0x258 )
    CLASS_VAR( float, m_surfaceFriction, 0x3210 )

    CLASS_VAR( CUserCmd *, m_pCurrentCommand, 0x3314 )
    CLASS_VAR( vec3, m_vecPreviouslyPredictedOrigin, 0x357C )

    void ItemPostFrame( );

    bool IsAlive( )
    {
        return this->m_lifeState( ) == LIFE_ALIVE;
    }

    bool IsOnGround( )
    {
        return ( this->m_fFlags( ) & FL_ONGROUND ) != 0;
    }
};

class CCSGOPlayerAnimState
{
public:
    void        Update( float eye_yaw, float eye_pitch );
    const char *GetWeaponPrefix( );
    void        UpdateLayerOrderPreset( i32 layer, i32 sequence );

    CLASS_VAR( bool, m_bFirstRunSinceInit, 0x4 )
    CLASS_VAR( void *, m_pWeapon, 0x64 )
    CLASS_VAR( void *, m_pWeaponLast, 0x68 )
    CLASS_VAR( float, m_flLastUpdateTime, 0x6C )
    CLASS_VAR( i32, m_nLastUpdateFrame, 0x70 )
    CLASS_VAR( float, m_flLastUpdateIncrement, 0x74 )
    CLASS_VAR( float, m_flEyeYaw, 0x78 )
    CLASS_VAR( float, m_flFootYaw, 0x80 )
    CLASS_VAR( float, m_flFootYawLast, 0x84 )
    CLASS_VAR( float, m_flPrimaryCycle, 0x98 )
    CLASS_VAR( float, m_flMoveWeight, 0x9C )
    CLASS_VAR( float, m_flAnimDuckAmount, 0xA4 )
    CLASS_VAR( vec3, m_vecVelocityNormalizedNonZero, 0xE0 )
    CLASS_VAR( float, m_flVelocityLengthXY, 0xEC )
    CLASS_VAR( float, m_flVelocityLengthZ, 0xF0 )
    CLASS_VAR( float, m_flSpeedAsPortionOfRunTopSpeed, 0xF4 )
    CLASS_VAR( float, m_flSpeedAsPortionOfWalkTopSpeed, 0xF8 )
    CLASS_VAR( float, m_flSpeedAsPortionOfCrouchTopSpeed, 0xFC )
    CLASS_VAR( float, m_flDurationStill, 0x104 )
    CLASS_VAR( bool, m_bOnGround, 0x108 )
    CLASS_VAR( bool, m_bLanding, 0x109 )
    CLASS_VAR( float, m_flDurationInAir, 0x110 )
    CLASS_VAR( bool, m_bOnLadder, 0x128 )
    CLASS_VAR( float, m_flLadderWeight, 0x12C )
    CLASS_VAR( float, m_flStutterStep, 0x2AC )
    CLASS_VAR( i32, m_nAnimstateModelVersion, 0x340 )
};

class C_CSPlayer : public C_BasePlayer
{
public:
    NETVAR( vec3, m_angEyeAngles, "DT_CSPlayer", "m_angEyeAngles[0]" )
    NETVAR( float, m_flLowerBodyYawTarget, "DT_CSPlayer", "m_flLowerBodyYawTarget" )
    NETVAR( float, m_flThirdpersonRecoil, "DT_CSPlayer", "m_flThirdpersonRecoil" )
    NETVAR( i32, m_ArmorValue, "DT_CSPlayer", "m_ArmorValue" )
    NETVAR( bool, m_bHasHelmet, "DT_CSPlayer", "m_bHasHelmet" )
    NETVAR( bool, m_bHasHeavyArmor, "DT_CSPlayer", "m_bHasHeavyArmor" )
    NETVAR( bool, m_bIsScoped, "DT_CSPlayer", "m_bIsScoped" )
    NETVAR( bool, m_bIsDefusing, "DT_CSPlayer", "m_bIsDefusing" )
    NETVAR( bool, m_bHasDefuser, "DT_CSPlayer", "m_bHasDefuser" )
    NETVAR( bool, m_bGunGameImmunity, "DT_CSPlayer", "m_bGunGameImmunity" )
    NETVAR( i32, m_iAccount, "DT_CSPlayer", "m_iAccount" )
    NETVAR( float, m_flFlashDuration, "DT_CSPlayer", "m_flFlashDuration" )
    NETVAR( float, m_flFlashMaxAlpha, "DT_CSPlayer", "m_flFlashMaxAlpha" )
    NETVAR( bool, m_bStrafing, "DT_CSPlayer", "m_bStrafing" )
    NETVAR( i32, m_iShotsFired, "DT_CSPlayer", "m_iShotsFired" )
    NETVAR( bool, m_bWaitForNoAttack, "DT_CSPlayer", "m_bWaitForNoAttack" )
    NETVAR( bool, m_bResumeZoom, "DT_CSPlayer", "m_bResumeZoom" )

    CLASS_VAR( CCSGOPlayerAnimState *, m_PlayerAnimState, 0x3874 )

    vec3  GetAimPunchAngleScaled( );
    vec3  Weapon_ShootPosition( );
    bool  IsOtherEnemy( C_CSPlayer *other );
    float GetLayerSequenceCycleRate( C_AnimationLayer *layer, i32 sequence );
    float GetMaxSpeed( C_BaseCombatWeapon *weapon );
};

class C_EconEntity : public C_BaseEntity
{
public:
    NETVAR( i16, m_iItemDefinitionIndex, "DT_BaseAttributableItem", "m_iItemDefinitionIndex" )
    NETVAR( i32, m_iEntityQuality, "DT_BaseAttributableItem", "m_iEntityQuality" )
    NETVAR( i32, m_iItemIDHigh, "DT_BaseAttributableItem", "m_iItemIDHigh" )
    NETVAR( i32, m_iAccountID, "DT_BaseAttributableItem", "m_iAccountID" )
    NETVAR( bool, m_bInitialized, "DT_BaseAttributableItem", "m_bInitialized" )
    NETVAR_PTR( char, m_szCustomName, "DT_BaseAttributableItem", "m_szCustomName" )
    NETVAR( u32, m_OriginalOwnerXuidLow, "DT_BaseAttributableItem", "m_OriginalOwnerXuidLow" )
    NETVAR( u32, m_OriginalOwnerXuidHigh, "DT_BaseAttributableItem", "m_OriginalOwnerXuidHigh" )
    NETVAR( i32, m_nFallbackPaintKit, "DT_BaseAttributableItem", "m_nFallbackPaintKit" )
    NETVAR( i32, m_nFallbackSeed, "DT_BaseAttributableItem", "m_nFallbackSeed" )
    NETVAR( float, m_flFallbackWear, "DT_BaseAttributableItem", "m_flFallbackWear" )
    NETVAR( i32, m_nFallbackStatTrak, "DT_BaseAttributableItem", "m_nFallbackStatTrak" )
    NETVAR_PTR( u8, m_AttributeManager, "DT_BaseAttributableItem", "m_AttributeManager" )
    NETVAR_PTR( u8, m_Item, "DT_BaseAttributableItem", "m_Item" )

    bool &m_bFallbackApplied( )
    {
        return *reinterpret_cast< bool * >( this->m_AttributeManager( ) - 4 );
    }

    void UpdateFallbackAttributes( );
};

class C_BaseViewModel : public C_BaseEntity
{
public:
    NETVAR( EHANDLE, m_hWeapon, "DT_BaseViewModel", "m_hWeapon" )
    NETVAR( EHANDLE, m_hOwner, "DT_BaseViewModel", "m_hOwner" )

    void UpdateAllViewmodelAddons( );
    void RemoveViewmodelArmModels( );
};

class C_PlayerResource : public C_BaseEntity
{
public:
    NETVAR_PTR( bool, m_bConnected, "DT_PlayerResource", "m_bConnected" )
    NETVAR_PTR( i32, m_iTeam, "DT_PlayerResource", "m_iTeam" )
    NETVAR_PTR( bool, m_bAlive, "DT_PlayerResource", "m_bAlive" )
};
