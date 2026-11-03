#pragma once

#include "lagcomp.hpp"
#include "link.hpp"

struct lag_check_t
{
    matrix3x4 m_ours[ MAX_RECORD_BONES ]{ };

    alignas( 16 ) matrix3x4 m_server[ MAX_RECORD_BONES ]{ };

    const studiohdr_t *m_header{ };
    vec3               m_origin{ };
    i32                m_tick{ };
    i32                m_landed{ };
    bool               m_accepted{ };
    float              m_origin_error{ };
    float              m_hitbox_error{ };
};

class server_hitboxes
{
public:
    void paint( ) const;
    void check_lag_records( );

private:
    void draw_server( i32 index, const mstudiohitboxset_t *set ) const;
    void draw_client( C_CSPlayer *player, const mstudiohitboxset_t *set ) const;
    void draw_lag_check( i32 index, const studiohdr_t *studio, const mstudiohitboxset_t *set ) const;
    void pair_record( i32 index, const lag_record_t &entry, const LagRecordNode *head );
    void rewind( i32 index, i32 age, CGlobalVars *globals, CBaseAnimating *shooter, CBaseAnimating *target, const LagRecordNode *head );

    lag_check_t m_checks[ MAX_TRACKED_PLAYERS ]{ };
    i32         m_paired[ MAX_TRACKED_PLAYERS ]{ };
    i32         m_check_age{ };
};

extern server_hitboxes g_server_hitboxes;
