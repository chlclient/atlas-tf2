#pragma once

#include "link.hpp"

inline constexpr usize   ESP_FLAG_CAPACITY{ 5 };
inline constexpr i32     ESP_BLOCK_GAP{ 3 };
inline constexpr i32     ESP_BAR_GAP{ 4 };
inline constexpr i32     ESP_BAR_WIDTH{ 2 };
inline constexpr i32     ESP_NAME_HEIGHT{ 13 };
inline constexpr i32     ESP_SMALL_HEIGHT{ 9 };
inline constexpr color_t ESP_BAR_BACKGROUND{ 16, 16, 18, 220 };

struct esp_flag_t
{
    const char *m_label{ };
    color_t     m_color{ };
};

struct esp_target_t
{
    bool        m_elements[ config::ESP_ELEMENT_COUNT ]{ };
    color_t     m_box_color{ };
    color_t     m_name_color{ };
    const char *m_name{ };
    rect_t      m_bounds{ };
    i32         m_health{ };
    i32         m_max_health{ };
    esp_flag_t  m_flags[ ESP_FLAG_CAPACITY ]{ };
    usize       m_flag_count{ };

    bool shows( config::esp_element_t element ) const
    {
        return this->m_elements[ static_cast< usize >( element ) ];
    }

    void add_flag( const char *label, color_t color );
};

class esp
{
public:
    void paint( );

private:
    bool should_run( ) const;
    bool get_target( C_BaseEntity *entity, C_TerrorPlayer *local, esp_target_t &target ) const;
    bool get_player_target( C_TerrorPlayer *player, C_TerrorPlayer *local, esp_target_t &target ) const;
    bool get_infected_target( C_Infected *infected, usize kind, esp_target_t &target ) const;
    bool get_bounds( C_BaseEntity *entity, rect_t &bounds ) const;

    void draw_target( const esp_target_t &target ) const;
    void draw_box( const esp_target_t &target ) const;
    void draw_health_bar( const esp_target_t &target ) const;
    void draw_name( const esp_target_t &target ) const;
    void draw_flags( const esp_target_t &target ) const;
};

extern esp g_esp;
