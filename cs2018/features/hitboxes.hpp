#pragma once

#include "link.hpp"

inline constexpr i32 MAX_POSE_HITBOXES{ 24 };

struct pose_hitbox_t
{
    matrix3x4 m_transform{ };
    vec3      m_start{ };
    vec3      m_end{ };
    float     m_radius{ };
    i32       m_group{ };
};

struct pose_crossing_t
{
    u32   m_groups{ };
    float m_entry{ FLT_MAX };
};

struct pose_hitboxes_t
{
    pose_hitbox_t m_boxes[ MAX_POSE_HITBOXES ]{ };
    vec3          m_centre{ };
    float         m_reach{ };
    i32           m_count{ };

    void  build( const mstudiohitboxset_t *set, const matrix3x4 *bones );
    void  shift( const vec3 &offset );
    bool  cross( const vec3 &start, const vec3 &direction, float length, pose_crossing_t &crossing ) const;
    bool  hits( const vec3 &start, const vec3 &direction, float length ) const;
    i32   first_group( const vec3 &start, const vec3 &direction, float length ) const;
    float distance( const vec3 &point ) const;
};
