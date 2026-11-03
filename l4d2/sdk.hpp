#pragma once

#include "base.hpp"

class bf_write
{
public:
    void StartWriting( void *data, i32 bytes, i32 start_bit = 0, i32 max_bits = -1 )
    {
        m_pData      = static_cast< unsigned long * >( data );
        m_nDataBytes = bytes & ~3;
        m_nDataBits  = max_bits == -1 ? m_nDataBytes << 3 : max_bits;
        m_iCurBit    = start_bit;
        m_bOverflow  = false;
    }

    unsigned long *m_pData{ };
    i32            m_nDataBytes{ };
    i32            m_nDataBits{ -1 };
    i32            m_iCurBit{ };
    bool           m_bOverflow{ };
    bool           m_bAssertOnOverflow{ true };
    const char    *m_pDebugName{ };
};

enum struct EL4D2ClassID : int
{
    Boomer,
    CAssaultRifle,
    CAutoShotgun,
    CBaseAbility,
    CBaseAnimating,
    CBaseAnimatingOverlay,
    CBaseAutoShotgun,
    CBaseBackpackItem,
    CBaseBeltItem,
    CBaseButton,
    CBaseCombatCharacter,
    CBaseCombatWeapon,
    CBaseCSGrenade,
    CBaseCSGrenadeProjectile,
    CBaseDoor,
    CBaseEntity,
    CBaseFlex,
    CBaseGrenade,
    CBaseMountedWeapon,
    CBaseParticleEntity,
    CBasePlayer,
    CBasePropDoor,
    CBaseRifle,
    CBaseShotgun,
    CBaseSniperRifle,
    CBaseTeamObjectiveResource,
    CBaseTempEntity,
    CBaseToggle,
    CBaseTrigger,
    CBaseUpgradeItem,
    CBaseViewModel,
    CBeam,
    CBeamSpotlight,
    CBoneFollower,
    CBoomerClaw,
    CBreakable,
    CBreakableProp,
    CBreakableSurface,
    CButtonTimed,
    CChainsaw,
    CCharge,
    CChargerClaw,
    CClaw,
    CClientPath,
    CColaBottles,
    CColorCorrection,
    CColorCorrectionVolume,
    CCSGameRulesProxy,
    CCSPlayer,
    CCSRagdoll,
    CCSTeam,
    CDynamicLight,
    CDynamicProp,
    CEmbers,
    CEntityDissolve,
    CEntityFlame,
    CEntityParticleTrail,
    CEnvDetailController,
    CEnvDOFController,
    CEnvParticleScript,
    CEnvPhysicsBlocker,
    CEnvProjectedTexture,
    CEnvQuadraticBeam,
    CEnvScreenEffect,
    CEnvScreenOverlay,
    CEnvTonemapController,
    CEnvWeaponfire,
    CEnvWind,
    CFinaleTrigger,
    CFireCrackerBlast,
    CFireSmoke,
    CFireTrail,
    CFireworkCrate,
    CFirstAidKit,
    CFish,
    CFogController,
    CFunc_Dust,
    CFunc_LOD,
    CFuncAreaPortalWindow,
    CFuncBrush,
    CFuncConveyor,
    CFuncElevator,
    CFuncLadder,
    CFuncMoveLinear,
    CFuncOccluder,
    CFuncPlayerGhostInfectedClip,
    CFuncPlayerInfectedClip,
    CFuncRagdollFader,
    CFuncReflectiveGlass,
    CFuncRotating,
    CFuncSimpleLadder,
    CFuncSmokeVolume,
    CFuncTrackTrain,
    CGameRulesProxy,
    CGasCan,
    CGnome,
    CGrenadeLauncher,
    CGrenadeLauncher_Projectile,
    CHandleTest,
    Charger,
    CHunterClaw,
    CInferno,
    CInfoLadderDismount,
    CInfoOverlayAccessor,
    CInsectSwarm,
    CItem_Adrenaline,
    CItem_VomitJar,
    CItemAmmoPack,
    CItemBaseUpgradePack,
    CItemDefibrillator,
    CItemUpgradePackExplosive,
    CItemUpgradePackIncendiary,
    CJockeyClaw,
    CLeap,
    CLightGlow,
    CLunge,
    CMagnumPistol,
    CMaterialModifyControl,
    CMolotov,
    CMolotovProjectile,
    COxygenTank,
    CPainPills,
    CParticleFire,
    CParticlePerformanceMonitor,
    CParticleSystem,
    CPhysBox,
    CPhysBoxMultiplayer,
    CPhysicsProp,
    CPhysMagnet,
    CPipeBomb,
    CPipeBombProjectile,
    CPistol,
    CPlasma,
    CPlayerResource,
    CPointCommentaryNode,
    CPointPropUseTarget,
    CPointScriptUseTarget,
    CPoseController,
    CPostProcessController,
    CPrecipitation,
    CPrecipitationBlocker,
    CPredictedViewModel,
    CPropaneTank,
    CPropDoorRotatingCheckpoint,
    CPropHealthCabinet,
    CPropMinigun,
    CPropMountedGun,
    CPropVehicleDriveable,
    CPumpShotgun,
    CRagdollManager,
    CRagdollProp,
    CRagdollPropAttached,
    CRifle_AK47,
    CRifle_Desert,
    CRifle_M60,
    CRifle_SG552,
    CRopeKeyframe,
    CScavengeProgressDisplay,
    CSceneEntity,
    CScriptBaseButton,
    CSelfDestruct,
    CShadowControl,
    CShotgun_Chrome,
    CShotgun_SPAS,
    CSlideshowDisplay,
    CSMG_MP5,
    CSMG_Silenced,
    CSmokerClaw,
    CSmokeStack,
    CSniper_AWP,
    CSniper_Military,
    CSniper_Scout,
    CSniperRifle,
    CSoundMixLayer,
    CSpitAbility,
    CSpitterClaw,
    CSpitterProjectile,
    CSpotlightEnd,
    CSprite,
    CSpriteOriented,
    CSpriteTrail,
    CSteamJet,
    CSubMachinegun,
    CSun,
    CSurvivorDeathModel,
    CSurvivorPosition,
    CSurvivorRescue,
    CTankClaw,
    CTeam,
    CTeamplayRoundBasedRulesProxy,
    CTEArmorRicochet,
    CTEBaseBeam,
    CTEBeamEntPoint,
    CTEBeamEnts,
    CTEBeamFollow,
    CTEBeamLaser,
    CTEBeamPoints,
    CTEBeamRing,
    CTEBeamRingPoint,
    CTEBeamSpline,
    CTEBloodSprite,
    CTEBloodStream,
    CTEBreakModel,
    CTEBSPDecal,
    CTEBubbles,
    CTEBubbleTrail,
    CTEBullets,
    CTEClientProjectile,
    CTEDecal,
    CTEDust,
    CTEDynamicLight,
    CTEEffectDispatch,
    CTEEnergySplash,
    CTEExplosion,
    CTEFizz,
    CTEFootprintDecal,
    CTEFoundryHelpers,
    CTEGaussExplosion,
    CTEGlowSprite,
    CTEImpact,
    CTEKillPlayerAttachments,
    CTELargeFunnel,
    CTEMetalSparks,
    CTEMuzzleFlash,
    CTEParticleSystem,
    CTEPhysicsProp,
    CTEPlayerAnimEvent,
    CTEPlayerDecal,
    CTEProjectedDecal,
    CTerrorGameRulesProxy,
    CTerrorGun,
    CTerrorMeleeWeapon,
    CTerrorPlayer,
    CTerrorPlayerResource,
    CTerrorViewModel,
    CTerrorWeapon,
    CTEShatterSurface,
    CTEShowLine,
    CTESmoke,
    CTESparks,
    CTESprite,
    CTESpriteSpray,
    CTest_ProxyToggle_Networkable,
    CTestTraceline,
    CTEWorldDecal,
    CThrow,
    CTongue,
    CTransitioningPlayer,
    CTriggerPlayerMovement,
    CTutorLesson,
    CVGuiScreen,
    CVomit,
    CVomitJarProjectile,
    CVoteController,
    CWaterBullet,
    CWaterLODControl,
    CWeaponAmmoSpawn,
    CWeaponCSBase,
    CWeaponCSBaseGun,
    CWeaponScavengeItemSpawn,
    CWeaponSpawn,
    CWorld,
    DustTrail,
    Hunter,
    Infected,
    Jockey,
    MovieExplosion,
    NextBotCombatCharacter,
    ParticleSmokeGrenade,
    RocketTrail,
    Smoker,
    SmokeTrail,
    Spitter,
    SporeExplosion,
    SporeTrail,
    SurvivorBot,
    Tank,
    Witch,
};

