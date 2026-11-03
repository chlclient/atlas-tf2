#pragma once

#include "base.hpp"

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

class CRecvDecoder;
class CRecvProxyData;
class RecvTable;
class RecvProp;
class IClientNetworkable;

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

inline constexpr i32 SPROP_COORD{ 1 << 1 };
inline constexpr i32 SPROP_NOSCALE{ 1 << 2 };
inline constexpr i32 SPROP_NORMAL{ 1 << 5 };
inline constexpr i32 SPROP_EXCLUDE{ 1 << 6 };
inline constexpr i32 SPROP_COORD_MP{ 1 << 12 };
inline constexpr i32 SPROP_COORD_MP_LOWPRECISION{ 1 << 13 };
inline constexpr i32 SPROP_COORD_MP_INTEGRAL{ 1 << 14 };
inline constexpr i32 SPROP_CELL_COORD{ 1 << 15 };
inline constexpr i32 SPROP_CELL_COORD_LOWPRECISION{ 1 << 16 };
inline constexpr i32 SPROP_CELL_COORD_INTEGRAL{ 1 << 17 };

inline constexpr i32 COORD_DENOMINATOR{ 1 << 5 };

class SendTable;

class SendProp
{
public:
    void     *m_pVTable;
    RecvProp *m_pMatchingRecvProp;

    SendPropType m_Type;
    int          m_nBits;
    float        m_fLowValue;
    float        m_fHighValue;

    SendProp *m_pArrayProp;
    void     *m_ArrayLengthProxy;

    int m_nElements;
    int m_ElementStride;

    const char *m_pExcludeDTName;
    const char *m_pParentArrayPropName;

    const char *m_pVarName;
    float       m_fHighLowMul;
    u8          m_priority;

    int         m_Flags;
    void       *m_ProxyFn;
    void       *m_DataTableProxyFn;
    SendTable  *m_pDataTable;
    int         m_Offset;
    const void *m_pExtraData;
};

static_assert( __builtin_offsetof( SendProp, m_Type ) == 0x8 );
static_assert( __builtin_offsetof( SendProp, m_fLowValue ) == 0x10 );
static_assert( __builtin_offsetof( SendProp, m_pExcludeDTName ) == 0x28 );
static_assert( __builtin_offsetof( SendProp, m_pVarName ) == 0x30 );
static_assert( __builtin_offsetof( SendProp, m_Flags ) == 0x3C );
static_assert( __builtin_offsetof( SendProp, m_pDataTable ) == 0x48 );
static_assert( sizeof( SendProp ) == 0x54 );

class SendTable
{
public:
    SendProp   *m_pProps;
    int         m_nProps;
    const char *m_pNetTableName;
};

class CRecvDecoder
{
public:
    RecvTable *m_pTable;
    SendTable *m_pClientSendTable;
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
};

class bf_write;

class CHLClient
{
public:
    ClientClass *GetAllClasses( );
    void         CreateMove( i32 sequence_number, float input_sample_frametime, bool active );
    bool         WriteUsercmdDeltaToBuffer( i32 slot, bf_write *buffer, i32 from, i32 to, bool is_new_command );
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

class CBaseHandle
{
public:
    IClientEntity *Get( ) const;

    unsigned long ToInt( ) const
    {
        return this->m_Index;
    }

protected:
    unsigned long m_Index{ 0xFFFFFFFFu };
};

template< class T >
class CHandle : public CBaseHandle
{
public:
    T *Get( ) const
    {
        IClientEntity *entity{ CBaseHandle::Get( ) };
        return entity ? entity->As< T * >( ) : nullptr;
    }
};

using EHANDLE = CHandle< C_BaseEntity >;

class CClientEntityList
{
public:
    IClientEntity *GetClientEntity( int num );
    IClientEntity *GetClientEntityFromHandle( CBaseHandle num );
};

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
}

class CMatSystemSurface
{
public:
    void UnlockCursor( );
    void PlaySound( const char *file_name );
};

class CGlobalVarsBase
{
public:
    float realtime{ };
    int   framecount{ };
    float absoluteframetime{ };
    float absoluteframestarttimestddev{ };
    float curtime{ };
    float frametime{ };
    int   maxClients{ };
    int   tickcount{ };
    float interval_per_tick{ };
};

class ClientModeShared
{
public:
};

typedef unsigned int CRC32_t;

inline constexpr i32 MULTIPLAYER_BACKUP{ 150 };

inline constexpr i32 IN_ATTACK{ 1 << 0 };
inline constexpr i32 IN_JUMP{ 1 << 1 };
inline constexpr i32 IN_DUCK{ 1 << 2 };
inline constexpr i32 IN_FORWARD{ 1 << 3 };
inline constexpr i32 IN_BACK{ 1 << 4 };
inline constexpr i32 IN_USE{ 1 << 5 };
inline constexpr i32 IN_MOVELEFT{ 1 << 9 };
inline constexpr i32 IN_MOVERIGHT{ 1 << 10 };
inline constexpr i32 IN_ATTACK2{ 1 << 11 };
inline constexpr i32 IN_SPEED{ 1 << 17 };

inline constexpr i32 FL_ONGROUND{ 1 << 0 };
inline constexpr i32 FL_DUCKING{ 1 << 1 };

inline constexpr i32 MAX_PLAYERS{ 64 };
inline constexpr i32 MAX_TRACKED_PLAYERS{ MAX_PLAYERS + 1 };

inline constexpr float MAX_MOVE{ 450.0f };

inline constexpr float AIR_WISH_SPEED_CAP{ 30.0f };

inline constexpr float LAG_COMPENSATION_TELEPORTED_DISTANCE_SQR{ 64.0f * 64.0f };

enum MoveType_t : u8
{
    MOVETYPE_NONE,
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
    vec3  aimdirection;
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
    vec3  headangles;
    vec3  headoffset;
};

static_assert( sizeof( CUserCmd ) == 0x64 );

class CVerifiedUserCmd
{
public:
    CUserCmd m_cmd;
    CRC32_t  m_crc;
};

class CViewSetup
{
private:
    u8 m_pad[ 0xB0 ];

public:
    float fov;
    float fovViewmodel;
    vec3  origin;
    vec3  angles;
};

class CInput
{
public:
    bool              CAM_IsThirdPerson( );
    void              CAM_ToThirdPerson( );
    void              CAM_ToFirstPerson( );
    CUserCmd         *GetUserCmd( i32 sequence_number );
    CVerifiedUserCmd *GetVerifiedUserCmd( i32 sequence_number );
};

class ConVar
{
public:
    float GetFloat( ) const;
    i32   GetInt( ) const;
    void  SetValue( i32 value );
};

class ICvar
{
public:
    ConVar *FindVar( const char *name );
};

struct player_info_t
{
    i64     version;
    i64     xuid;
    char    name[ 128 ];
    int     userID;
    char    guid[ 33 ];
    u32     friendsID;
    char    friendsName[ 128 ];
    bool    fakeplayer;
    bool    ishltv;
    CRC32_t customFiles[ 4 ];
    u8      filesDownloaded;
};

static_assert( sizeof( player_info_t ) == 0x158 );

