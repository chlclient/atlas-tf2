#pragma once

#include "link.hpp"

struct datamap_copy_t
{
    u8   *m_data{ };
    usize m_size{ };

    bool ensure( usize needed );
    void release( );
};

class engine_prediction
{
public:
    bool install_proxies( );
    bool start( );
    void end( );
    void post_think( );
    void update( );

    bool m_is_active{ false };

private:
    bool save_data_maps( );
    void restore_data_maps( );

    bool  m_in_prediction{ };
    bool  m_first_time_predicted{ };
    vec3  m_local_view_angles{ };
    float m_last_fire_bullet_time{ };

    typedescription_t *m_viewmodel_sequence{ };

    globals_backup_t m_backup{ };
    datamap_copy_t   m_player_buffer{ };
    datamap_copy_t   m_weapon_buffer{ };
    datamap_copy_t   m_viewmodel_buffers[ MAX_VIEWMODELS ]{ };

    C_CSPlayer         *m_player{ };
    C_BaseCombatWeapon *m_weapon{ };
    C_BaseEntity       *m_viewmodels[ MAX_VIEWMODELS ]{ };
};

extern engine_prediction g_prediction;
