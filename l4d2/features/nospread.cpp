#include "nospread.hpp"

#include "hitscan.hpp"

ATLAS_NO_DESTROY no_spread g_nospread{ };

struct crc32_table_t
{
    u32 m_values[ 256 ]{ };

    constexpr crc32_table_t( )
    {
        for ( u32 index{ }; index < 256; ++index )
        {
            u32 value{ index };
            for ( i32 bit{ }; bit < 8; ++bit )
            {
                value = ( value >> 1 ) ^ ( ( value & 1u ) ? 0xEDB88320u : 0u );
            }

            this->m_values[ index ] = value;
        }
    }
};

static constexpr crc32_table_t CRC32_TABLE{ };

static constexpr float SPREAD_SPEED_CAP{ 220.0f };
static constexpr float SPREAD_SPEED_SCALE{ 1.0f / SPREAD_SPEED_CAP };
static constexpr float SPREAD_WALK_SCALE{ 0.75f };
static constexpr float SPREAD_DUCK_SCALE{ 0.5f };
static constexpr float SPREAD_PENALTY_SCALE{ 10.0f };
static constexpr float SPREAD_CEILING{ 90.0f };

static constexpr u32 UPGRADE_LASER_SIGHT{ 2 };

// this is ghetto and im lazy
ATLAS_FORCE_INLINE i32 default_fov( C_TerrorPlayer *player )
{
    const i32 fov{ player->m_iDefaultFOV( ) };
    return fov > 0 ? fov : 90;
}

ATLAS_FORCE_INLINE i32 server_fov( C_TerrorPlayer *player )
{
    const i32 fov{ player->m_iFOV( ) };
    return fov != 0 ? fov : default_fov( player );
}

// SharedRandomFloat: CRC32( seed, extra, name )
ATLAS_FORCE_INLINE float shared_random_float( const char *name, float minimum, float maximum, i32 extra, i32 seed )
{
    u32       crc{ 0xFFFFFFFFu };
    const u32 words[ 2 ]{ static_cast< u32 >( seed ), static_cast< u32 >( extra ) };

    for ( const u32 word : words )
    {
        for ( i32 shift{ }; shift < 32; shift += 8 )
        {
            crc = CRC32_TABLE.m_values[ ( crc ^ ( word >> shift ) ) & 0xFFu ] ^ ( crc >> 8 );
        }
    }

    for ( ; *name; ++name )
    {
        crc = CRC32_TABLE.m_values[ ( crc ^ static_cast< u8 >( *name ) ) & 0xFFu ] ^ ( crc >> 8 );
    }

    g_l4d2.m_random.SetSeed( static_cast< i32 >( crc ^ 0xFFFFFFFFu ) );
    return g_l4d2.m_random.RandomFloat( minimum, maximum );
}

ATLAS_FORCE_INLINE vec3 spread_offset( i32 seed, float spread )
{
    return vec3{
        shared_random_float( ATLAS_XORSTR( "CTerrorGun::FireBullet HorizSpread" ).get( ), -spread, spread, 0, seed ),
        shared_random_float( ATLAS_XORSTR( "CTerrorGun::FireBullet VertSpread" ).get( ), -spread, spread, 0, seed ),
        0.0f
    };
}

ATLAS_FORCE_INLINE void aim_away_from_spread( float spread )
{
    const vec3 offset{ spread_offset( g_cl.m_cmd->random_seed, spread ) };
    const vec3 original{ g_cl.m_cmd->viewangles };

    g_cl.m_cmd->viewangles -= offset;
    math::clamp_angles( g_cl.m_cmd->viewangles );
    g_cl.fix_movement( g_cl.m_cmd, original );
}

ATLAS_FORCE_INLINE float spread_per_shot( CTerrorWeaponInfo *info, C_TerrorGun *gun, i32 fov, i32 fov_default )
{
    float kick{ static_cast< float >( fov ) * info->m_flSpreadPerShot( ) / static_cast< float >( fov_default ) };

    if ( gun->HasUpgrade( UPGRADE_LASER_SIGHT ) )
    {
        kick *= g_l4d2.upgrade_laser_sight_spread_factor->GetFloat( );
    }

    return kick;
}

