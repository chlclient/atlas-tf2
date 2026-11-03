#include "hitboxes.hpp"

constexpr float BOX_TOLERANCE{ 0.01f };

ATLAS_FORCE_INLINE bool ray_hits_sphere( const vec3 &start, const vec3 &direction, const vec3 &centre, float radius, float &entry )
{
    const vec3  offset{ start - centre };
    const float along{ direction.dot( offset ) };
    const float discriminant{ along * along - ( offset.dot( offset ) - radius * radius ) };
    if ( discriminant < 0.0f )
    {
        return false;
    }

    entry = -along - math::sqrt( discriminant );
    return entry >= 0.0f;
}

ATLAS_FORCE_INLINE bool ray_hits_capsule( const vec3 &start, const vec3 &direction, const vec3 &a, const vec3 &b, float radius,
                                          float &entry )
{
    if ( start.dist_to( math::closest_on_edge( start, a, b ) ) <= radius )
    {
        entry = 0.0f;
        return true;
    }

    const vec3  axis{ b - a };
    const vec3  offset{ start - a };
    const float axis_axis{ axis.dot( axis ) };
    const float axis_ray{ axis.dot( direction ) };
    const float axis_offset{ axis.dot( offset ) };
    const float squeeze{ axis_axis - axis_ray * axis_ray };

    if ( squeeze <= 0.0f )
    {
        const vec3 cap{ axis_ray > 0.0f ? a : b };
        return ray_hits_sphere( start, direction, cap, radius, entry );
    }

    const float half{ axis_axis * direction.dot( offset ) - axis_offset * axis_ray };
    const float rest{ axis_axis * offset.dot( offset ) - axis_offset * axis_offset - radius * radius * axis_axis };
    const float discriminant{ half * half - squeeze * rest };
    if ( discriminant < 0.0f )
    {
        return false;
    }

    const float body{ ( -half - math::sqrt( discriminant ) ) / squeeze };
    const float height{ axis_offset + body * axis_ray };
    if ( height > 0.0f && height < axis_axis )
    {
        entry = body;
        return entry >= 0.0f;
    }

    return ray_hits_sphere( start, direction, height <= 0.0f ? a : b, radius, entry );
}

ATLAS_FORCE_INLINE void grow( vec3 &lowest, vec3 &highest, const vec3 &point, float pad )
{
    lowest  = vec3{ math::smaller( lowest.x, point.x - pad ), math::smaller( lowest.y, point.y - pad ), math::smaller( lowest.z, point.z - pad ) };
    highest = vec3{ math::bigger( highest.x, point.x + pad ), math::bigger( highest.y, point.y + pad ), math::bigger( highest.z, point.z + pad ) };
}

ATLAS_FORCE_INLINE bool box_entry( const pose_hitbox_t &box, const vec3 &start, const vec3 &direction, float &entry )
{
    if ( box.m_radius > 0.0f )
    {
        return ray_hits_capsule( start, direction, box.m_start, box.m_end, box.m_radius, entry );
    }

    return math::ray_hits_box( start, start + direction, box.m_transform, box.m_start, box.m_end, -BOX_TOLERANCE, &entry );
}