typedef enum
{
    DPT_Int = 0,
    DPT_Float,
    DPT_Vector,
    DPT_VectorXY,
    DPT_String,
    DPT_Array,
    DPT_DataTable,
    DPT_NUMSendPropTypes
} SendPropType;

class DVariant
{
public:
    union
    {
        float       m_Float;
        int         m_Int;
        const char *m_pString;
        void       *m_pData;
        float       m_Vector[ 3 ];
    };
    SendPropType m_Type;
};

class CRecvDecoder;
class RecvTable;
class RecvProp;
class IClientNetworkable;

class CRecvProxyData
{
public:
    const RecvProp *m_pRecvProp;
    DVariant        m_Value;
    int             m_iElement;
    int             m_ObjectID;
};

typedef IClientNetworkable *( *CreateClientClassFn )( int entnum, int serialNum );
typedef IClientNetworkable *( *CreateEventFn )( );
typedef void ( *RecvVarProxyFn )( const CRecvProxyData *pData, void *pStruct, void *pOut );
typedef void ( *ArrayLengthRecvProxyFn )( void *pStruct, int objectID, int currentArrayLength );
typedef void ( *DataTableRecvVarProxyFn )( const RecvProp *pProp, void **pOut, void *pData, int objectID );

class RecvProp
{
public:
    const char  *m_pVarName;
    SendPropType m_RecvType;
    int          m_Flags;
    int          m_StringBufferSize;

    bool m_bInsideArray;

    const void *m_pExtraData;

    RecvProp              *m_pArrayProp;
    ArrayLengthRecvProxyFn m_ArrayLengthProxy;