class CSfxTable;
class KeyValues;

struct StartSoundParams_t
{
    int         userdata;
    int         soundsource;
    int         entchannel;
    CSfxTable  *pSfx;
    vec3        origin;
    vec3        direction;
    float       fvol;
    int         soundlevel;
    int         flags;
    int         pitch;
    float       delay;
    int         speakerentity;
    int         initialStreamPosition;
    int         skipInitialSamples;
    int         m_nQueuedGUID;
    u32         m_nSoundScriptHash;
    const char *m_pSoundEntryName;
    KeyValues  *m_pOperatorsKV;
    float       opStackElapsedTime;
    float       opStackElapsedStopTime;
    bool        staticsound : 1;
    bool        bUpdatePositions : 1;
    bool        fromserver : 1;
    bool        bToolSound : 1;
    bool        m_bIsScriptHandle : 1;
    bool        m_bDelayedStart : 1;
    bool        m_bInEyeSound : 1;
    bool        m_bHRTFFollowEntity : 1;
    bool        m_bHRTFBilinear : 1;
    bool        m_bHRTFLock : 1;
};

static_assert( sizeof( StartSoundParams_t ) == 0x64 );

inline constexpr u32 CONTENTS_SOLID{ 0x1 };
inline constexpr u32 CONTENTS_WINDOW{ 0x2 };
inline constexpr u32 CONTENTS_GRATE{ 0x8 };
inline constexpr u32 CONTENTS_MOVEABLE{ 0x4000 };
inline constexpr u32 CONTENTS_MONSTER{ 0x2000000 };
inline constexpr u32 CONTENTS_DEBRIS{ 0x4000000 };
inline constexpr u32 CONTENTS_HITBOX{ 0x40000000 };
inline constexpr u32 MASK_SOLID{ CONTENTS_SOLID | CONTENTS_MOVEABLE | CONTENTS_WINDOW | CONTENTS_MONSTER | CONTENTS_GRATE };
inline constexpr u32 CS_MASK_SHOOT{ MASK_SOLID | CONTENTS_DEBRIS };

inline constexpr u16 SURF_NODRAW{ 0x80 };
inline constexpr u16 SURF_HITBOX{ 0x8000 };

enum TraceType_t
{
    TRACE_EVERYTHING = 0,
    TRACE_WORLD_ONLY,
    TRACE_ENTITIES_ONLY,
    TRACE_EVERYTHING_FILTER_PROPS,
};

class ITraceFilter
{
public:
    virtual bool        ShouldHitEntity( IHandleEntity *pEntity, int contentsMask ) = 0;
    virtual TraceType_t GetTraceType( ) const                                       = 0;
};

class CTraceFilterWorldAndPropsOnly : public ITraceFilter
{
public:
    bool ShouldHitEntity( IHandleEntity *, int ) override
    {
        return false;
    }

    TraceType_t GetTraceType( ) const override
    {
        return TRACE_EVERYTHING;
    }
};

struct Ray_t
{
    vec3_aligned     m_Start;
    vec3_aligned     m_Delta;
    vec3_aligned     m_StartOffset;
    vec3_aligned     m_Extents;
    const matrix3x4 *m_pWorldAxisTransform{ };
    bool             m_IsRay;
    bool             m_IsSwept;

    void Init( vec3 const &start, vec3 const &end )
    {
        m_Delta       = end - start;
        m_IsSwept     = m_Delta.length_sqr( ) != 0.0f;
        m_Extents     = vec3{ };
        m_IsRay       = true;
        m_StartOffset = vec3{ };
        m_Start       = start;
    }

    void Init( vec3 const &start, vec3 const &end, vec3 const &mins, vec3 const &maxs )
    {
        m_Delta   = end - start;
        m_IsSwept = m_Delta.length( ) != 0.0f;
        m_Extents = maxs - mins;
        m_Extents *= 0.5f;
        m_IsRay       = ( m_Extents.length( ) < 1e-6f );
        m_StartOffset = mins + maxs;
        m_StartOffset *= 0.5f;
        m_Start = start + m_StartOffset;
        m_StartOffset *= -1.0f;
    }
};

static_assert( sizeof( Ray_t ) == 0x50 );

struct cplane_t
{
    vec3  normal;
    float dist;
    u8    type;
    u8    signbits;
    u8    pad[ 2 ];
};

struct csurface_t
{
    const char    *name;
    short          surfaceProps;
    unsigned short flags;
};

class CGameTrace
{
public:
    vec3          startpos;
    vec3          endpos;
    cplane_t      plane;
    float         fraction;
    i32           contents;
    u16           dispFlags;
    bool          allsolid;
    bool          startsolid;
    float         fractionleftsolid;
    csurface_t    surface;
    i32           hitgroup;
    i16           physicsbone;
    u16           worldSurfaceIndex;
    C_BaseEntity *m_pEnt;
    i32           hitbox;

    bool DidHit( ) const
    {
        return fraction < 1.0f || allsolid || startsolid;
    }

    bool DidHitNonWorldEntity( ) const;
};

typedef CGameTrace trace_t;

static_assert( sizeof( trace_t ) == 0x54 );
static_assert( __builtin_offsetof( trace_t, fraction ) == 0x2C );
static_assert( __builtin_offsetof( trace_t, startsolid ) == 0x37 );
static_assert( __builtin_offsetof( trace_t, surface ) == 0x3C );
static_assert( __builtin_offsetof( trace_t, hitgroup ) == 0x44 );
static_assert( __builtin_offsetof( trace_t, m_pEnt ) == 0x4C );

struct CTraceFilterSkipTwoEntities
{
    CTraceFilterSkipTwoEntities( const IHandleEntity *pass, const IHandleEntity *pass2 );

    ITraceFilter *filter( )
    {
        return reinterpret_cast< ITraceFilter * >( this );
    }

    void                *m_vmt{ };
    const IHandleEntity *m_pPassEnt{ };
    i32                  m_collisionGroup{ };
    void                *m_pExtraShouldHitCheckFunction{ };
    const IHandleEntity *m_pPassEnt2{ };
};

static_assert( sizeof( CTraceFilterSkipTwoEntities ) == 0x14 );

bool IsBreakableEntity( C_BaseEntity *entity );

class CEngineTrace
{
public:
    i32  GetPointContents( const vec3 &position, u32 mask, IHandleEntity **entity );
    void ClipRayToEntity( const Ray_t &ray, u32 mask, IHandleEntity *entity, trace_t *trace );
    void TraceRay( const Ray_t &ray, u32 mask, ITraceFilter *filter, trace_t *trace );
};

enum : u16
{
    CHAR_TEX_FLESH     = 'F',
    CHAR_TEX_GRATE     = 'G',
    CHAR_TEX_PLASTIC   = 'L',
    CHAR_TEX_CARDBOARD = 'U',
    CHAR_TEX_WOOD      = 'W',
    CHAR_TEX_GLASS     = 'Y',
};

struct surfacegameprops_t
{
    f32 maxSpeedFactor;
    f32 jumpFactor;
    f32 penetrationModifier;
    f32 damageModifier;
    u16 material;
    u8  climbable;
    u8  pad;
};