void pose_hitboxes_t::build( const mstudiohitboxset_t *set, const matrix3x4 *bones )
{
    this->m_count = math::smaller( set->numhitboxes, MAX_POSE_HITBOXES );

    vec3 lowest{ FLT_MAX, FLT_MAX, FLT_MAX };
    vec3 highest{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

    for ( i32 index{ }; index < this->m_count; ++index )
    {
        const mstudiobbox_t *box{ set->pHitbox( index ) };
        pose_hitbox_t       &out{ this->m_boxes[ index ] };

        matrix3x4 offset{ };
        math::angle_matrix( box->angOffsetOrientation, vec3{ }, offset );
        math::concat_transforms( bones[ box->bone ], offset, out.m_transform );

        out.m_radius = box->flCapsuleRadius;
        out.m_group  = box->group;

        if ( out.m_radius > 0.0f )
        {
            out.m_start = math::transform_point( box->bbmin, out.m_transform );
            out.m_end   = math::transform_point( box->bbmax, out.m_transform );

            grow( lowest, highest, out.m_start, out.m_radius );
            grow( lowest, highest, out.m_end, out.m_radius );
            continue;
        }

        out.m_start = box->bbmin;
        out.m_end   = box->bbmax;

        for ( i32 corner{ }; corner < 8; ++corner )
        {
            const vec3 local{ corner & 1 ? out.m_end.x : out.m_start.x, corner & 2 ? out.m_end.y : out.m_start.y,
                              corner & 4 ? out.m_end.z : out.m_start.z };
            grow( lowest, highest, math::transform_point( local, out.m_transform ), 0.0f );
        }
    }

    this->m_centre = ( lowest + highest ) * 0.5f;
    this->m_reach  = highest.dist_to( this->m_centre );
}

void pose_hitboxes_t::shift( const vec3 &offset )
{
    for ( i32 index{ }; index < this->m_count; ++index )
    {
        pose_hitbox_t &box{ this->m_boxes[ index ] };
        box.m_transform[ 0 ][ 3 ] += offset.x;
        box.m_transform[ 1 ][ 3 ] += offset.y;
        box.m_transform[ 2 ][ 3 ] += offset.z;

        if ( box.m_radius > 0.0f )
        {
            box.m_start += offset;
            box.m_end += offset;
        }
    }

    this->m_centre += offset;
}

bool pose_hitboxes_t::cross( const vec3 &start, const vec3 &direction, float length, pose_crossing_t &crossing ) const
{
    crossing = pose_crossing_t{ };

    for ( i32 index{ }; index < this->m_count; ++index )
    {
        const pose_hitbox_t &box{ this->m_boxes[ index ] };

        float entry{ };
        if ( !box_entry( box, start, direction, entry ) || entry > length )
        {
            continue;
        }

        crossing.m_groups |= 1u << box.m_group;
        crossing.m_entry = math::smaller( crossing.m_entry, entry );
    }

    return crossing.m_groups != 0;
}

bool pose_hitboxes_t::hits( const vec3 &start, const vec3 &direction, float length ) const
{
    const vec3  offset{ this->m_centre - start };
    const float along{ offset.dot( direction ) };
    if ( offset.dot( offset ) - along * along > this->m_reach * this->m_reach )
    {
        return false;
    }

    for ( i32 index{ }; index < this->m_count; ++index )
    {
        float entry{ };
        if ( box_entry( this->m_boxes[ index ], start, direction, entry ) && entry <= length )
        {
            return true;
        }
    }

    return false;
}

i32 pose_hitboxes_t::first_group( const vec3 &start, const vec3 &direction, float length ) const
{
    i32   group{ -1 };
    float nearest{ length };
    for ( i32 index{ }; index < this->m_count; ++index )
    {
        float entry{ };
        if ( box_entry( this->m_boxes[ index ], start, direction, entry ) && entry <= nearest )
        {
            nearest = entry;
            group   = this->m_boxes[ index ].m_group;
        }
    }

    return group;
}

float pose_hitboxes_t::distance( const vec3 &point ) const
{
    float nearest{ FLT_MAX };

    for ( i32 index{ }; index < this->m_count; ++index )
    {
        const pose_hitbox_t &box{ this->m_boxes[ index ] };

        if ( box.m_radius > 0.0f )
        {
            nearest = math::smaller( nearest, point.dist_to( math::closest_on_edge( point, box.m_start, box.m_end ) ) - box.m_radius );
            continue;
        }

        vec3 local{ };
        math::vector_itransform( point, box.m_transform, local );

        const vec3 clamped{ math::clamp( local.x, box.m_start.x, box.m_end.x ), math::clamp( local.y, box.m_start.y, box.m_end.y ),
                            math::clamp( local.z, box.m_start.z, box.m_end.z ) };

        nearest = math::smaller( nearest, local.dist_to( clamped ) );
    }

    return math::bigger( nearest, 0.0f );
}