    RecvVarProxyFn          m_ProxyFn;
    DataTableRecvVarProxyFn m_DataTableProxyFn;

    RecvTable *m_pDataTable;
    int        m_Offset;

    int m_ElementStride;
    int m_nElements;

    const char *m_pParentArrayPropName;
};

class RecvTable
{
public:
    RecvProp     *m_pProps;
    int           m_nProps;
    CRecvDecoder *m_pDecoder;
    const char   *m_pNetTableName;
    bool          m_bInitialized;
    bool          m_bInMainList;
};

class ClientClass
{
public:
    CreateClientClassFn m_pCreateFn;
    CreateEventFn       m_pCreateEventFn;
    const char         *m_pNetworkName;
    RecvTable          *m_pRecvTable;
    ClientClass        *m_pNext;
    int                 m_ClassID;

    EL4D2ClassID GetClassID( );
};

class CHLClient
{
public:
    ClientClass *GetAllClasses( );
    void         CreateMove( i32 sequence_number, float input_sample_frametime, bool active );
    bool         WriteUsercmdDeltaToBuffer( i32 slot, bf_write *buffer, i32 from, i32 to, bool is_new_command );
};

inline constexpr usize MULTIPLAYER_BACKUP{ 150 };
inline constexpr usize MAX_VIEWMODELS{ 2 };
inline constexpr usize MAX_SPLITSCREEN_PLAYERS{ 2 };

enum fieldtype_t : int
{
    FIELD_VOID = 0,
    FIELD_FLOAT,
    FIELD_STRING,
    FIELD_VECTOR,
    FIELD_QUATERNION,
    FIELD_INTEGER,
    FIELD_BOOLEAN,
    FIELD_SHORT,
    FIELD_CHARACTER,
    FIELD_COLOR32,
    FIELD_EMBEDDED,
    FIELD_CUSTOM,

    FIELD_CLASSPTR,
    FIELD_EHANDLE,
    FIELD_EDICT,

    FIELD_POSITION_VECTOR,
    FIELD_TIME,
    FIELD_TICK,
    FIELD_MODELNAME,
    FIELD_SOUNDNAME,

    FIELD_INPUT,
    FIELD_FUNCTION,

    FIELD_VMATRIX,
    FIELD_VMATRIX_WORLDSPACE,
    FIELD_MATRIX3X4_WORLDSPACE,

    FIELD_INTERVAL,
};

enum
{
    TD_OFFSET_NORMAL = 0,
    TD_OFFSET_PACKED = 1,
    TD_OFFSET_COUNT,
};

struct datamap_t;
struct optimized_datamap_t;
class ISaveRestoreOps;

struct typedescription_t
{
    fieldtype_t        fieldType{ };
    const char        *fieldName{ };
    int                fieldOffset{ };
    unsigned short     fieldSize{ };
    short              flags{ };
    const char        *externalName{ };
    ISaveRestoreOps   *pSaveRestoreOps{ };
    void              *inputFunc{ };
    datamap_t         *td{ };
    int                fieldSizeInBytes{ };
    typedescription_t *override_field{ };
    int                override_count{ };
    float              fieldTolerance{ };
    int                flatOffset[ TD_OFFSET_COUNT ]{ };
    unsigned short     flatGroup{ };
};

struct datamap_t
{
    typedescription_t   *dataDesc{ };
    int                  dataNumFields{ };
    const char          *dataClassName{ };
    datamap_t           *baseMap{ };
    int                  m_nPackedSize{ };
    optimized_datamap_t *m_pOptimizedDataMap{ };
};

struct flattened_chain_t
{
    typedescription_t *m_fields{ };
    int                m_allocated{ };
    int                m_grow{ };
    int                m_count{ };
    typedescription_t *m_elements{ };
    int                m_packed_size{ };
    int                m_packed_start{ };
    u8                 m_runs[ 0x14 ]{ };
};

struct optimized_datamap_t
{
    flattened_chain_t m_chains[ 2 ];
};

static_assert( sizeof( flattened_chain_t ) == 0x30 );
static_assert( sizeof( typedescription_t ) == 0x3C );
static_assert( sizeof( datamap_t ) == 0x18 );

inline constexpr short FTYPEDESC_INSENDTABLE{ 0x0100 };
inline constexpr short FTYPEDESC_NOERRORCHECK{ 0x0400 };

inline constexpr int PC_EVERYTHING{ 2 };
inline constexpr bool PC_DATA_PACKED{ true };
inline constexpr bool PC_DATA_NORMAL{ false };

class CPredictionCopy
{
public:
    CPredictionCopy( int type, void *dest, bool dest_packed, const void *src, bool src_packed ) :
        m_nType{ type },
        m_pDest{ dest },
        m_pSrc{ src },
        m_nDestOffsetIndex{ dest_packed ? TD_OFFSET_PACKED : TD_OFFSET_NORMAL },
        m_nSrcOffsetIndex{ src_packed ? TD_OFFSET_PACKED : TD_OFFSET_NORMAL }
    {
    }

