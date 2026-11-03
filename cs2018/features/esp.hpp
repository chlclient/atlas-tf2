#pragma once

#include "link.hpp"
#include "runtime/vector.hpp"

struct esp_entity_t
{
    C_CSPlayer          *m_ent{ };
    i32                  m_index{ };
    float                m_distance{ };
    config::esp_target_t m_target{ };
    bool                 m_dormant{ };
    bool                 m_dead{ };
};

struct visual_record_t
{
    C_CSPlayer *m_player{ };
    bool        m_dormant{ };
    float       m_dormant_left{ };

    float m_alpha{ };
    float m_fill{ };
    float m_lag{ };
    float m_lag_hold{ };
    float m_ratio{ };
    float m_hp_pop{ };
    bool  m_seeded{ };
    float m_flag_pop[ config::ESP_FLAG_COUNT ]{ };
};

inline constexpr i32     ESP_BLOCK_GAP{ 3 };
inline constexpr i32     ESP_BAR_GAP{ 4 };
inline constexpr i32     ESP_BAR_WIDTH{ 2 };
inline constexpr i32     ESP_BAR_STEP{ 5 };
inline constexpr i32     ESP_NAME_HEIGHT{ 13 };
inline constexpr i32     ESP_SMALL_HEIGHT{ 9 };
inline constexpr color_t ESP_BAR_BACKGROUND{ 16, 16, 18, 220 };

struct esp_layout_t
{
    rect_t m_box{ };
    i32    m_bar[ 4 ]{ ESP_BLOCK_GAP, ESP_BLOCK_GAP, ESP_BAR_GAP, ESP_BAR_GAP };
    i32    m_stack[ 4 ][ 3 ]{ };

    rect_t  claim_bar( config::esp_anchor_t anchor );
    point_t claim_block( config::esp_anchor_t anchor, config::esp_align_t align, i32 height );

    static render::text_align align_for( config::esp_anchor_t anchor );
};

class esp
{
public:
    void think( );
    void paint( );

private:
    void draw_player( const esp_entity_t &entry, const rect_t &bounds, visual_record_t &visual, u32 delta );
    void draw_health_bar( esp_layout_t &layout, config::esp_anchor_t anchor, const config::esp_profile_t &profile,
                          visual_record_t &visual, i32 health, u32 delta, u8 opacity );
    void draw_flags( const esp_entity_t &entry, const config::esp_profile_t &profile, esp_layout_t &layout,
                     config::esp_anchor_t anchor, config::esp_align_t align, visual_record_t &visual, u32 delta,
                     u8 opacity );

    atl::vector< esp_entity_t > m_entities{ };
    visual_record_t             m_records[ MAX_PLAYERS + 1 ]{ };
    u64                         m_last_tick{ };
};

extern esp g_esp;