struct surfacedata_t
{
    u8                 m_physics_audio_sounds[ 0x50 ];
    surfacegameprops_t game;
};

static_assert( __builtin_offsetof( surfacedata_t, game.penetrationModifier ) == 0x58 );
static_assert( __builtin_offsetof( surfacedata_t, game.damageModifier ) == 0x5C );
static_assert( __builtin_offsetof( surfacedata_t, game.material ) == 0x60 );

class IPhysicsSurfaceProps
{
public:
    const surfacedata_t *GetSurfaceData( i32 surface_data_index );
};

enum
{
    HITGROUP_GENERIC,
    HITGROUP_HEAD,
    HITGROUP_CHEST,
    HITGROUP_STOMACH,
    HITGROUP_LEFTARM,
    HITGROUP_RIGHTARM,
    HITGROUP_LEFTLEG,
    HITGROUP_RIGHTLEG,
    HITGROUP_GEAR = 10,
};

enum CSWeaponType : i32
{
    WEAPONTYPE_KNIFE,
    WEAPONTYPE_PISTOL,
    WEAPONTYPE_SUBMACHINEGUN,
    WEAPONTYPE_RIFLE,
    WEAPONTYPE_SHOTGUN,
    WEAPONTYPE_SNIPER_RIFLE,
    WEAPONTYPE_MACHINEGUN,
    WEAPONTYPE_C4,
    WEAPONTYPE_TASER,
    WEAPONTYPE_GRENADE,
    WEAPONTYPE_EQUIPMENT,
    WEAPONTYPE_STACKABLEITEM,
    WEAPONTYPE_UNKNOWN,
};

class CCSWeaponInfo
{
public:
    u8 m_unknown0[ 0xC8 ];

    CSWeaponType m_WeaponType;

    u8 m_unknown1[ 0x20 ];

    i32 m_iDamage;
    f32 m_flArmorRatio;

    u8 m_unknown2[ 0x4 ];

    f32 m_flPenetration;

    u8 m_unknown3[ 0x8 ];

    f32 m_flRange;
    f32 m_flRangeModifier;

    u8 m_unknown4[ 0x20 ];

    f32 m_flMaxSpeed[ 2 ];

    u8 m_unknown5[ 0x8C ];

    i32 m_iZoomLevels;
};

static_assert( __builtin_offsetof( CCSWeaponInfo, m_WeaponType ) == 0xC8 );
static_assert( __builtin_offsetof( CCSWeaponInfo, m_iDamage ) == 0xEC );
static_assert( __builtin_offsetof( CCSWeaponInfo, m_flArmorRatio ) == 0xF0 );
static_assert( __builtin_offsetof( CCSWeaponInfo, m_flPenetration ) == 0xF8 );
static_assert( __builtin_offsetof( CCSWeaponInfo, m_flRange ) == 0x104 );
static_assert( __builtin_offsetof( CCSWeaponInfo, m_flRangeModifier ) == 0x108 );
static_assert( __builtin_offsetof( CCSWeaponInfo, m_flMaxSpeed ) == 0x12C );
static_assert( __builtin_offsetof( CCSWeaponInfo, m_iZoomLevels ) == 0x1C0 );

inline constexpr i32 MAX_PENETRATIONS{ 4 };
inline constexpr f32 MAX_PENETRATION_DISTANCE{ 90.0f };

enum : i32
{
    GAMETYPE_CLASSIC,
    GAMETYPE_GUNGAME,
    GAMETYPE_TRAINING,
    GAMETYPE_CUSTOM,
    GAMETYPE_COOPERATIVE,
    GAMETYPE_SKIRMISH,
};

inline constexpr i32 FLOW_OUTGOING{ 0 };
inline constexpr i32 FLOW_INCOMING{ 1 };

class INetChannelInfo
{
public:
    float GetLatency( i32 flow );
};

inline constexpr i32 SIGNONSTATE_CONNECTED{ 2 };
inline constexpr i32 SIGNONSTATE_FULL{ 6 };

inline constexpr i32 MAX_CMD_BUFFER{ 4000 };
inline constexpr i32 MAX_NEW_COMMANDS{ 15 };
inline constexpr i32 NUM_BACKUP_COMMANDS{ 2 };

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

    i32 GetNumBytesWritten( ) const
    {
        return ( m_iCurBit + 7 ) >> 3;
    }

    unsigned long *m_pData{ };
    i32            m_nDataBytes{ };
    i32            m_nDataBits{ -1 };
    i32            m_iCurBit{ };
    bool           m_bOverflow{ };
    bool           m_bAssertOnOverflow{ true };
    const char    *m_pDebugName{ };
};

struct std_string_t
{
    const void *m_data{ };
    u8          m_inline[ 12 ]{ };
    usize       m_size{ };
    usize       m_capacity{ 15 };
};

static_assert( sizeof( std_string_t ) == 0x18 );

class INetMessage
{
public:
    void *m_vmt{ };
};

class CCLCMsg_Move : public INetMessage
{
public:
    void         *m_message_vmt{ };
    void         *m_internal_metadata{ };
    i32           num_backup_commands{ };
    i32           num_new_commands{ };
    std_string_t *data{ };
    i32           m_cached_size{ };
    u32           m_has_bits{ };
    bool          m_bReliable{ };
    std_string_t  m_toString{ };
};

static_assert( sizeof( CCLCMsg_Move ) == 0x3C );

class CNETMsg_Tick : public INetMessage
{
public:
    void        *m_message_vmt{ };
    void        *m_internal_metadata{ };
    u32          tick{ };
    u32          host_computationtime{ };
    u32          host_computationtime_std_deviation{ };
    u32          host_framestarttime_std_deviation{ };
    u32          hltv_replay_flags{ };
    i32          m_cached_size{ };
    u32          m_has_bits{ };
    bool         m_bReliable{ };
    std_string_t m_toString{ };
};

static_assert( sizeof( CNETMsg_Tick ) == 0x44 );

class CNetChannel : public INetChannelInfo
{
public:
    bool IsLoopback( );
    bool IsTimingOut( );
    bool SendNetMsg( INetMessage &msg, bool force_reliable, bool voice );
    void SetChoked( );
    i32  SendDatagram( bf_write *data );
    bool CanPacket( );

    u8 m_unknown0[ 0x18 ];

    int m_nOutSequenceNr;
    int m_nInSequenceNr;
    int m_nOutSequenceNrAck;
    int m_nOutReliableState;
    int m_nInReliableState;
    int m_nChokedPackets;

    bf_write m_StreamReliable;

    u8 m_unknown1[ 0xC ];

    bf_write m_StreamUnreliable;
};

static_assert( __builtin_offsetof( CNetChannel, m_nOutSequenceNr ) == 0x18 );
static_assert( __builtin_offsetof( CNetChannel, m_nInReliableState ) == 0x28 );
static_assert( __builtin_offsetof( CNetChannel, m_nChokedPackets ) == 0x2C );
static_assert( __builtin_offsetof( CNetChannel, m_StreamReliable ) == 0x30 );
static_assert( __builtin_offsetof( CNetChannel, m_StreamUnreliable ) == 0x54 );