    int TransferData( const char *operation, int entindex, datamap_t *dmap );

private:
    int                m_OpType{ };
    int                m_nType{ };
    void              *m_pDest{ };
    const void        *m_pSrc{ };
    int                m_nDestOffsetIndex{ };
    int                m_nSrcOffsetIndex{ };
    int                m_nErrorCount{ };
    int                m_nEntIndex{ -1 };
    void              *m_FieldCompareFunc{ };
    typedescription_t *m_pWatchField{ };
    const char        *m_pOperation{ };
    u8                 m_FieldStack[ 0x14 ]{ };
};

class CBaseHandle;
class ICollideable;
class IClientEntity;
class IClientRenderable;
class IClientThinkable;
class C_BaseEntity;

class IHandleEntity
{
public:
    virtual ~IHandleEntity( )
    {
    }
    virtual void               SetRefEHandle( const CBaseHandle &handle ) = 0;
    virtual const CBaseHandle &GetRefEHandle( ) const                     = 0;
};

class IClientUnknown : public IHandleEntity
{
public:
    virtual ICollideable       *GetCollideable( )       = 0;
    virtual IClientNetworkable *GetClientNetworkable( ) = 0;
    virtual IClientRenderable  *GetClientRenderable( )  = 0;
    virtual IClientEntity      *GetIClientEntity( )     = 0;
    virtual C_BaseEntity       *GetBaseEntity( )        = 0;
    virtual IClientThinkable   *GetClientThinkable( )   = 0;

    template< typename T >
    inline T As( )
    {
        return reinterpret_cast< T >( this );
    }
};

class IClientEntity : public IClientUnknown
{
};

inline constexpr int MAX_EDICT_BITS{ 11 };
inline constexpr int NUM_ENT_ENTRY_BITS{ MAX_EDICT_BITS + 1 };
inline constexpr int NUM_ENT_ENTRIES{ 1 << NUM_ENT_ENTRY_BITS };
inline constexpr int NUM_SERIAL_NUM_BITS{ 32 - NUM_ENT_ENTRY_BITS };
inline constexpr int NUM_SERIAL_NUM_SHIFT_BITS{ 32 - NUM_SERIAL_NUM_BITS };
inline constexpr int ENT_ENTRY_MASK{ ( 1 << NUM_SERIAL_NUM_SHIFT_BITS ) - 1 };
inline constexpr u32 INVALID_EHANDLE_INDEX{ 0xFFFFFFFFu };

class CBaseHandle
{
public:
    constexpr ATLAS_FORCE_INLINE CBaseHandle( )
    {
        m_Index = INVALID_EHANDLE_INDEX;
    }
    CBaseHandle( const CBaseHandle &other );
    explicit CBaseHandle( unsigned long value );
    CBaseHandle( int iEntry, int iSerialNumber );

    void Init( int iEntry, int iSerialNumber );
    void Term( );

    bool IsValid( ) const;

    int           GetEntryIndex( ) const;
    int           GetSerialNumber( ) const;
    int           ToInt( ) const;
    unsigned long GetIndex( ) const;

    bool operator==( const CBaseHandle &other ) const;
    bool operator!=( const CBaseHandle &other ) const;
    bool operator<( const CBaseHandle &other ) const;

    bool operator==( const IHandleEntity *pEnt ) const;
    bool operator!=( const IHandleEntity *pEnt ) const;
    bool operator<( const IHandleEntity *pEnt ) const;

    const CBaseHandle &operator=( const IHandleEntity *pEntity );
    const CBaseHandle &Set( const IHandleEntity *pEntity );

    IHandleEntity *Get( ) const;

protected:
    unsigned long m_Index;
};

template< class T >
class CHandle : public CBaseHandle
{
public:
    constexpr CHandle( ) = default;
    CHandle( int iEntry, int iSerialNumber );
    CHandle( const CBaseHandle &handle );
    CHandle( T *pVal );

    static CHandle< T > FromIndex( int index );

    T   *Get( ) const;
    void Set( const T *pVal );

    operator T *( );
    operator T *( ) const;

    bool operator!( ) const;
    bool operator==( T *val ) const;
    bool operator!=( T *val ) const;

    const CBaseHandle &operator=( const T *val );

    T *operator->( ) const;
};

class CClientEntityList
{
public:
    IClientEntity *GetClientEntity( int num );
    IClientEntity *GetClientEntityFromHandle( CBaseHandle num );
    int            GetHighestEntityIndex( );
};

template< class T >
inline CHandle< T >::CHandle( int iEntry, int iSerialNumber )
{
    this->Init( iEntry, iSerialNumber );
}

template< class T >
inline CHandle< T >::CHandle( const CBaseHandle &handle ) : CBaseHandle( handle )
{
}

template< class T >
inline CHandle< T >::CHandle( T *pVal )
{
    this->Set( pVal );
}

template< class T >
inline CHandle< T > CHandle< T >::FromIndex( int index )
{
    CHandle< T > h{ };
    h.m_Index = static_cast< unsigned long >( index );
    return h;
}

template< class T >
inline T *CHandle< T >::Get( ) const
{
    return reinterpret_cast< T * >( CBaseHandle::Get( ) );
}

template< class T >
inline void CHandle< T >::Set( const T *pVal )
{
    CBaseHandle::Set( reinterpret_cast< const IHandleEntity * >( pVal ) );
}

