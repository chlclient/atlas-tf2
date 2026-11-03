#pragma once

#include "gui/particles.hpp"
#include "gui/watermark.hpp"

#include "util/windefs.hpp"

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
        distance,
        health,
        weapon,
        flags,
        count,
    };

    enum class esp_anchor_t : u8
    {
        top,
        bottom,
        left,
        right,
    };

    enum class esp_align_t : u8
    {
        start,
        center,
        end,
    };

    static constexpr usize ESP_TARGET_COUNT{ static_cast< usize >( esp_target_t::count ) };
    static constexpr usize ESP_ELEMENT_COUNT{ static_cast< usize >( esp_element_t::count ) };

    enum : usize
    {
        HITSCAN_HEAD,
        HITSCAN_CHEST,
        HITSCAN_STOMACH,
        HITSCAN_ARMS,
        HITSCAN_LEGS,
        HITSCAN_COUNT,
    };

    enum : usize
    {
        RAGE_GROUP_PISTOL,
        RAGE_GROUP_HEAVY_PISTOL,
        RAGE_GROUP_SMG,
        RAGE_GROUP_RIFLE,
        RAGE_GROUP_MACHINE_GUN,
        RAGE_GROUP_AWP,
        RAGE_GROUP_SCOUT,
        RAGE_GROUP_AUTO,
        RAGE_GROUP_TASER,
        RAGE_GROUP_COUNT,
    };

    enum : usize
    {
        AUTOSTOP_OFF,
        AUTOSTOP_ACCURATE,
        AUTOSTOP_FULL,
        AUTOSTOP_COUNT,
    };

    enum : usize
    {
        SAFE_POINT_OFF,
        SAFE_POINT_PREFER,
        SAFE_POINT_FORCE,
        SAFE_POINT_COUNT,
    };

    enum : usize
    {
        SHOT_LOG_CONSOLE,
        SHOT_LOG_NOTIFICATIONS,
        SHOT_LOG_COUNT,
    };

    static constexpr usize RAGE_WEAPON_COUNT{ 65 };

    struct ragebot_profile_t
    {
        bool  m_hitscan_boxes[ HITSCAN_COUNT ]{ true, true, true, false, false };
        bool  m_multipoint[ HITSCAN_COUNT ]{ true, true, true, false, false };
        i32   m_head_scale{ 70 };
        i32   m_body_scale{ 60 };
        i32   m_min_damage{ 20 };
        i32   m_hitchance{ 60 };
        usize m_safe_point{ SAFE_POINT_PREFER };
        usize m_autostop{ AUTOSTOP_ACCURATE };
        bool  m_autostop_early{ false };
        bool  m_autoscope{ false };
    };

    struct ragebot_override_t
    {
        bool              m_enabled{ false };
        ragebot_profile_t m_profile{ };
    };

    struct ragebot_t
    {
        bool   m_enabled{ false };
        bind_t m_activate_key{ 0, bind_mode_t::always };
        bool   m_auto_shoot{ true };
        i32    m_fov{ 180 };
        bool   m_nospread{ false };
        bool   m_shot_log[ SHOT_LOG_COUNT ]{ true, true };

        usize m_edited_tab{ };
        usize m_edited_item{ };

        ragebot_profile_t  m_groups[ RAGE_GROUP_COUNT ]{ { }, { }, { }, { }, { }, { .m_autoscope = true }, { .m_autoscope = true }, { .m_autoscope = true }, { } };
        ragebot_override_t m_overrides[ RAGE_WEAPON_COUNT ]{ };
    };

    enum : usize
    {
        FAKELAG_MODE_STATIC,
        FAKELAG_MODE_ADAPTIVE,
        FAKELAG_MODE_RANDOM,
        FAKELAG_MODE_COUNT,
    };

    enum : usize
    {
        ANTI_AIM_STATE_STAND,
        ANTI_AIM_STATE_MOVE,
        ANTI_AIM_STATE_AIR,
        ANTI_AIM_STATE_COUNT,
    };

    enum : usize
    {
        YAW_BASE_VIEW,
        YAW_BASE_AT_TARGET,
        YAW_BASE_COUNT,
    };

    enum : usize
    {
        REAL_YAW_STATIC,
        REAL_YAW_HIDE_HEAD,
        REAL_YAW_JITTER,
        REAL_YAW_SPIN,
        REAL_YAW_COUNT,
    };

    enum : usize
    {
        FAKE_YAW_NONE,
        FAKE_YAW_OPPOSITE,
        FAKE_YAW_STATIC,
        FAKE_YAW_JITTER,
        FAKE_YAW_RANDOM,
        FAKE_YAW_COUNT,
    };

    enum : usize
    {
        PITCH_VIEW,
        PITCH_DOWN,
        PITCH_UP,
        PITCH_ZERO,
        PITCH_COUNT,
    };

    struct anti_aim_profile_t
    {
        usize m_yaw_base{ YAW_BASE_AT_TARGET };
        usize m_real_yaw{ REAL_YAW_HIDE_HEAD };
        i32   m_real_offset{ 180 };
        i32   m_jitter_range{ 60 };
        i32   m_spin_speed{ 25 };
        usize m_fake_yaw{ FAKE_YAW_OPPOSITE };
        i32   m_fake_offset{ 0 };
        i32   m_fake_range{ 60 };
        usize m_pitch{ PITCH_DOWN };
    };

    enum : usize
    {
        LEG_MOVEMENT_DEFAULT,
        LEG_MOVEMENT_SLIDE,
        LEG_MOVEMENT_NEVER_SLIDE,
        LEG_MOVEMENT_COUNT,
    };

    struct anti_hack_t
    {
        bool   m_fakelag_enabled{ false };
        bind_t m_fakelag_key{ 0, bind_mode_t::always };

        usize m_fakelag_mode{ FAKELAG_MODE_ADAPTIVE };
        i32   m_static_amount{ 6 };
        i32   m_random_min{ 4 };
        i32   m_random_max{ 10 };
        bool  m_adaptive_randomized_choke{ false };

        bool               m_anti_aim_enabled{ false };
        bind_t             m_anti_aim_key{ 0, bind_mode_t::always };
        usize              m_edited_state{ };
        anti_aim_profile_t m_anti_aim_profiles[ ANTI_AIM_STATE_COUNT ]{ };
        bool               m_break_lower_body{ true };
        i32                m_lower_body_offset{ 110 };
        bool               m_avoid_guesses{ true };

        usize m_leg_movement{ LEG_MOVEMENT_DEFAULT };
    };

    enum : usize
    {
        HEALTH_STYLE_SOLID,
        HEALTH_STYLE_GRADIENT,
        HEALTH_STYLE_HEALTH,
        HEALTH_STYLE_COUNT,
    };

    enum : usize
    {
        ESP_FLAG_DORMANT,
        ESP_FLAG_ARMOR,
        ESP_FLAG_MONEY,
        ESP_FLAG_KIT,
        ESP_FLAG_SCOPED,
        ESP_FLAG_DEFUSING,
        ESP_FLAG_FLASHED,
        ESP_FLAG_IMMUNE,
        ESP_FLAG_COUNT,
    };

    struct esp_profile_t
    {
        bool m_elements[ ESP_ELEMENT_COUNT ]{ };
        bool m_flags[ ESP_FLAG_COUNT ]{ true, true, true, true, true, true, true, true };

        color_t m_box_color{ 255, 255, 255 };
        color_t m_name_color{ 255, 255, 255 };
        color_t m_distance_color{ 255, 255, 255 };
        usize   m_health_style{ HEALTH_STYLE_HEALTH };
        color_t m_health_color{ 0, 200, 32 };
        color_t m_health_low_color{ 200, 0, 32 };
        i32     m_health_divisions{ };
        color_t m_weapon_color{ 220, 220, 220 };
    };

    struct esp_t
    {
        esp_profile_t m_profiles[ ESP_TARGET_COUNT ]{
            { { true, true, true, true, true, true } },
            { },
        };

        usize m_target_group{ };
        usize m_edited{ };

        i32 m_dormant_time{ 3 };
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
            return !this->m_visible && !this->m_visible_overlay && !this->m_invisible && !this->m_invisible_overlay;
        }
    };

    struct chams_t
    {
        cham_profile_t m_profiles[ ESP_TARGET_COUNT ]{ };
    };

    struct glow_profile_t
    {
        i32     m_stencil{ };
        i32     m_blur{ };
        color_t m_color{ 220, 45, 40 };

        bool off( ) const
        {
            return this->m_stencil <= 0 && this->m_blur <= 0;
        }
    };

    struct glow_t
    {
        glow_profile_t m_players[ ESP_TARGET_COUNT ]{ { 0, 0, { 220, 45, 40 } }, { 0, 0, { 60, 140, 245 } } };
    };

    enum : usize
    {
        REMOVAL_VISUAL_RECOIL,
        REMOVAL_FLASH,
        REMOVAL_SCOPE_ZOOM,
        REMOVAL_SCOPE_OVERLAY,
        REMOVAL_POST_FX,
        REMOVAL_LANDING_BOB,
        REMOVAL_COUNT,
    };

    struct viewmodel_t
    {
        bool m_enabled{ false };
        i32  m_forward{ };
        i32  m_right{ };
        i32  m_up{ };

        bool m_fov_override{ false };
        i32  m_fov{ 54 };

        i32 m_sway{ 0 };
    };

    enum : usize
    {
        HITMARKER_NORMAL,
        HITMARKER_HEADSHOT,
        HITMARKER_KILL,
        HITMARKER_EVENT_COUNT,
    };

    enum : usize
    {
        HITMARKER_WORLD,
        HITMARKER_IMPACT,
        HITMARKER_2D,
        HITMARKER_MODE_COUNT,
    };

    struct hitmarker_t
    {
        bool    m_enabled{ };
        bool    m_show_damage{ true };
        usize   m_render_mode{ HITMARKER_IMPACT };
        color_t m_color{ 220, 235, 240 };

        usize   m_edited_event{ };
        bool    m_override_color[ HITMARKER_EVENT_COUNT ]{ false, true, true };
        color_t m_event_color[ HITMARKER_EVENT_COUNT ]{
            { 220, 235, 240 }, { 255, 205, 80 }, { 255, 95, 90 }
        };

        bool m_hitsound{ };
    };

    struct bullet_tracer_t
    {
        bool    m_enabled{ };
        color_t m_color{ 120, 170, 255 };
        color_t m_hit_color{ 255, 95, 90 };
        float   m_duration{ 2.0f };
    };

    struct misc_t
    {
        color_t m_accent{ 239, 75, 72 };
        color_t m_shadow{ 0, 0, 0, 72 };
        bool    m_menu_blur{ false };
        usize   m_menu_particles{ gui::MENU_PARTICLES_NETWORK };
        bool    m_menu_sounds{ true };
        i32     m_menu_sound_volume{ 100 };
        bool    m_draw_watermark{ true };
        i32     m_watermark_position{ ANCHOR_TOP_RIGHT };

        bool  m_bhop{ false };
        bool  m_auto_strafe{ false };
        usize m_auto_strafe_mode{ 1 };
        float m_auto_strafe_retrack{ 50.0f };

        bool   m_slow_walk{ false };
        bind_t m_slow_walk_key{ VK_SHIFT };
        i32    m_slow_walk_speed{ 34 };

        bool m_fov_override{ false };
        i32  m_fov{ 100 };

        bool   m_thirdperson{ false };
        bind_t m_thirdperson_key{ VK_F5 };
        i32    m_thirdperson_distance{ 120 };
        i32    m_thirdperson_side{ 0 };
        i32    m_thirdperson_height{ 0 };
        bool   m_thirdperson_collision{ true };
        bool   m_thirdperson_smooth{ true };
        i32    m_thirdperson_smooth_time{ 200 };

        bool m_force_crosshair{ false };

        bool m_removals[ REMOVAL_COUNT ]{ };
    };

    static constexpr usize SKIN_WEAPON_COUNT{ 64 };
    static constexpr usize SKIN_NAME_LENGTH{ 32 };

    struct skin_t
    {
        usize m_paintkit{ };
        float m_wear{ };
        i32   m_seed{ };
        bool  m_stattrak{ };
        i32   m_stattrak_count{ };
        char  m_name[ SKIN_NAME_LENGTH + 1 ]{ };

        bool operator==( const skin_t &other ) const = default;

        bool off( ) const
        {
            return *this == skin_t{ };
        }
    };

    struct skin_entry_t
    {
        i32    m_definition{ -1 };
        skin_t m_skin{ };
    };

    struct glove_skin_t
    {
        usize m_model{ };
        usize m_paintkit{ };
        float m_wear{ };
        i32   m_seed{ };

        bool operator==( const glove_skin_t &other ) const = default;
    };

    struct skins_t
    {
        bool m_enabled{ false };

        skin_t       m_weapon_edit{ };
        skin_entry_t m_weapons[ SKIN_WEAPON_COUNT ]{ };

        usize        m_knife{ };
        skin_t       m_knife_edit{ };
        glove_skin_t m_gloves{ };
    };

#if defined( ATLAS_DEV )
    struct debug_t
    {
        bool    m_server_hitboxes{ false };
        color_t m_server_hitbox_color{ 72, 160, 255, 255 };
        bool    m_client_hitboxes{ false };
        color_t m_client_hitbox_color{ 239, 75, 72, 255 };
        bool    m_lag_records{ false };
        color_t m_lag_newest_color{ 255, 122, 122, 255 };
        color_t m_lag_record_color{ 196, 199, 208, 120 };
        bool    m_lag_server{ false };
        bool    m_autowall{ false };
        bool    m_anti_aim{ false };
    };
#endif // ATLAS_DEV

    ragebot_t   m_ragebot{ };
    anti_hack_t m_anti_hack{ };
    esp_t       m_esp{ };
    chams_t     m_chams{ };
    glow_t      m_glow{ };
    viewmodel_t m_viewmodel{ };
    hitmarker_t     m_hitmarker{ };
    bullet_tracer_t m_bullet_tracer{ };
    misc_t          m_misc{ };
    skins_t     m_skins{ };
#if defined( ATLAS_DEV )
    debug_t m_debug{ };
#endif // ATLAS_DEV
};

extern config g_cfg;