class CClientState
{
public:
    bool IsPaused( );
    void ForceFullUpdate( const char *reason );

    u8 m_unknown0[ 0x9C ];

    CNetChannel *m_NetChannel;

    u8 m_unknown1[ 0x68 ];

    int m_nSignonState;

    u8 m_unknown2[ 0x4 ];

    double m_flNextCmdTime;

    u8 m_unknown3[ 0x54 ];

    int m_nServerTick;

    u8 m_unknown4[ 0x4 ];

    int m_nDeltaTick;

    u8 m_unknown5[ 0x4B34 ];

    int lastoutgoingcommand;
    int chokedcommands;
    int last_command_ack;
    int last_server_tick;
    int command_ack;

    u8 m_unknown6[ 0x8 ];

    bool ishltv;

    u8 m_unknown7[ 0x1A59F ];

    int m_nHltvReplayDelay;
};

static_assert( __builtin_offsetof( CClientState, m_NetChannel ) == 0x9C );
static_assert( __builtin_offsetof( CClientState, m_nSignonState ) == 0x108 );
static_assert( __builtin_offsetof( CClientState, m_flNextCmdTime ) == 0x110 );
static_assert( __builtin_offsetof( CClientState, m_nServerTick ) == 0x16C );
static_assert( __builtin_offsetof( CClientState, m_nDeltaTick ) == 0x174 );
static_assert( __builtin_offsetof( CClientState, lastoutgoingcommand ) == 0x4CAC );
static_assert( __builtin_offsetof( CClientState, chokedcommands ) == 0x4CB0 );
static_assert( __builtin_offsetof( CClientState, last_command_ack ) == 0x4CB4 );
static_assert( __builtin_offsetof( CClientState, command_ack ) == 0x4CBC );
static_assert( __builtin_offsetof( CClientState, ishltv ) == 0x4CC8 );
static_assert( __builtin_offsetof( CClientState, m_nHltvReplayDelay ) == 0x1F268 );

class CDemoPlayer
{
public:
    bool IsPlayingBack( );
};

class CDemoRecorder
{
public:
    void RecordUserInput( i32 command );
    bool IsRecording( );
};

class CEngineClient
{
public:
    INetChannelInfo *GetNetChannelInfo( );
    bool             GetPlayerInfo( i32 index, player_info_t *info );
    i32              GetPlayerForUserID( i32 user_id );
    i32              GetLocalPlayer( );
    i32              GetMaxClients( );
    void             GetViewAngles( vec3 &angles );
    bool             IsInGame( );
    const VMatrix   &WorldToScreenMatrix( );
};

enum
{
    TEAM_UNASSIGNED,
    TEAM_SPECTATOR,
    TEAM_TERRORIST,
    TEAM_CT,
};

inline constexpr u8 LIFE_ALIVE{ 0 };

enum ItemDefinitionIndex : i16
{
    WEAPON_DEAGLE               = 1,
    WEAPON_ELITE                = 2,
    WEAPON_FIVESEVEN            = 3,
    WEAPON_GLOCK                = 4,
    WEAPON_AK47                 = 7,
    WEAPON_AUG                  = 8,
    WEAPON_AWP                  = 9,
    WEAPON_FAMAS                = 10,
    WEAPON_G3SG1                = 11,
    WEAPON_GALILAR              = 13,
    WEAPON_M249                 = 14,
    WEAPON_M4A1                 = 16,
    WEAPON_MAC10                = 17,
    WEAPON_P90                  = 19,
    WEAPON_UMP45                = 24,
    WEAPON_XM1014               = 25,
    WEAPON_BIZON                = 26,
    WEAPON_MAG7                 = 27,
    WEAPON_NEGEV                = 28,
    WEAPON_SAWEDOFF             = 29,
    WEAPON_TEC9                 = 30,
    WEAPON_TASER                = 31,
    WEAPON_HKP2000              = 32,
    WEAPON_MP7                  = 33,
    WEAPON_MP9                  = 34,
    WEAPON_NOVA                 = 35,
    WEAPON_P250                 = 36,
    WEAPON_SCAR20               = 38,
    WEAPON_SG556                = 39,
    WEAPON_SSG08                = 40,
    WEAPON_KNIFEGG              = 41,
    WEAPON_KNIFE                = 42,
    WEAPON_FLASHBANG            = 43,
    WEAPON_HEGRENADE            = 44,
    WEAPON_SMOKEGRENADE         = 45,
    WEAPON_MOLOTOV              = 46,
    WEAPON_DECOY                = 47,
    WEAPON_INCGRENADE           = 48,
    WEAPON_C4                   = 49,
    WEAPON_HEALTHSHOT           = 57,
    WEAPON_KNIFE_T              = 59,
    WEAPON_M4A1_SILENCER        = 60,
    WEAPON_USP_SILENCER         = 61,
    WEAPON_CZ75A                = 63,
    WEAPON_REVOLVER             = 64,
    WEAPON_TAGRENADE            = 68,
    WEAPON_BAYONET              = 500,
    WEAPON_KNIFE_FLIP           = 505,
    WEAPON_KNIFE_GUT            = 506,
    WEAPON_KNIFE_KARAMBIT       = 507,
    WEAPON_KNIFE_M9_BAYONET     = 508,
    WEAPON_KNIFE_TACTICAL       = 509,
    WEAPON_KNIFE_FALCHION       = 512,
    WEAPON_KNIFE_SURVIVAL_BOWIE = 514,
    WEAPON_KNIFE_BUTTERFLY      = 515,
    WEAPON_KNIFE_PUSH           = 516,
};

inline constexpr usize MAX_VIEWMODELS{ 2 };

enum fieldtype_t : int
{
    FIELD_VOID = 0,
    FIELD_FLOAT,
    FIELD_STRING,
    FIELD_VECTOR,
    FIELD_QUATERNION,
    FIELD_INTEGER,
    FIELD_BOOLEAN,
};

enum
{
    TD_OFFSET_NORMAL = 0,
    TD_OFFSET_PACKED = 1,
    TD_OFFSET_COUNT,
};

struct datamap_t;

struct typedescription_t
{
    fieldtype_t        fieldType{ };
    const char        *fieldName{ };
    int                fieldOffset{ };
    unsigned short     fieldSize{ };
    short              flags{ };
    const char        *externalName{ };
    void              *pSaveRestoreOps{ };
    void              *inputFunc{ };
    datamap_t         *td{ };
    int                fieldSizeInBytes{ };
    typedescription_t *override_field{ };
    int                override_count{ };
    float              fieldTolerance{ };
    int                flatOffset[ TD_OFFSET_COUNT ]{ };
    unsigned short     flatGroup{ };
};

static_assert( sizeof( typedescription_t ) == 0x3C );
static_assert( __builtin_offsetof( typedescription_t, flatOffset ) == 0x30 );

inline constexpr int PC_COPYTYPE_COUNT{ 2 };

struct flattenedoffsets_t
{
    typedescription_t *m_Flattened{ };
    int                m_nAllocationCount{ };
    int                m_nGrowSize{ };
    int                m_nSize{ };
    u8                 m_pad[ 0x20 ];
};