template< class T >
inline CHandle< T >::operator T *( )
{
    return this->Get( );
}

template< class T >
inline CHandle< T >::operator T *( ) const
{
    return this->Get( );
}

template< class T >
inline bool CHandle< T >::operator!( ) const
{
    return !this->IsValid( );
}

template< class T >
inline bool CHandle< T >::operator==( T *val ) const
{
    return this->Get( ) == val;
}

template< class T >
inline bool CHandle< T >::operator!=( T *val ) const
{
    return this->Get( ) != val;
}

template< class T >
inline const CBaseHandle &CHandle< T >::operator=( const T *val )
{
    return CBaseHandle::operator=( reinterpret_cast< const IHandleEntity * >( val ) );
}

template< class T >
inline T *CHandle< T >::operator->( ) const
{
    return this->Get( );
}

using EHANDLE = CHandle< C_BaseEntity >;

enum ClientFrameStage_t
{
    FRAME_UNDEFINED = -1,
    FRAME_START,
    FRAME_NET_UPDATE_START,
    FRAME_NET_UPDATE_POSTDATAUPDATE_START,
    FRAME_NET_UPDATE_POSTDATAUPDATE_END,
    FRAME_NET_UPDATE_END,
    FRAME_RENDER_START,
    FRAME_RENDER_END,
};

enum PaintMode_t
{
    PAINT_UIPANELS     = ( 1 << 0 ),
    PAINT_INGAMEPANELS = ( 1 << 1 ),
};

class CEngineVGui
{
public:
};

namespace vgui
{
    enum CursorCode
    {
        dc_user,
        dc_none,
        dc_arrow,
        dc_ibeam,
        dc_hourglass,
        dc_waitarrow,
        dc_crosshair,
        dc_up,
        dc_sizenwse,
        dc_sizenesw,
        dc_sizewe,
        dc_sizens,
        dc_sizeall,
        dc_no,
        dc_hand,
        dc_blank,
        dc_last,
        dc_alwaysvisible_push,
        dc_alwaysvisible_pop,
    };

    typedef u32 HCursor;
} // namespace vgui

class CMatSystemSurface
{
public:
    void UnlockCursor( );
};

enum SolidType_t
{
    SOLID_NONE     = 0,
    SOLID_BSP      = 1,
    SOLID_BBOX     = 2,
    SOLID_OBB      = 3,
    SOLID_OBB_YAW  = 4,
    SOLID_CUSTOM   = 5,
    SOLID_VPHYSICS = 6,
    SOLID_LAST,
};

enum SolidFlags_t
{
    FSOLID_NOT_SOLID = 0x0004,
};

enum
{
    LIFE_ALIVE = 0,
    LIFE_DYING = 1,
    LIFE_DEAD  = 2
};

inline constexpr i32 ACT_TERROR_DIE_FROM_STAND{ 667 };
inline constexpr i32 ACT_SLICING_DEATH_RIGHT{ 681 };

enum ZombieClassType : int
{
    ZOMBIE_CLASS_COMMON = 0,
    ZOMBIE_CLASS_SMOKER,
    ZOMBIE_CLASS_BOOMER,
    ZOMBIE_CLASS_HUNTER,
    ZOMBIE_CLASS_SPITTER,
    ZOMBIE_CLASS_JOCKEY,
    ZOMBIE_CLASS_CHARGER,
    ZOMBIE_CLASS_WITCH,
    ZOMBIE_CLASS_TANK,
};

class CEngineClient
{
public:
    void           GetScreenSize( i32 &width, i32 &height );
    i32            GetLocalPlayer( );
    bool           IsInGame( );
    const VMatrix &WorldToScreenMatrix( );
    i32            GetActiveSplitScreenPlayerSlot( );
};

typedef unsigned int CRC32_t;

enum MoveType_t : u8
{
    MOVETYPE_NONE = 0,
    MOVETYPE_ISOMETRIC,
    MOVETYPE_WALK,
    MOVETYPE_STEP,
    MOVETYPE_FLY,
    MOVETYPE_FLYGRAVITY,
    MOVETYPE_VPHYSICS,
    MOVETYPE_PUSH,
    MOVETYPE_NOCLIP,
    MOVETYPE_LADDER,
    MOVETYPE_OBSERVER,
    MOVETYPE_CUSTOM,
};

constexpr auto FL_ONGROUND = ( 1 << 0 );
constexpr auto FL_DUCKING  = ( 1 << 1 );

constexpr auto IN_ATTACK = ( 1 << 0 );
constexpr auto IN_JUMP   = ( 1 << 1 );

class CUniformRandomStream
{
public:
    void  SetSeed( int seed );
    float RandomFloat( float minimum, float maximum );

private:
    void *m_vtable{ };
    int   m_idum{ };
    int   m_iy{ };
    int   m_iv[ 32 ]{ };
    u32   m_mutex_owner{ };
    int   m_mutex_depth{ };

    u8 m_reserved[ 16 ]{ };
};

static_assert( sizeof( CUniformRandomStream ) == 148 + 16 );

