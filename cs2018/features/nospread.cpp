#include "nospread.hpp"

ATLAS_NO_DESTROY nospread g_nospread{ };

ATLAS_FORCE_INLINE float negev_density( float value, float recoil_index )
{
    for ( i32 step{ 3 }; step > recoil_index; --step )
    {
        value *= value;
    }

    return 1.0f - value;
}

ATLAS_FORCE_INLINE vec2 landed_offset( const fired_shot_t &shot, const vec3 &impact )
{
    vec3 forward{ };
    vec3 right{ };
    vec3 up{ };
    math::angle_vectors( shot.m_angles, &forward, &right, &up );

    const vec3  travel{ impact - shot.m_start };
    const float along{ travel.dot( forward ) };

    return vec2{ travel.dot( right ) / along, travel.dot( up ) / along };
}

ATLAS_FORCE_INLINE vec2 unit_direction( float angle )
{
    f64 sine{ };
    f64 cosine{ };
    math::sincos( static_cast< f64 >( angle ), sine, cosine );

    return vec2{ static_cast< float >( cosine ), static_cast< float >( sine ) };
}

void nospread::init( )
{
    CUniformRandomStream stream{ };
    stream.Construct( );

    for ( i32 patterns{ }; patterns < 2; ++patterns )
    {
        for ( i32 seed{ }; seed < SPREAD_SEEDS; ++seed )
        {
            spread_draw_t &draw{ this->m_draws[ patterns ][ seed ] };

            stream.SetSeed( seed + 1 );
            draw.m_inaccuracy_density   = stream.RandomFloat( 0.0f, 1.0f );
            draw.m_inaccuracy_direction = unit_direction( stream.RandomFloat( 0.0f, 2.0f * PI ) );

            if ( patterns )
            {
                draw.m_spread_direction = unit_direction( stream.RandomFloat( 0.0f, 2.0f * PI ) );
                draw.m_spread_density   = stream.RandomFloat( 0.0f, 1.0f );
                continue;
            }

            draw.m_spread_density   = stream.RandomFloat( 0.0f, 1.0f );
            draw.m_spread_direction = unit_direction( stream.RandomFloat( 0.0f, 2.0f * PI ) );
        }
    }
}

vec2 nospread::offset( i32 seed, const spread_t &spread ) const
{
    const spread_draw_t &draw{ this->m_draws[ spread.m_patterns ? 1 : 0 ][ seed ] };

    float inaccuracy_density{ draw.m_inaccuracy_density };
    float spread_density{ draw.m_spread_density };

    if ( spread.m_negev && spread.m_recoil_index < 3.0f )
    {
        inaccuracy_density = negev_density( inaccuracy_density, spread.m_recoil_index );
        spread_density     = negev_density( spread_density, spread.m_recoil_index );
    }

    return draw.m_inaccuracy_direction * ( inaccuracy_density * spread.m_inaccuracy ) +
           draw.m_spread_direction * ( spread_density * spread.m_spread );
}

bool nospread::compensate( const vec3 &direction, const vec2 &offset, vec3 &angles ) const
{
    const f64 x{ offset.x };
    const f64 y{ offset.y };
    const f64 rise{ math::sqrt( 1.0 + y * y ) };
    const f64 height{ static_cast< f64 >( direction.z ) / static_cast< f64 >( direction.length( ) ) *
                      math::sqrt( 1.0 + x * x + y * y ) / rise };

    if ( math::abs( height ) > 1.0 )
    {
        return false;
    }

    const f64 pitch{ math::atan2( y, 1.0 ) - math::asin( height ) };
    const f64 yaw{ math::atan2( static_cast< f64 >( direction.y ), static_cast< f64 >( direction.x ) ) +
                   math::atan2( x, rise * math::sqrt( 1.0 - height * height ) ) };

    angles = vec3{ RAD2DEG( pitch ), RAD2DEG( yaw ), 0.0f };
    return true;
}

void nospread::observe( const fired_shot_t &shot, const vec3 &impact )
{
    const vec2 landed{ landed_offset( shot, impact ) };

    float     distance{ };
    const i32 seed{ this->nearest_seed( landed, shot.m_spread, distance ) };

    DBGOUT( "nospread: impact %.6f off the aim in a %.6f cone, nearest seed %d is %.7f away (command seed %d)", landed.length( ),
            shot.m_spread.m_inaccuracy + shot.m_spread.m_spread, seed, distance, shot.m_seed & ( SPREAD_SEEDS - 1 ) );

    if ( seed != ( shot.m_seed & ( SPREAD_SEEDS - 1 ) ) )
    {
        this->m_matches = 0;
        return;
    }

    ++this->m_matches;
}

void nospread::clear( )
{
    this->m_matches = 0;
}

i32 nospread::nearest_seed( const vec2 &landed, const spread_t &spread, float &distance ) const
{
    i32   nearest{ };
    float closest{ FLT_MAX };

    for ( i32 seed{ }; seed < SPREAD_SEEDS; ++seed )
    {
        const float gap{ this->offset( seed, spread ).dist_to_sqr( landed ) };
        if ( gap >= closest )
        {
            continue;
        }

        nearest = seed;
        closest = gap;
    }

    distance = math::sqrt( closest );
    return nearest;
}