static_assert( sizeof( flattenedoffsets_t ) == 0x30 );

struct optimized_datamap_t
{
    flattenedoffsets_t m_Info[ PC_COPYTYPE_COUNT ];
};

struct datamap_t
{
    typedescription_t   *dataDesc;
    int                  dataNumFields;
    const char          *dataClassName;
    datamap_t           *baseMap;
    int                  m_nPackedSize;
    optimized_datamap_t *m_pOptimizedDataMap;
};

static_assert( sizeof( datamap_t ) == 0x18 );

inline constexpr short FTYPEDESC_INSENDTABLE{ 0x0100 };
inline constexpr short FTYPEDESC_NOERRORCHECK{ 0x0400 };

inline constexpr int  PC_NETWORKED_ONLY{ 1 };
inline constexpr int  PC_EVERYTHING{ 2 };
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

static_assert( sizeof( CPredictionCopy ) == 0x40 );

class IMemAlloc
{
public:
    void *Alloc( usize size );
    void *Realloc( void *mem, usize size );
    void  Free( void *mem );
};

class C_BasePlayer;

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
};

class CPrediction
{
public:
    void GetLocalViewAngles( vec3 &angles );
    void SetLocalViewAngles( vec3 &angles );
    void RunCommand( C_BasePlayer *player, CUserCmd *cmd, CMoveHelperClient *move_helper );

    u8   m_pad0[ 0x8 ];
    bool m_bInPrediction;
    u8   m_pad1[ 0xF ];
    bool m_bFirstTimePredicted;
};

static_assert( __builtin_offsetof( CPrediction, m_bInPrediction ) == 0x8 );
static_assert( __builtin_offsetof( CPrediction, m_bFirstTimePredicted ) == 0x18 );

class CGameMovement;

struct model_t;

inline constexpr i32 MAXSTUDIOBONES{ 256 };
inline constexpr i32 MAXSTUDIOPOSEPARAM{ 24 };

inline constexpr usize ANIMSTATE_SIZE{ 0x344 };

inline constexpr i32 ACT_VM_HAULBACK{ 208 };
inline constexpr i32 BONE_USED_BY_HITBOX{ 0x100 };
inline constexpr i32 BONE_USED_BY_ATTACHMENT{ 0x200 };
inline constexpr i32 BONE_USED_BY_ANYTHING{ 0x0007FF00 };

class CStudioHdr;

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.h#L23-L38
enum animstate_layer_t : i32
{
    ANIMATION_LAYER_AIMMATRIX,
    ANIMATION_LAYER_WEAPON_ACTION,
    ANIMATION_LAYER_WEAPON_ACTION_RECROUCH,
    ANIMATION_LAYER_ADJUST,
    ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL,
    ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB,
    ANIMATION_LAYER_MOVEMENT_MOVE,
    ANIMATION_LAYER_MOVEMENT_STRAFECHANGE,
    ANIMATION_LAYER_WHOLE_BODY,
    ANIMATION_LAYER_FLASHED,
    ANIMATION_LAYER_FLINCH,
    ANIMATION_LAYER_ALIVELOOP,
    ANIMATION_LAYER_LEAN,
    ANIMATION_LAYER_COUNT
};

// ai_activity.h's ACT_CSGO_* as this build registers them (server 0x101078FA-0x10107995)
enum csgo_activity_t : i32
{
    ACT_CSGO_DEPLOY                    = 972,
    ACT_CSGO_IDLE_TURN_BALANCEADJUST   = 979,
    ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING = 980,
    ACT_CSGO_ALIVE_LOOP                = 981,
    ACT_CSGO_JUMP                      = 985,
    ACT_CSGO_FALL                      = 986,
    ACT_CSGO_CLIMB_LADDER              = 987,
    ACT_CSGO_LAND_LIGHT                = 988,
    ACT_CSGO_LAND_HEAVY                = 989,
};

struct C_AnimationLayer
{
    float       m_flLayerAnimtime;
    float       m_flLayerFadeOuttime;
    CStudioHdr *m_pDispatchedStudioHdr;
    i32         m_nDispatchedSrc;
    i32         m_nDispatchedDst;
    i32         m_nOrder;
    i32         m_nSequence;
    float       m_flPrevCycle;
    float       m_flWeight;
    float       m_flWeightDeltaRate;
    float       m_flPlaybackRate;
    float       m_flCycle;
    void       *m_pOwner;
    i32         m_nInvalidatePhysicsBits;
};

static_assert( sizeof( C_AnimationLayer ) == 0x38 );
static_assert( __builtin_offsetof( C_AnimationLayer, m_nOrder ) == 0x14 );
static_assert( __builtin_offsetof( C_AnimationLayer, m_nSequence ) == 0x18 );
static_assert( __builtin_offsetof( C_AnimationLayer, m_flCycle ) == 0x2C );

struct anim_overlay_t
{
    C_AnimationLayer *m_layers;
    i32               m_allocation_count;
    i32               m_grow_size;
    i32               m_count;

    i32 count( ) const
    {
        return this->m_layers ? this->m_count : 0;
    }

    C_AnimationLayer &operator[]( i32 index )
    {
        return this->m_layers[ index ];
    }
};

inline constexpr i32 STUDIO_ID{ 0x54534449 };
inline constexpr i32 STUDIO_VERSION{ 49 };
inline constexpr u32 STUDIO_NUMLOCALSEQ{ 0xBC };
inline constexpr u32 STUDIO_NUMINCLUDEMODELS{ 0x150 };
inline constexpr u32 STUDIO_HEADER_SIZE{ 0x158 };
inline constexpr u32 SEQDESC_SIZE{ 0xD4 };
inline constexpr u32 SEQDESC_ACTWEIGHT{ 0x14 };

struct mstudiobbox_t
{
    i32  bone{ };
    i32  group{ };
    vec3 bbmin{ };
    vec3 bbmax{ };
    i32  szhitboxnameindex{ };
    vec3 angOffsetOrientation{ };
    f32  flCapsuleRadius{ };
    i32  unused[ 4 ]{ };
};

static_assert( sizeof( mstudiobbox_t ) == 0x44 );

struct mstudiohitboxset_t
{
    i32 sznameindex{ };
    i32 numhitboxes{ };
    i32 hitboxindex{ };

    const mstudiobbox_t *pHitbox( i32 index ) const
    {
        return reinterpret_cast< const mstudiobbox_t * >( reinterpret_cast< const u8 * >( this ) + this->hitboxindex ) + index;
    }
};

struct mstudiobone_t
{
    i32       sznameindex;
    i32       parent;
    i32       bonecontroller[ 6 ];
    vec3      pos;
    float     quat[ 4 ];
    vec3      rot;
    vec3      posscale;
    vec3      rotscale;
    matrix3x4 poseToBone;
    float     qAlignment[ 4 ];
    i32       flags;
    i32       proctype;
    i32       procindex;
    i32       physicsbone;
    i32       surfacepropidx;
    i32       contents;
    i32       unused[ 8 ];
};