class CUserCmd
{
public:
    CRC32_t GetChecksum( ) const;

private:
    void *m_vmt;

public:
    int   command_number;
    int   tick_count;
    vec3  viewangles;
    float forwardmove;
    float sidemove;
    float upmove;
    int   buttons;
    u8    impulse;
    int   weaponselect;
    int   weaponsubtype;
    int   random_seed;
    short mousedx;
    short mousedy;
    bool  hasbeenpredicted;

private:
    // six more dwords that CreateMove zeroes and @::GetChecksum skips..
    u8 m_unknown[ 24 ];
};

class CVerifiedUserCmd
{
public:
    CUserCmd m_cmd;
    CRC32_t  m_crc;
};

class CInput
{
public:
    CUserCmd         *GetUserCmd( i32 slot, i32 sequence_number );
    CVerifiedUserCmd *GetVerifiedUserCmd( i32 slot, i32 sequence_number );
};

class CGlobalVarsBase
{
public:
    float realtime{ };
    int   framecount{ };
    float absoluteframetime{ };
    float curtime{ };
    float frametime{ };
    int   maxClients{ };
    int   tickcount{ };
    float interval_per_tick{ };
};

class ConVar
{
public:
    float GetFloat( ) const;
    int   GetInt( ) const;

private:
    void       *m_base_vmt;
    void       *m_pNext;
    bool        m_bRegistered;
    const char *m_pszName;
    const char *m_pszHelpString;
    int         m_nFlags;
    void       *m_convar_vmt;

public:
    ConVar     *m_pParent;
    const char *m_pszDefaultValue;
    char       *m_pszString;
    int         m_StringLength;
    float       m_fValue;
    int         m_nValue;
};

class ICvar
{
public:
    ConVar *FindVar( const char *var_name );
};

class CViewSetup
{
private:
    u8 m_unknown0[ 0x10 ];

public:
    int width;

private:
    u8 m_unknown1[ 0x4 ];

public:
    int height;

private:
    u8 m_unknown2[ 0x18 ];

public:
    float fov;
    float fovViewmodel;
    vec3  origin;
    vec3  angles;
    float zNear;
    float zFar;

private:
    u8 m_unknown3[ 0x74 ];
};

class ClientModeShared
{
public:
};

class C_BasePlayer;

class IMemAlloc
{
public:
    void *Alloc( usize size );
    void *Realloc( void *mem, usize size );
    void  Free( void *mem );
};

class CMoveHelperClient
{
public:
    void SetHost( C_BasePlayer *host );
};

class IPredictionSystem
{
private:
    void *m_vmt;

public:
    IPredictionSystem *m_pNextSystem;
    bool               m_bSuppressEvent;
    C_BaseEntity      *m_pSuppressHost;
    int                m_nStatusPushed;
};

class CPrediction
{
public:
    void GetLocalViewAngles( vec3 &angles );
    void SetLocalViewAngles( vec3 &angles );
    void RunCommand( C_BasePlayer *player, CUserCmd *cmd, CMoveHelperClient *move_helper );

    struct split_t
    {
        bool  m_bFirstTimePredicted;
        int   m_nCommandsPredicted;
        int   m_nServerCommandsAcknowledged;
        int   m_bPreviousAckHadErrors;
        float m_flIdealPitch;
    };

private:
    u8 m_unknown0[ 0x8 ];

public:
    bool m_bInPrediction;
    bool m_bOldCLPredictValue;
    bool m_bEnginePaused;

private:
    u8 m_unknown1[ 0xD ];

public:
    split_t m_Split[ MAX_SPLITSCREEN_PLAYERS ];
};

inline constexpr i32 SIGNONSTATE_CONNECTED{ 2 };
inline constexpr i32 SIGNONSTATE_FULL{ 6 };

inline constexpr i32 MAX_CMD_BUFFER{ 4000 };
inline constexpr i32 MAX_NEW_COMMANDS{ 15 };
inline constexpr i32 NUM_BACKUP_COMMANDS{ 2 };

class CNetMessage
{
public:
    void *m_vmt{ };
    bool  m_bReliable{ };
    void *m_NetChannel{ };

private:
    u8 m_unknown0[ 0x48 ]{ };
};

class NET_Tick : public CNetMessage
{
public:
    int   m_nTick{ };
    float m_flHostFrameTime{ };
    float m_flHostFrameTimeStdDeviation{ };
};

class CLC_Move : public CNetMessage
{
public:
    int m_nBackupCommands{ };
    int m_nNewCommands{ };
    int m_nLength{ };

private:
    u8 m_DataIn[ 0x24 ]{ };

public:
    bf_write m_DataOut{ };
};

inline constexpr i32 FLOW_OUTGOING{ 0 };

class CNetChannel
{
public:
    bool  IsLoopback( );
    bool  IsTimingOut( );
    float GetLatency( i32 flow );
    bool  SendNetMsg( CNetMessage &msg, bool force_reliable, bool voice );
    void  SetChoked( );
    int   SendDatagram( bf_write *data );
    bool  CanPacket( );
};