float no_spread::target_spread( CTerrorWeaponInfo *info, C_TerrorGun *gun, i32 fov_ratio ) const
{
    C_TerrorPlayer *player{ g_cl.m_local };

    const i32  flags{ player->m_fFlags( ) };
    const bool incapacitated{ player->IsIncapacitated( ) };

    float movement{ SPREAD_SPEED_CAP };
    float base{ info->m_flMinInAirSpread( ) };

    if ( flags & FL_ONGROUND )
    {
        const float speed{ math::clamp( player->m_vecVelocity( ).length( ), 0.0f, SPREAD_SPEED_CAP ) };

        movement = speed * ( ( flags & FL_DUCKING ) ? SPREAD_DUCK_SCALE : SPREAD_WALK_SCALE );
        base     = ( ( flags & FL_DUCKING ) && !incapacitated ) ? info->m_flMinDuckingSpread( )
                                                                : info->m_flMinStandingSpread( );
    }

    float target{ ( info->m_flMaxMovementSpread( ) - base ) * movement * SPREAD_SPEED_SCALE + base };

    if ( incapacitated && gun->GetClientClass( )->GetClassID( ) != EL4D2ClassID::CMagnumPistol )
    {
        target += g_l4d2.survivor_incapacitated_accuracy_penalty->GetFloat( ) * SPREAD_PENALTY_SCALE;
    }

    if ( player->m_tongueOwner( ).Get( ) )
    {
        target += g_l4d2.tongue_victim_accuracy_penalty->GetFloat( ) * SPREAD_PENALTY_SCALE;
    }

    if ( gun->HasUpgrade( UPGRADE_LASER_SIGHT ) )
    {
        target *= g_l4d2.upgrade_laser_sight_spread_factor->GetFloat( );
    }

    return static_cast< float >( fov_ratio ) * target;
}

void no_spread::clear( )
{
    this->m_armed         = false;
    this->m_gun           = nullptr;
    this->m_shadow_valid  = false;
    this->m_shadow_weapon = -1;
}

void no_spread::pre_predict( )
{
    this->m_armed = false;

    if ( ( !g_cfg.m_misc.m_no_spread && !g_hitscan.active( ) ) ||    // neither no spread nor the hitscan wants it
         !g_cl.m_weapon ||                                           // we have no weapon
         g_cl.m_cmd->weaponselect != 0 ||                            // a weapon switch is queued
         g_cl.m_local->m_hActiveWeapon( ).Get( ) != g_cl.m_weapon || // the active weapon is not the cached one yet
         !g_cl.m_weapon->IsTerrorGun( ) )                            // not a gun
    {
        this->m_shadow_valid = false;
        return;
    }

    C_TerrorGun *gun{ g_cl.m_weapon->As< C_TerrorGun * >( ) };
    const i32    handle{ g_cl.m_local->m_hActiveWeapon( ).ToInt( ) };

    if ( this->m_shadow_valid && this->m_shadow_weapon != handle )
    {
        this->m_shadow_valid = false;
    }

    // @::CL_RunPrediction already ran every earlier command, so the client copy holds the value
    // this command's UpdateSpread starts from;
    this->m_gun    = gun;
    this->m_spread = this->m_shadow_valid ? this->m_shadow : gun->m_flSpread( );
    this->m_shots  = g_cl.m_local->m_iShotsFired( );
    this->m_clip   = gun->m_iClip1( );
    this->m_armed  = true;
}

void no_spread::post_predict( )
{
    if ( !this->m_armed )
    {
        return;
    }

    this->m_armed = false;

    C_TerrorGun       *gun{ this->m_gun };
    CTerrorWeaponInfo *info{ gun->GetWeaponInfo( ) };

    if ( !info )
    {
        this->m_shadow_valid = false;
        return;
    }

    C_TerrorPlayer *player{ g_cl.m_local };
    const i32       fov{ server_fov( player ) };
    const i32       fov_default{ default_fov( player ) };
    const i32       fov_ratio{ fov / fov_default };

    float spread{ this->m_spread };
    spread -= info->m_flSpreadDecay( ) * TICK_INTERVAL * ( spread - this->target_spread( info, gun, fov_ratio ) );
    spread = math::clamp( spread, 0.0f, SPREAD_CEILING );

    const i32  shots{ player->m_iShotsFired( ) - this->m_shots };
    const bool fired{ this->m_clip > 0 && ( shots > 0 || gun->m_iClip1( ) < this->m_clip ) };

    if ( fired )
    {
        aim_away_from_spread( spread );
        const float kick{ spread_per_shot( info, gun, fov, fov_default ) };

        for ( i32 shot{ }; shot < math::bigger( shots, 1 ); ++shot )
        {
            spread = math::smaller( spread + kick, info->m_flMaxSpread( ) );
        }
    }

    this->m_shadow_valid = fov_ratio == 0;
    if ( this->m_shadow_valid )
    {
        this->m_shadow        = spread;
        this->m_shadow_weapon = player->m_hActiveWeapon( ).ToInt( );
    }
}