static_assert( sizeof( mstudiobone_t ) == 216 );
static_assert( __builtin_offsetof( mstudiobone_t, flags ) == 160 );

struct mstudiomesh_t
{
    i32 material;
    i32 modelindex;
    i32 numvertices;
    i32 vertexoffset;
    i32 numflexes;
    i32 flexindex;
    i32 materialtype;
    i32 materialparam;
    i32 meshid;
    u8  pad24[ 80 ];
};

static_assert( sizeof( mstudiomesh_t ) == 116 );

struct mstudiomodel_t
{
    char  name[ 64 ];
    i32   type;
    float boundingradius;
    i32   nummeshes;
    i32   meshindex;
    u8    pad50[ 68 ];

    const mstudiomesh_t *pMesh( i32 index ) const
    {
        return reinterpret_cast< const mstudiomesh_t * >( reinterpret_cast< const u8 * >( this ) + this->meshindex ) + index;
    }
};

static_assert( sizeof( mstudiomodel_t ) == 148 );
static_assert( __builtin_offsetof( mstudiomodel_t, nummeshes ) == 72 );

struct mstudiobodyparts_t
{
    i32 sznameindex;
    i32 nummodels;
    i32 base;
    i32 modelindex;

    const mstudiomodel_t *pModel( i32 index ) const
    {
        return reinterpret_cast< const mstudiomodel_t * >( reinterpret_cast< const u8 * >( this ) + this->modelindex ) + index;
    }
};

inline constexpr i32 STUDIOHDR_FLAGS_STATIC_PROP{ 0x10 };

struct studiohdr_t
{
    i32  id{ };
    i32  version{ };
    i32  checksum{ };
    char name[ 64 ]{ };
    i32  length{ };
    vec3 eyeposition{ };
    vec3 illumposition{ };
    vec3 hull_min{ };
    vec3 hull_max{ };
    vec3 view_bbmin{ };
    vec3 view_bbmax{ };
    i32  flags{ };
    i32  numbones{ };
    i32  boneindex{ };
    i32  numbonecontrollers{ };
    i32  bonecontrollerindex{ };
    i32  numhitboxsets{ };
    i32  hitboxsetindex{ };

    const mstudiohitboxset_t *pHitboxSet( i32 index ) const
    {
        if ( index < 0 || index >= this->numhitboxsets )
        {
            return nullptr;
        }

        return reinterpret_cast< const mstudiohitboxset_t * >( reinterpret_cast< const u8 * >( this ) + this->hitboxsetindex ) + index;
    }

    i32 numlocalanim{ };
    i32 localanimindex{ };
    i32 numlocalseq{ };
    i32 localseqindex{ };
    i32 activitylistversion{ };
    i32 eventsindexed{ };
    i32 numtextures{ };
    i32 textureindex{ };
    i32 numcdtextures{ };
    i32 cdtextureindex{ };
    i32 numskinref{ };
    i32 numskinfamilies{ };
    i32 skinindex{ };
    i32 numbodyparts{ };
    i32 bodypartindex{ };

    const mstudiobone_t *pBone( i32 index ) const
    {
        return reinterpret_cast< const mstudiobone_t * >( reinterpret_cast< const u8 * >( this ) + this->boneindex ) + index;
    }

    const mstudiobodyparts_t *pBodypart( i32 index ) const
    {
        return reinterpret_cast< const mstudiobodyparts_t * >( reinterpret_cast< const u8 * >( this ) + this->bodypartindex ) + index;
    }
};

static_assert( __builtin_offsetof( studiohdr_t, hitboxsetindex ) == 0xB0 );
static_assert( __builtin_offsetof( studiohdr_t, numbones ) == 0x9C );
static_assert( __builtin_offsetof( studiohdr_t, numbodyparts ) == 0xE8 );
static_assert( __builtin_offsetof( studiohdr_t, numlocalseq ) == 0xBC );

struct mstudioactivitymodifier_t
{
    i32 sznameindex;

    const char *pszName( ) const
    {
        return reinterpret_cast< const char * >( this ) + this->sznameindex;
    }
};

struct mstudioseqdesc_t
{
    u8    m_unknown0[ 0x68 ];
    float fadeintime;
    float fadeouttime;
    u8    m_unknown1[ 0x48 ];
    i32   activitymodifierindex;
    i32   numactivitymodifiers;
    u8    m_unknown2[ 0x14 ];

    const mstudioactivitymodifier_t *pActivityModifier( i32 index ) const
    {
        return reinterpret_cast< const mstudioactivitymodifier_t * >( reinterpret_cast< const u8 * >( this ) + this->activitymodifierindex ) + index;
    }
};

static_assert( __builtin_offsetof( mstudioseqdesc_t, fadeintime ) == 0x68 );
static_assert( __builtin_offsetof( mstudioseqdesc_t, activitymodifierindex ) == 0xB8 );
static_assert( sizeof( mstudioseqdesc_t ) == 0xD4 );

class CStudioHdr
{
public:
    i32                     GetNumSeq( ) const;
    const mstudioseqdesc_t &pSeqdesc( i32 sequence );
    i32                     GetSequenceActivity( i32 sequence );

    const studiohdr_t *m_pStudioHdr;
    const u8          *m_pVModel;
};

class CModelInfoClient
{
public:
    i32                GetModelIndex( const char *name );
    const studiohdr_t *GetStudiomodel( const model_t *model );
};

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/client/iviewrender_beams.h
struct BeamInfo_t
{
    i32         m_nType{ };
    void       *m_pStartEnt{ };
    i32         m_nStartAttachment{ };
    void       *m_pEndEnt{ };
    i32         m_nEndAttachment{ };
    vec3        m_vecStart{ };
    vec3        m_vecEnd{ };
    i32         m_nModelIndex{ -1 };
    const char *m_pszModelName{ };
    i32         m_nHaloIndex{ -1 };
    const char *m_pszHaloName{ };
    float       m_flHaloScale{ };
    float       m_flLife{ };
    float       m_flWidth{ };
    float       m_flEndWidth{ };
    float       m_flFadeLength{ };
    float       m_flAmplitude{ };
    float       m_flBrightness{ };
    float       m_flSpeed{ };
    i32         m_nStartFrame{ };
    float       m_flFrameRate{ };
    float       m_flRed{ };
    float       m_flGreen{ };
    float       m_flBlue{ };
    bool        m_bRenderable{ true };
    i32         m_nSegments{ -1 };
    i32         m_nFlags{ };
    vec3        m_vecCenter{ };
    float       m_flStartRadius{ };
    float       m_flEndRadius{ };
};

static_assert( __builtin_offsetof( BeamInfo_t, m_vecStart ) == 0x14 );
static_assert( __builtin_offsetof( BeamInfo_t, m_nModelIndex ) == 0x2C );
static_assert( __builtin_offsetof( BeamInfo_t, m_flLife ) == 0x40 );
static_assert( __builtin_offsetof( BeamInfo_t, m_nStartFrame ) == 0x5C );
static_assert( __builtin_offsetof( BeamInfo_t, m_flRed ) == 0x64 );
static_assert( __builtin_offsetof( BeamInfo_t, m_bRenderable ) == 0x70 );
static_assert( __builtin_offsetof( BeamInfo_t, m_nFlags ) == 0x78 );