class CClientState
{
public:
    bool IsPaused( );
    void ForceFullUpdate( const char *reason );

private:
    u8 m_unknown0[ 0x10 ];

public:
    CNetChannel *m_NetChannel;

private:
    u8 m_unknown1[ 0x54 ];

public:
    int m_nSignonState;

private:
    u8 m_unknown2[ 0x4 ];

public:
    double m_flNextCmdTime;

private:
    u8 m_unknown3[ 0x54 ];

public:
    int m_nDeltaTick;

private:
    u8 m_unknown4[ 0x4974 ];

public:
    int lastoutgoingcommand;
    int chokedcommands;

private:
    u8 m_unknown5[ 0x10 ];

public:
    bool ishltv;
};

class CSplitScreen
{
public:
    int  SetActiveSplitScreenPlayerSlot( int slot );
    int  FirstValidSplitScreenSlot( );
    int  NextValidSplitScreenSlot( int slot );
    bool IsDisconnecting( int slot );
    bool SetLocalPlayerIsResolvable( const char *file, int line, bool resolvable );

    CClientState *GetClientState( int slot )
    {
        return reinterpret_cast< CClientState * >( m_SplitScreenPlayers[ slot ] + 0x8 );
    }

private:
    void *m_vmt;
    u8   *m_SplitScreenPlayers[ MAX_SPLITSCREEN_PLAYERS ];
};

class IDemoPlayer
{
public:
    bool IsPlayingBack( );
};

class IDemoRecorder
{
public:
    bool IsRecording( );
    void RecordUserInput( int command_number );
};

inline constexpr i32 EF_NODRAW{ 0x20 };

inline constexpr i32 STUDIO_RENDER{ 0x1 };
inline constexpr i32 STUDIO_DONOTMODIFYSTENCILSTATE{ 0x800 };
inline constexpr i32 STUDIO_SHADOWDEPTHTEXTURE{ 0x40000000 };

struct RenderableInstance_t
{
    u8 m_nAlpha{ 255 };
};

struct model_t;

class IClientRenderable
{
public:
    const model_t *GetModel( );
    i32            DrawModel( i32 flags, const RenderableInstance_t &instance );
    bool           SetupBones( matrix3x4 *bone_to_world, i32 max_bones, i32 bone_mask, float current_time );
};

struct DrawModelState_t;

struct ModelRenderInfo_t
{
    vec3               origin;
    vec3               angles;
    IClientRenderable *pRenderable;
    const void        *pModel;
    const matrix3x4   *pModelToWorld;
    const matrix3x4   *pLightingOffset;
    const vec3        *pLightingOrigin;
    i32                flags;
    i32                entity_index;
    i32                skin;
    i32                body;
    i32                hitboxset;
    u16                instance;
};
static_assert( __builtin_offsetof( ModelRenderInfo_t, entity_index ) == 0x30 );

class CModelRender
{
public:
};

enum OverrideType_t
{
    OVERRIDE_NORMAL = 0,
};

enum StencilOperation_t
{
    STENCILOPERATION_KEEP    = 1,
    STENCILOPERATION_REPLACE = 3,
};

enum StencilComparisonFunction_t
{
    STENCILCOMPARISONFUNCTION_EQUAL  = 3,
    STENCILCOMPARISONFUNCTION_ALWAYS = 8,
};

struct ShaderStencilState_t
{
    bool                        m_bEnable{ };
    StencilOperation_t          m_FailOp{ STENCILOPERATION_KEEP };
    StencilOperation_t          m_ZFailOp{ STENCILOPERATION_KEEP };
    StencilOperation_t          m_PassOp{ STENCILOPERATION_KEEP };
    StencilComparisonFunction_t m_CompareFunc{ STENCILCOMPARISONFUNCTION_ALWAYS };
    i32                         m_nReferenceValue{ };
    u32                         m_nTestMask{ 0xFFFFFFFF };
    u32                         m_nWriteMask{ 0xFFFFFFFF };
};
static_assert( sizeof( ShaderStencilState_t ) == 0x20 );

class IMaterial
{
public:
    void IncrementReferenceCount( );
};

class ITexture
{
public:
};

class IKeyValuesSystem
{
public:
    void *AllocKeyValuesMemory( i32 size );
};

class KeyValues
{
public:
    KeyValues( const char *name );

    void  SetString( const char *key, const char *value );
    void  SetInt( const char *key, i32 value );
    void *operator new( usize size );

private:
    u8 m_pad[ 0x2C ];
};

class IMatRenderContext
{
public:
    void Release( );
    void BeginRender( );
    void EndRender( );
    void SetRenderTarget( ITexture *texture );
    void DepthRange( float z_near, float z_far );
    void ClearBuffers( bool clear_color, bool clear_depth, bool clear_stencil );
    void Viewport( i32 x, i32 y, i32 width, i32 height );
    void ClearColor4ub( u8 r, u8 g, u8 b, u8 a );
    void DrawScreenSpaceRectangle( IMaterial *material, i32 dest_x, i32 dest_y, i32 width, i32 height,
                                   float src_x0, float src_y0, float src_x1, float src_y1,
                                   i32 src_width, i32 src_height );
    void PushRenderTargetAndViewport( );
    void PopRenderTargetAndViewport( );
    void SetStencilState( const ShaderStencilState_t &state );
};

