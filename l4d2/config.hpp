#pragma once

#include "base.hpp"

class config
{
public:
    enum class esp_target_t : u8
    {
        enemy,
        team,
        count,
    };

    enum class esp_element_t : usize
    {
        box,
        name,
        health,
        flags,
        count,
    };

    static constexpr usize ESP_TARGET_COUNT{ static_cast< usize >( esp_target_t::count ) };
    static constexpr usize ESP_ELEMENT_COUNT{ static_cast< usize >( esp_element_t::count ) };

    enum : usize
    {
        INFECTED_COMMON,
        INFECTED_WITCH,
        INFECTED_COUNT,
    };

    enum : usize
    {
        INFECTED_ELEMENT_BOX,
        INFECTED_ELEMENT_NAME,
        INFECTED_ELEMENT_FLAGS,
        INFECTED_ELEMENT_COUNT,
    };

    enum : usize
    {
        TARGET_COMMON = ESP_TARGET_COUNT,
        TARGET_WITCH,
        TARGET_COUNT,
    };

    struct cham_profile_t
    {
        usize m_visible{ };
        usize m_visible_overlay{ };
        usize m_invisible{ };
        usize m_invisible_overlay{ };

        color_t m_visible_color{ 0x9C, 0xA8, 0xB0, 0x6F };
        color_t m_occluded_color{ 245, 190, 40 };
        color_t m_visible_overlay_color{ 25, 200, 255 };
        color_t m_occluded_overlay_color{ 255, 255, 255 };

        bool off( ) const
        {
            return !this->m_visible && !this->m_visible_overlay && !this->m_invisible &&
                   !this->m_invisible_overlay;
        }
    };

    struct esp_profile_t
    {
        bool           m_elements[ ESP_ELEMENT_COUNT ]{ };
        cham_profile_t m_chams{ };

        color_t m_box_color{ 255, 255, 255 };
        color_t m_name_color{ 255, 255, 255 };
    };

    struct esp_t
    {
        esp_profile_t m_profiles[ ESP_TARGET_COUNT ]{
            { { true, true, true, true }, { }, { 255, 80, 80 } },
            { { }, { }, { 90, 170, 255 } }
        };

        usize m_target_group{ };
        usize m_edited{ };

        bool m_infected_elements[ INFECTED_COUNT ][ INFECTED_ELEMENT_COUNT ]{
            { true, true, true },
            { true, true, true }
        };
    };

    struct chams_t
    {
        cham_profile_t m_infected[ INFECTED_COUNT ]{ };
    };

    struct glow_profile_t
    {
        i32     m_stencil{ };
        bool    m_blur{ };
        color_t m_color{ 220, 45, 40 };

        bool off( ) const
        {
            return this->m_stencil <= 0 && !this->m_blur;
        }
    };

    struct glow_t
    {
        glow_profile_t m_players[ ESP_TARGET_COUNT ]{ { 0, false, { 220, 45, 40 } },
                                                      { 0, false, { 60, 140, 245 } } };
        glow_profile_t m_infected[ INFECTED_COUNT ]{ { 0, false, { 200, 200, 200 } },
                                                     { 0, false, { 255, 120, 220 } } };
    };

    enum : usize
    {
        REMOVAL_VIEWPUNCH = 0,
        REMOVAL_PUSH,

        REMOVAL_COUNT,
    };

    struct viewmodel_t
    {
        bool m_enabled{ false };
        i32  m_forward{ };
        i32  m_right{ };
        i32  m_up{ };
    };

    struct misc_t
    {
        bool m_removals[ REMOVAL_COUNT ]{ };

        bool  m_bhop{ false };
        bool  m_auto_strafe{ false };
        usize m_auto_strafe_mode{ 1 }; // viewangles, wasd
        float m_auto_strafe_retrack{ 50.0f };

        bool m_fov_override{ false };
        i32  m_fov{ 100 };

        bool m_no_spread{ false };
    };

    enum : usize
    {
        HITSCAN_HEAD,
        HITSCAN_CHEST,
        HITSCAN_STOMACH,
        HITSCAN_ARMS,
        HITSCAN_LEGS,

        HITSCAN_GROUP_COUNT,
    };

    struct hitscan_t
    {
        bool    m_enabled{ false };
        bind_t  m_activate_key{ VK_XBUTTON1 };
        bool    m_psilent{ true };
        i32     m_fov{ 30 };
        bool    m_draw_fov{ false };
        color_t m_fov_color{ 255, 255, 255, 90 };

        bool m_players{ true };
        bool m_common{ true };
        bool m_witch{ false };

        usize m_priority{ HITSCAN_HEAD };
        bool  m_hitboxes[ HITSCAN_GROUP_COUNT ]{ true, true, true, false, false };
        bool  m_backtrack{ false };

        bool    m_draw_hitboxes{ false };
        color_t m_hitbox_color{ 255, 255, 255, 140 };
        color_t m_target_color{ 239, 75, 72, 220 };
        bool    m_draw_shot{ false };
        color_t m_shot_color{ 120, 200, 255, 200 };
        float   m_draw_duration{ 2.0f };
    };

    esp_t       m_esp{ };
    chams_t     m_chams{ };
    glow_t      m_glow{ };
    viewmodel_t m_viewmodel{ };
    hitscan_t   m_hitscan{ };
    misc_t      m_misc{ };
};

extern config g_cfg;