struct Beam_t;

class CViewRenderBeams
{
public:
    Beam_t *CreateBeamPoints( BeamInfo_t &info );
};

#if defined( ATLAS_DEV )
class CBaseAnimating
{
public:
    void SetupBones( matrix3x4 *bone_to_world, i32 bone_mask );
    void InvalidateBoneCache( );

    const vec3 &m_vecAbsOrigin( ) const
    {
        return *reinterpret_cast< const vec3 * >( reinterpret_cast< const u8 * >( this ) + 0x1D8 );
    }

    CUserCmd *&m_pCurrentCommand( )
    {
        return *reinterpret_cast< CUserCmd ** >( reinterpret_cast< u8 * >( this ) + 0xF60 );
    }
};

struct LagRecord
{
    i32   m_fFlags;
    vec3  m_vecOrigin;
    vec3  m_vecAngles;
    vec3  m_vecMinsPreScaled;
    vec3  m_vecMaxsPreScaled;
    float m_flSimulationTime;
    u8    m_layerRecords[ 15 * 16 ];
    i32   m_masterSequence;
    float m_masterCycle;
    float m_poseParameters[ 24 ];
};

static_assert( sizeof( LagRecord ) == 400 );

struct LagRecordNode
{
    LagRecord      m_record;
    LagRecordNode *m_previous;
    LagRecordNode *m_next;
};

struct LagCompensatedEntity
{
    bool           m_bRestoreEntity;
    u8             m_unknown0[ 0xF ];
    LagRecordNode *m_head;
};

static_assert( __builtin_offsetof( LagCompensatedEntity, m_head ) == 0x10 );

struct LagCompensatedEntityNode
{
    u16                   m_left;
    u16                   m_right;
    u16                   m_parent;
    u16                   m_tag;
    u32                   m_handle;
    LagCompensatedEntity *m_entity;
};

static_assert( sizeof( LagCompensatedEntityNode ) == 0x10 );

inline constexpr i32 LAG_COMPENSATE_HITBOXES_ALONG_RAY{ 2 };

class ILagCompensationManager
{
public:
    void StartLagCompensation( CBaseAnimating *player, i32 type, const vec3 &weapon_pos, const vec3 &weapon_angles, float weapon_range );
    void FinishLagCompensation( CBaseAnimating *player );

    const LagCompensatedEntityNode *nodes( ) const
    {
        return *reinterpret_cast< LagCompensatedEntityNode *const * >( reinterpret_cast< const u8 * >( this ) + 0x8 );
    }

    u16 root( ) const
    {
        return *reinterpret_cast< const u16 * >( reinterpret_cast< const u8 * >( this ) + 0x14 );
    }
};

class IServerUnknown : public IHandleEntity
{
public:
    virtual ICollideable   *GetCollideable( ) = 0;
    virtual void           *GetNetworkable( ) = 0;
    virtual CBaseAnimating *GetBaseEntity( )  = 0;
};

inline constexpr i32 FL_EDICT_FREE{ 1 << 1 };

struct edict_t
{
    i32             m_fStateFlags{ };
    i32             m_NetworkSerialNumber{ };
    void           *m_pNetworkable{ };
    IServerUnknown *m_pUnk{ };
};

static_assert( sizeof( edict_t ) == 0x10 );

struct CGlobalVars
{
    float       realtime{ };
    i32         framecount{ };
    float       absoluteframetime{ };
    float       absoluteframestarttimestddev{ };
    float       curtime{ };
    float       frametime{ };
    i32         maxClients{ };
    i32         tickcount{ };
    float       interval_per_tick{ };
    float       interpolation_amount{ };
    i32         simTicksThisFrame{ };
    i32         network_protocol{ };
    void       *pSaveData{ };
    bool        m_bClient{ };
    bool        m_bRemoteClient{ };
    i32         nTimestampNetworkingBase{ };
    i32         nTimestampRandomizeWindow{ };
    const char *mapname{ };
    const char *mapGroupName{ };
    i32         mapversion{ };
    const char *startspot{ };
    i32         eLoadType{ };
    bool        bMapLoadFailed{ };
    bool        deathmatch{ };
    bool        coop{ };
    bool        teamplay{ };
    i32         maxEntities{ };
    i32         serverCount{ };
    edict_t    *pEdicts{ };
};

static_assert( __builtin_offsetof( CGlobalVars, maxClients ) == 0x18 );
static_assert( __builtin_offsetof( CGlobalVars, pEdicts ) == 0x60 );

class IPlayerInfoManager
{
public:
    CGlobalVars *GetGlobalVars( );
};
#endif // ATLAS_DEV

class IClientRenderable
{
public:
    virtual IClientUnknown *GetIClientUnknown( ) = 0;
};

class IRefCounted
{
public:
    virtual i32 AddRef( )  = 0;
    virtual i32 Release( ) = 0;
};

class CFunctor : public IRefCounted
{
public:
    virtual ~CFunctor( )        = default;
    virtual void operator( )( ) = 0;

    u32 m_nUserID{ };
};

class ICallQueue
{
public:
    void QueueFunctor( CFunctor *functor );
};

inline constexpr i32 MATERIAL_VIEW{ 0 };
inline constexpr i32 MATERIAL_PROJECTION{ 1 };

class IMatRenderContext
{
public:
    void        Release( );
    ICallQueue *GetCallQueue( );
    void        GetMatrix( i32 mode, VMatrix *matrix );
};

class CMaterialSystem
{
public:
    IMatRenderContext *GetRenderContext( );
};

struct CVertexBufferDx8
{
    void *m_vertex_buffer;
    u8    pad04[ 20 ];
    i32   m_vertex_count;
    i32   m_vertex_size;
};

static_assert( __builtin_offsetof( CVertexBufferDx8, m_vertex_size ) == 0x1C );

struct CIndexBufferDx8
{
    void *m_index_buffer;
};

struct CMeshDX8
{
    u8                pad00[ 16 ];
    u64               m_vertex_format;
    CVertexBufferDx8 *m_vertex_buffer;
    CIndexBufferDx8  *m_index_buffer;

    void HandleLateCreation( );
};

static_assert( __builtin_offsetof( CMeshDX8, m_vertex_format ) == 0x10 );
static_assert( __builtin_offsetof( CMeshDX8, m_index_buffer ) == 0x1C );

inline constexpr u64 VERTEX_NORMAL{ 0x2 };
inline constexpr u64 VERTEX_BONE_INDEX{ 0x80 };
inline constexpr u64 VERTEX_FORMAT_COMPRESSED{ 0x400 };
inline constexpr u64 VERTEX_FORMAT_PAD_POS_NORM{ 0x800 };
inline constexpr u64 VERTEX_BONE_WEIGHT_MASK{ 0x7ull << 12 };

inline constexpr i32 MESHGROUP_IS_HWSKINNED{ 0x2 };
inline constexpr u8  STRIP_IS_QUADLIST_REG{ 0x2 };
inline constexpr u8  STRIP_IS_QUADLIST_EXTRA{ 0x4 };