class CMaterialSystem
{
public:
    IMaterial         *CreateMaterial( const char *name, KeyValues *key_values );
    IMaterial         *FindMaterial( const char *name, const char *texture_group );
    ITexture          *FindTexture( const char *name, const char *texture_group );
    IMatRenderContext *GetRenderContext( );
};

class CStudioRender
{
public:
};

class CVRenderView
{
public:
    void  SetBlend( float blend );
    float GetBlend( );
    void  SetColorModulation( const float *color );
    void  GetColorModulation( float *color );
};

inline constexpr u32 MASK_SHOT{ 0x46004003u };

inline constexpr i32 BONE_USED_BY_HITBOX{ 0x100 };
inline constexpr i32 MAXSTUDIOBONES{ 128 };

enum
{
    HITGROUP_GENERIC = 0,
    HITGROUP_HEAD,
    HITGROUP_CHEST,
    HITGROUP_STOMACH,
    HITGROUP_LEFTARM,
    HITGROUP_RIGHTARM,
    HITGROUP_LEFTLEG,
    HITGROUP_RIGHTLEG,
};

struct mstudiobbox_t
{
    i32  bone;
    i32  group;
    vec3 bbmin;
    vec3 bbmax;
    i32  szhitboxnameindex;
    i32  unused[ 8 ];
};
static_assert( sizeof( mstudiobbox_t ) == 0x44 );

struct mstudiohitboxset_t
{
    i32 sznameindex;
    i32 numhitboxes;
    i32 hitboxindex;

    mstudiobbox_t *pHitbox( i32 index )
    {
        return reinterpret_cast< mstudiobbox_t * >( reinterpret_cast< u8 * >( this ) + hitboxindex ) + index;
    }
};

struct studiohdr_t
{
    u8  m_unknown0[ 0x9C ];
    i32 numbones;
    i32 boneindex;
    i32 numbonecontrollers;
    i32 bonecontrollerindex;
    i32 numhitboxsets;
    i32 hitboxsetindex;

    mstudiohitboxset_t *pHitboxSet( i32 index )
    {
        return reinterpret_cast< mstudiohitboxset_t * >( reinterpret_cast< u8 * >( this ) + hitboxsetindex ) + index;
    }
};
static_assert( __builtin_offsetof( studiohdr_t, hitboxsetindex ) == 0xB0 );

class CModelInfo
{
public:
    studiohdr_t *GetStudiomodel( const model_t *model );
};

struct alignas( 16 ) VectorAligned
{
    f32 x{ };
    f32 y{ };
    f32 z{ };
    f32 w{ };
};

struct Ray_t
{
    Ray_t( const vec3 &start, const vec3 &end ) :
        m_Start{ start.x, start.y, start.z },
        m_Delta{ end.x - start.x, end.y - start.y, end.z - start.z },
        m_IsRay{ true },
        m_IsSwept{ true }
    {
    }

    VectorAligned    m_Start;
    VectorAligned    m_Delta;
    VectorAligned    m_StartOffset;
    VectorAligned    m_Extents;
    const matrix3x4 *m_pWorldAxisTransform{ };
    bool             m_IsRay;
    bool             m_IsSwept;
};
static_assert( __builtin_offsetof( Ray_t, m_IsRay ) == 0x44 );

struct csurface_t
{
    const char *name;
    i16         surfaceProps;
    u16         flags;
};

struct CGameTrace
{
    vec3          startpos;
    vec3          endpos;
    vec3          plane_normal;
    f32           plane_dist;
    u8            plane_type;
    u8            plane_signbits;
    u8            plane_pad[ 2 ];
    f32           fraction;
    i32           contents;
    u16           dispFlags;
    bool          allsolid;
    bool          startsolid;
    f32           fractionleftsolid;
    csurface_t    surface;
    i32           hitgroup;
    i16           physicsbone;
    C_BaseEntity *m_pEnt;
    i32           hitbox;
};
static_assert( __builtin_offsetof( CGameTrace, fraction ) == 0x2C );
static_assert( __builtin_offsetof( CGameTrace, allsolid ) == 0x36 );
static_assert( __builtin_offsetof( CGameTrace, m_pEnt ) == 0x4C );

enum TraceType_t
{
    TRACE_EVERYTHING = 0,
};

class ITraceFilter
{
public:
    virtual bool        ShouldHitEntity( IHandleEntity *entity, i32 contents_mask ) = 0;
    virtual TraceType_t GetTraceType( ) const                                       = 0;
};

class CTraceFilterSkipEntity : public ITraceFilter
{
public:
    explicit CTraceFilterSkipEntity( const IHandleEntity *skip, const IHandleEntity *also_skip = nullptr ) :
        m_skip{ skip },
        m_also_skip{ also_skip }
    {
    }

    bool ShouldHitEntity( IHandleEntity *entity, i32 contents_mask ) override
    {
        ATLAS_UNUSED_PARAM( contents_mask );
        return entity != m_skip && entity != m_also_skip;
    }

    TraceType_t GetTraceType( ) const override
    {
        return TRACE_EVERYTHING;
    }

private:
    const IHandleEntity *m_skip;
    const IHandleEntity *m_also_skip;
};

class CEngineTrace
{
public:
    void TraceRay( const Ray_t &ray, u32 mask, ITraceFilter *filter, CGameTrace *trace );
};