inline constexpr i32 MAX_HARDWARE_BONES{ 53 };

struct BoneStateChangeHeader_t
{
    i32 hardwareID;
    i32 newBoneID;
};

#pragma pack( push, 1 )
struct StripHeader_t
{
    i32 numIndices;
    i32 indexOffset;
    i32 numVerts;
    i32 vertOffset;
    i16 numBones;
    u8  flags;
    i32 numBoneStateChanges;
    i32 boneStateChangeOffset;
    i32 numTopologyIndices;
    i32 topologyOffset;

    const BoneStateChangeHeader_t *pBoneStateChange( i32 index ) const
    {
        return reinterpret_cast< const BoneStateChangeHeader_t * >( reinterpret_cast< const u8 * >( this ) + this->boneStateChangeOffset ) + index;
    }
};
#pragma pack( pop )

static_assert( sizeof( StripHeader_t ) == 35 );

struct studiomeshgroup_t
{
    CMeshDX8      *m_pMesh;
    i32            m_NumStrips;
    i32            m_Flags;
    StripHeader_t *m_pStripData;
    u16           *m_pGroupIndexToMeshIndex;
    i32            m_NumVertices;
    i32           *m_pUniqueTris;
    u16           *m_pIndices;
    u8             pad20[ 4 ];
    bool           m_MeshNeedsRestore;
    i16            m_ColorMeshID;
    void          *m_pMorph;
};

static_assert( sizeof( studiomeshgroup_t ) == 44 );
static_assert( __builtin_offsetof( studiomeshgroup_t, m_MeshNeedsRestore ) == 36 );

struct studiomeshdata_t
{
    i32                m_NumGroup;
    studiomeshgroup_t *m_pMeshGroup;
};

struct studioloddata_t
{
    studiomeshdata_t *m_pMeshData;
    float             m_SwitchPoint;
    i32               numMaterials;
    void            **ppMaterials;
    i32              *pMaterialFlags;
    i32              *m_pHWMorphDecalBoneRemap;
    i32               m_nDecalBoneCount;
    u8                pad1c[ 4 ];
};

static_assert( sizeof( studioloddata_t ) == 32 );

struct studiohwdata_t
{
    i32              m_RootLOD;
    i32              m_NumLODs;
    studioloddata_t *m_pLODs;
    i32              m_NumStudioMeshes;
};

struct DrawModelResults_t;
struct StudioRenderContext_t;
struct FlexWeights_t;

struct DrawModelInfo_t
{
    studiohdr_t    *m_pStudioHdr;
    studiohwdata_t *m_pHardwareData;
    void           *m_Decals;
    i32             m_Skin;
    i32             m_Body;
    i32             m_HitboxSet;
    void           *m_pClientEntity;
    i32             m_Lod;
};

static_assert( __builtin_offsetof( DrawModelInfo_t, m_Body ) == 0x10 );
static_assert( __builtin_offsetof( DrawModelInfo_t, m_Lod ) == 0x1C );

inline constexpr i32 STUDIORENDER_DRAW_TRANSLUCENT_ONLY{ 0x2 };
inline constexpr i32 STUDIORENDER_DRAW_GROUP_MASK{ 0x3 };
inline constexpr i32 STUDIORENDER_SHADOWDEPTHTEXTURE{ 0x200 };
inline constexpr i32 STUDIORENDER_SSAODEPTHTEXTURE{ 0x8000 };

class CStudioRenderContext
{
};

class CStudioRender
{
};

inline constexpr i32 BONE_USED_BY_VERTEX_LOD0{ 0x400 };

using MDLHandle_t = u16;

inline constexpr i32 MOD_STUDIO{ 3 };

struct model_t
{
    u8          pad000[ 0x110 ];
    i32         type;
    u8          pad114[ 0x24 ];
    MDLHandle_t studio;
};

static_assert( __builtin_offsetof( model_t, type ) == 0x110 );
static_assert( __builtin_offsetof( model_t, studio ) == 0x138 );

class IMDLCache
{
public:
    const studiohdr_t    *GetStudioHdr( MDLHandle_t handle );
    const studiohwdata_t *GetHardwareData( MDLHandle_t handle );
};

class CRecvProxyData
{
public:
    const RecvProp *m_pRecvProp;
    i32             m_pad;
    union
    {
        i32   m_Int;
        float m_Vector[ 3 ];
    };
};

class IGameEvent
{
public:
    const char *GetName( );
    i32         GetInt( const char *name, i32 default_value = 0 );
    float       GetFloat( const char *name, float default_value = 0.0f );
    const char *GetString( const char *name, const char *default_value = "" );
    void        SetString( const char *name, const char *value );
};

class IGameEventListener2
{
public:
    virtual ~IGameEventListener2( ) { };
    virtual void FireGameEvent( IGameEvent *event ) = 0;
    virtual int  GetEventDebugID( )                 = 0;
};

class CGameEventManager
{
public:
    bool AddListener( IGameEventListener2 *listener, const char *name, bool server_side );
};

class CUniformRandomStream
{
public:
    void  Construct( );
    void  SetSeed( i32 seed );
    float RandomFloat( float minimum, float maximum );
    i32   RandomInt( i32 minimum, i32 maximum );

private:
    void *m_vtable{ };
    i32   m_idum{ };
    i32   m_iy{ };
    i32   m_iv[ 32 ]{ };
    u32   m_mutex_owner{ };
    i32   m_mutex_depth{ };
};

static_assert( sizeof( CUniformRandomStream ) == 148 );

enum DataUpdateType_t
{
    DATA_UPDATE_CREATED,
    DATA_UPDATE_DATATABLE_CHANGED,
};

inline constexpr i32 MAX_EDICTS{ 2048 };

inline constexpr usize ITEM_CUSTOM_MATERIAL{ 0x218 };
inline constexpr usize ITEM_ATTRIBUTE_LIST{ 0x234 };

inline constexpr usize ATTRIBUTE_SIZE{ 24 };
inline constexpr usize ATTRIBUTE_DEFINITION{ 4 };
inline constexpr usize LIST_MEMORY{ 4 };
inline constexpr usize LIST_SIZE{ 0x10 };

inline constexpr usize ENTITY_MATERIAL_OWNER{ 0x9D8 };
inline constexpr usize ITEM_MATERIAL_OWNER{ 0x10 };
inline constexpr usize WEAPON_MATERIALS_CHECKED{ 0x32DD };
inline constexpr usize ITEM_COMPOSITE_TEXTURES{ 0x21C };

inline constexpr usize MY_WEAPONS{ 64 };

inline constexpr usize CUSTOM_NAME_SIZE{ 161 };

inline constexpr i32 GLOVES_BODYGROUP_EMPTY{ 1 };

typedef void *FileHandle_t;

class IFileSystem
{
public:
    FileHandle_t Open( const char *file_name, const char *options, const char *path_id );
    u32          Size( FileHandle_t file );
    i32          Read( void *destination, i32 size, FileHandle_t file );
    void         Close( FileHandle_t file );

private:
    void *base( );
};
