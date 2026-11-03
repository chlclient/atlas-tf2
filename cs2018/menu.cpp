#include "base.hpp"
#include "gui/gui.hpp"
#include "gui/particles.hpp"
#include "gui/theme.hpp"

#include "config.hpp"
#include "features/chams.hpp"
#include "features/skins.hpp"
#include "global.hpp"
#include "menu.hpp"

enum : usize
{
    TARGET_GROUP_PLAYERS = 0,
    TARGET_GROUP_COUNT   = 1,
};

template< usize target >
static bool editing_target( )
{
    return g_cfg.m_esp.m_edited == target;
}

static bool target_marked( usize group, usize item )
{
    if ( group != TARGET_GROUP_PLAYERS || item >= config::ESP_TARGET_COUNT )
    {
        return false;
    }

    for ( bool element : g_cfg.m_esp.m_profiles[ item ].m_elements )
    {
        if ( element )
        {
            return true;
        }
    }

    return false;
}

static void apply_accent( )
{
    g_theme.set_accent( g_cfg.m_misc.m_accent );
}

template< usize target >
static bool editing_blended_health( )
{
    return editing_target< target >( ) &&
           g_cfg.m_esp.m_profiles[ target ].m_health_style != config::HEALTH_STYLE_SOLID;
}

template< usize event >
static bool editing_hitmarker_event( )
{
    return g_cfg.m_hitmarker.m_enabled && g_cfg.m_hitmarker.m_edited_event == event;
}

template< usize event >
static bool editing_hitmarker_color( )
{
    return editing_hitmarker_event< event >( ) && g_cfg.m_hitmarker.m_override_color[ event ];
}

static const char *const *chams_modes( )
{
    static const char *const MODES[]{
        ATLAS_XORSTR_STATIC( "Off" ), ATLAS_XORSTR_STATIC( "Flat" ),
        ATLAS_XORSTR_STATIC( "Shaded" ), ATLAS_XORSTR_STATIC( "Fresnel" ),
        ATLAS_XORSTR_STATIC( "Wireframe" ), ATLAS_XORSTR_STATIC( "Lava" ),
        ATLAS_XORSTR_STATIC( "Pearl" ), ATLAS_XORSTR_STATIC( "Bubble" ),
        ATLAS_XORSTR_STATIC( "LSD" ), ATLAS_XORSTR_STATIC( "Network" ),
        ATLAS_XORSTR_STATIC( "Soap" ), ATLAS_XORSTR_STATIC( "Aurora" ),
        ATLAS_XORSTR_STATIC( "Aquarium" ), ATLAS_XORSTR_STATIC( "Sky" )
    };

    return MODES;
}

static void add_cham_layers( gui::group_box *group, config::cham_profile_t &profile,
                             bool ( *when )( ) )
{
    const char *const *modes{ chams_modes( ) };

    gui::combo_box *layers[]{
        group->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Visible" ), modes,
                                      chams::CHAM_COUNT, &profile.m_visible ),
        group->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Visible overlay" ), modes,
                                      chams::CHAM_COUNT, &profile.m_visible_overlay ),
        group->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Invisible" ), modes,
                                      chams::CHAM_COUNT, &profile.m_invisible ),
        group->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Invisible overlay" ), modes,
                                      chams::CHAM_COUNT, &profile.m_invisible_overlay )
    };

    layers[ 0 ]->set_tooltip( ATLAS_XORSTR_STATIC( "Replaces the model where you can see it, together with the "
                                                   "weapon it holds. Off keeps the game's own textures." ) );
    layers[ 1 ]->set_tooltip( ATLAS_XORSTR_STATIC( "Drawn on top of the visible model, whatever Visible is set to." ) );
    layers[ 2 ]->set_tooltip( ATLAS_XORSTR_STATIC( "Shows the part of the model a wall or prop hides." ) );
    layers[ 3 ]->set_tooltip( ATLAS_XORSTR_STATIC( "Drawn on top of the hidden part, over Invisible." ) );

    for ( gui::combo_box *layer : layers )
    {
        layer->set_keywords( ATLAS_XORSTR_STATIC( "chams model material visible invisible overlay xqz wall" ) )
            ->set_render_on( when );
    }
}

static void add_cham_tints( gui::group_box *group, config::cham_profile_t &profile,
                            bool ( *when )( ) )
{
    gui::color_picker *tints[]{
        group->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Visible tint" ),
                                         &profile.m_visible_color ),
        group->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Visible overlay tint" ),
                                         &profile.m_visible_overlay_color ),
        group->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Invisible tint" ),
                                         &profile.m_occluded_color ),
        group->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Invisible overlay tint" ),
                                         &profile.m_occluded_overlay_color )
    };

    for ( gui::color_picker *tint : tints )
    {
        tint->set_keywords( ATLAS_XORSTR_STATIC( "chams colour color material" ) )
            ->set_render_on( when );
    }
}

static void add_glow_layers( gui::group_box *group, config::glow_profile_t &profile,
                             bool ( *when )( ) )
{
    gui::slider_int   *outline{ group->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Glow" ),
                                                               &profile.m_stencil, 0, 10 ) };
    gui::slider_int   *blur{ group->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Glow blur" ),
                                                            &profile.m_blur, 0, 10 ) };
    gui::color_picker *tint{ group->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Glow tint" ),
                                                              &profile.m_color ) };

    outline->set_tooltip( ATLAS_XORSTR_STATIC( "Width of the solid outline drawn around the model, through walls too. "
                                               "0 leaves only the blur." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "glow outline stencil halo" ) );
    blur->set_tooltip( ATLAS_XORSTR_STATIC( "Soft halo spread around the outline. Both at 0 turns the glow off." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "glow blur bloom halo" ) );
    tint->set_keywords( ATLAS_XORSTR_STATIC( "glow colour color tint" ) );

    outline->set_render_on( when );
    blur->set_render_on( when );
    tint->set_render_on( when );
}

struct rage_row_t
{
    usize m_group{ };
    usize m_definition{ };
};

static constexpr rage_row_t RAGE_ROWS[]{
    { config::RAGE_GROUP_PISTOL },
    { config::RAGE_GROUP_PISTOL, WEAPON_GLOCK },
    { config::RAGE_GROUP_PISTOL, WEAPON_HKP2000 },
    { config::RAGE_GROUP_PISTOL, WEAPON_USP_SILENCER },
    { config::RAGE_GROUP_PISTOL, WEAPON_P250 },
    { config::RAGE_GROUP_PISTOL, WEAPON_FIVESEVEN },
    { config::RAGE_GROUP_PISTOL, WEAPON_TEC9 },
    { config::RAGE_GROUP_PISTOL, WEAPON_ELITE },
    { config::RAGE_GROUP_PISTOL, WEAPON_CZ75A },
    { config::RAGE_GROUP_HEAVY_PISTOL },
    { config::RAGE_GROUP_HEAVY_PISTOL, WEAPON_DEAGLE },
    { config::RAGE_GROUP_HEAVY_PISTOL, WEAPON_REVOLVER },
    { config::RAGE_GROUP_SMG },
    { config::RAGE_GROUP_SMG, WEAPON_MAC10 },
    { config::RAGE_GROUP_SMG, WEAPON_MP9 },
    { config::RAGE_GROUP_SMG, WEAPON_MP7 },
    { config::RAGE_GROUP_SMG, WEAPON_UMP45 },
    { config::RAGE_GROUP_SMG, WEAPON_P90 },
    { config::RAGE_GROUP_SMG, WEAPON_BIZON },
    { config::RAGE_GROUP_RIFLE },
    { config::RAGE_GROUP_RIFLE, WEAPON_AK47 },
    { config::RAGE_GROUP_RIFLE, WEAPON_M4A1 },
    { config::RAGE_GROUP_RIFLE, WEAPON_M4A1_SILENCER },
    { config::RAGE_GROUP_RIFLE, WEAPON_GALILAR },
    { config::RAGE_GROUP_RIFLE, WEAPON_FAMAS },
    { config::RAGE_GROUP_RIFLE, WEAPON_AUG },
    { config::RAGE_GROUP_RIFLE, WEAPON_SG556 },
    { config::RAGE_GROUP_MACHINE_GUN },
    { config::RAGE_GROUP_MACHINE_GUN, WEAPON_M249 },
    { config::RAGE_GROUP_MACHINE_GUN, WEAPON_NEGEV },
    { config::RAGE_GROUP_AWP },
    { config::RAGE_GROUP_SCOUT },
    { config::RAGE_GROUP_AUTO },
    { config::RAGE_GROUP_AUTO, WEAPON_G3SG1 },
    { config::RAGE_GROUP_AUTO, WEAPON_SCAR20 },
    { config::RAGE_GROUP_TASER },
};

static constexpr usize RAGE_ROW_COUNT{ sizeof( RAGE_ROWS ) / sizeof( *RAGE_ROWS ) };

enum : usize
{
    RAGE_TAB_PISTOLS,
    RAGE_TAB_SMGS,
    RAGE_TAB_RIFLES,
    RAGE_TAB_SNIPERS,
    RAGE_TAB_ZEUS,
    RAGE_TAB_COUNT,
};

static constexpr usize rage_tab( usize group )
{
    switch ( group )
    {
        case config::RAGE_GROUP_PISTOL:
        case config::RAGE_GROUP_HEAVY_PISTOL: return RAGE_TAB_PISTOLS;
        case config::RAGE_GROUP_SMG: return RAGE_TAB_SMGS;
        case config::RAGE_GROUP_RIFLE:
        case config::RAGE_GROUP_MACHINE_GUN: return RAGE_TAB_RIFLES;
        case config::RAGE_GROUP_AWP:
        case config::RAGE_GROUP_SCOUT:
        case config::RAGE_GROUP_AUTO: return RAGE_TAB_SNIPERS;
        default: return RAGE_TAB_ZEUS;
    }
}

static constexpr usize rage_tab_row( usize tab )
{
    usize row{ };

    while ( rage_tab( RAGE_ROWS[ row ].m_group ) != tab )
    {
        ++row;
    }

    return row;
}

static config::ragebot_profile_t &rage_row_profile( const rage_row_t &row )
{
    if ( !row.m_definition )
    {
        return g_cfg.m_ragebot.m_groups[ row.m_group ];
    }

    return g_cfg.m_ragebot.m_overrides[ row.m_definition ].m_profile;
}

template< usize row >
static bool editing_rage_row( )
{
    constexpr usize tab{ rage_tab( RAGE_ROWS[ row ].m_group ) };
    constexpr usize item{ row - rage_tab_row( tab ) };

    return g_cfg.m_ragebot.m_edited_tab == tab && g_cfg.m_ragebot.m_edited_item == item;
}

template< usize row >
static bool rage_profile_shown( )
{
    constexpr usize definition{ RAGE_ROWS[ row ].m_definition };

    return editing_rage_row< row >( ) && ( !definition || g_cfg.m_ragebot.m_overrides[ definition ].m_enabled );
}

template< usize row >
static bool rage_early_stop_shown( )
{
    return rage_profile_shown< row >( ) && rage_row_profile( RAGE_ROWS[ row ] ).m_autostop != config::AUTOSTOP_OFF;
}

template< typename type, type... rows >
struct rage_row_predicates
{
    static constexpr bool ( *const editing[] )( ){ &editing_rage_row< rows >... };
    static constexpr bool ( *const profile[] )( ){ &rage_profile_shown< rows >... };
    static constexpr bool ( *const early_stop[] )( ){ &rage_early_stop_shown< rows >... };
};

using rage_row_table = __make_integer_seq< rage_row_predicates, usize, RAGE_ROW_COUNT >;

static bool rage_row_marked( usize tab, usize item )
{
    const usize definition{ RAGE_ROWS[ rage_tab_row( tab ) + item ].m_definition };

    return definition && g_cfg.m_ragebot.m_overrides[ definition ].m_enabled;
}

static bool rage_row_can_scope( const rage_row_t &row )
{
    switch ( row.m_group )
    {
        case config::RAGE_GROUP_AWP:
        case config::RAGE_GROUP_SCOUT:
        case config::RAGE_GROUP_AUTO: return true;
        default: return row.m_definition == WEAPON_AUG || row.m_definition == WEAPON_SG556;
    }
}

static void add_rage_row( gui::group_box *group, usize row )
{
    static const char *const HITSCAN_BOXES[]{
        ATLAS_XORSTR_STATIC( "Head" ),
        ATLAS_XORSTR_STATIC( "Chest" ),
        ATLAS_XORSTR_STATIC( "Stomach" ),
        ATLAS_XORSTR_STATIC( "Arms" ),
        ATLAS_XORSTR_STATIC( "Legs" ),
    };

    static const char *const AUTOSTOP_MODES[]{
        ATLAS_XORSTR_STATIC( "Off" ),
        ATLAS_XORSTR_STATIC( "Accurate" ),
        ATLAS_XORSTR_STATIC( "Full" ),
    };

    static const char *const SAFE_POINT_MODES[]{
        ATLAS_XORSTR_STATIC( "Off" ),
        ATLAS_XORSTR_STATIC( "Prefer" ),
        ATLAS_XORSTR_STATIC( "Force" ),
    };

    config::ragebot_t         &rage{ g_cfg.m_ragebot };
    const rage_row_t          &entry{ RAGE_ROWS[ row ] };
    config::ragebot_profile_t &profile{ rage_row_profile( entry ) };
    bool ( *const shown )( ){ rage_row_table::profile[ row ] };

    if ( entry.m_definition )
    {
        group->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Override group" ), &rage.m_overrides[ entry.m_definition ].m_enabled )
            ->set_tooltip( ATLAS_XORSTR_STATIC( "Gives this weapon settings of its own instead of its group's. The weapons that do "
                                                "are marked in the list." ) )
            ->set_keywords( ATLAS_XORSTR_STATIC( "override weapon own settings" ) )
            ->set_render_on( &rage.m_enabled )
            ->set_render_on( rage_row_table::editing[ row ] );
    }

    gui::combo_box  *boxes{ group->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Hitboxes" ), HITSCAN_BOXES, config::HITSCAN_COUNT, profile.m_hitscan_boxes ) };
    gui::combo_box  *multipoint{ group->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Multipoint" ), HITSCAN_BOXES, config::HITSCAN_COUNT, profile.m_multipoint ) };
    gui::slider_int *head_scale{ group->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Head scale" ), &profile.m_head_scale, 0, 100, ATLAS_XORSTR_STATIC( "%" ) ) };
    gui::slider_int *body_scale{ group->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Body scale" ), &profile.m_body_scale, 0, 100, ATLAS_XORSTR_STATIC( "%" ) ) };
    gui::slider_int *min_damage{ group->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Minimum damage" ), &profile.m_min_damage, 1, 100, ATLAS_XORSTR_STATIC( "hp" ) ) };
    gui::slider_int *hitchance{ group->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Hitchance" ), &profile.m_hitchance, 0, 100, ATLAS_XORSTR_STATIC( "%" ) ) };
    gui::combo_box  *safe_point{ group->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Safe point" ), SAFE_POINT_MODES, config::SAFE_POINT_COUNT, &profile.m_safe_point ) };
    gui::combo_box  *autostop{ group->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Autostop" ), AUTOSTOP_MODES, config::AUTOSTOP_COUNT, &profile.m_autostop ) };
    gui::checkbox   *early_stop{ group->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Early autostop" ), &profile.m_autostop_early ) };

    boxes->set_tooltip( ATLAS_XORSTR_STATIC( "The hitgroups the ragebot scans, through walls too." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "hitboxes hitscan head chest stomach arms legs" ) )
        ->set_render_on( &rage.m_enabled )
        ->set_render_on( shown );
    multipoint->set_tooltip( ATLAS_XORSTR_STATIC( "Adds the edges of a hitbox that face you, and the crown of the head, when its "
                                                  "centre cannot be hit." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "multipoint multi point edge crown" ) )
        ->set_render_on( &rage.m_enabled )
        ->set_render_on( shown );
    head_scale->set_tooltip( ATLAS_XORSTR_STATIC( "How far out toward the edge of the head the multipoint points sit." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "head scale multipoint point scale" ) )
        ->set_render_on( &rage.m_enabled )
        ->set_render_on( shown );
    body_scale->set_tooltip( ATLAS_XORSTR_STATIC( "How far out toward the edge of every other hitbox the multipoint points sit." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "body scale multipoint point scale" ) )
        ->set_render_on( &rage.m_enabled )
        ->set_render_on( shown );
    min_damage->set_tooltip( ATLAS_XORSTR_STATIC( "The least damage a shot has to do. It drops to the target's health when that is "
                                                  "lower." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "mindmg min damage autowall" ) )
        ->set_render_on( &rage.m_enabled )
        ->set_render_on( shown );
    hitchance->set_tooltip( ATLAS_XORSTR_STATIC( "The share of the weapon's 256 possible spreads that has to hit the target." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "hitchance hit chance spread accuracy" ) )
        ->set_render_on( &rage.m_enabled )
        ->set_render_on( shown );
    safe_point->set_tooltip( ATLAS_XORSTR_STATIC( "Shoots where the hitbox is hit wherever the target's feet stand within the desync its "
                                                  "speed allows, and counts the hitchance on all of them. Prefer takes other points "
                                                  "when there is none, until a shot misses that target's pose; Force never does." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "safe point safepoint desync feet resolver" ) )
        ->set_render_on( &rage.m_enabled )
        ->set_render_on( shown );
    autostop->set_tooltip( ATLAS_XORSTR_STATIC( "Accurate slows you to 0.34 of the weapon's top speed, where moving adds no "
                                                "inaccuracy. Full stops you completely." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "autostop auto stop quick stop slow" ) )
        ->set_render_on( &rage.m_enabled )
        ->set_render_on( shown );
    early_stop->set_tooltip( ATLAS_XORSTR_STATIC( "Starts stopping before the weapon can fire again, so it is accurate on the first "
                                                  "tick it can." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "early autostop stop" ) )
        ->set_render_on( &rage.m_enabled )
        ->set_render_on( rage_row_table::early_stop[ row ] );

    if ( !rage_row_can_scope( entry ) )
    {
        return;
    }

    group->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Auto-scope" ), &profile.m_autoscope )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "Scopes in when a target is found but the shot is not accurate enough unscoped." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "autoscope auto scope zoom sniper" ) )
        ->set_render_on( &rage.m_enabled )
        ->set_render_on( shown );
}

static void build_ragebot( )
{
    static const char *const SHOT_LOGS[]{
        ATLAS_XORSTR_STATIC( "Console" ),
        ATLAS_XORSTR_STATIC( "Notifications" ),
    };

    static const char *const TAB_LABELS[ RAGE_TAB_COUNT ]{
        ATLAS_XORSTR_STATIC( "Pistols" ),
        ATLAS_XORSTR_STATIC( "SMGs" ),
        ATLAS_XORSTR_STATIC( "Rifles" ),
        ATLAS_XORSTR_STATIC( "Snipers" ),
        ATLAS_XORSTR_STATIC( "Zeus" ),
    };

    static const char *const ROW_LABELS[]{
        ATLAS_XORSTR_STATIC( "All pistols" ),
        ATLAS_XORSTR_STATIC( "Glock-18" ),
        ATLAS_XORSTR_STATIC( "P2000" ),
        ATLAS_XORSTR_STATIC( "USP-S" ),
        ATLAS_XORSTR_STATIC( "P250" ),
        ATLAS_XORSTR_STATIC( "Five-SeveN" ),
        ATLAS_XORSTR_STATIC( "Tec-9" ),
        ATLAS_XORSTR_STATIC( "Dual Berettas" ),
        ATLAS_XORSTR_STATIC( "CZ75-Auto" ),
        ATLAS_XORSTR_STATIC( "All heavy pistols" ),
        ATLAS_XORSTR_STATIC( "Desert Eagle" ),
        ATLAS_XORSTR_STATIC( "R8 Revolver" ),
        ATLAS_XORSTR_STATIC( "All SMGs" ),
        ATLAS_XORSTR_STATIC( "MAC-10" ),
        ATLAS_XORSTR_STATIC( "MP9" ),
        ATLAS_XORSTR_STATIC( "MP7" ),
        ATLAS_XORSTR_STATIC( "UMP-45" ),
        ATLAS_XORSTR_STATIC( "P90" ),
        ATLAS_XORSTR_STATIC( "PP-Bizon" ),
        ATLAS_XORSTR_STATIC( "All rifles" ),
        ATLAS_XORSTR_STATIC( "AK-47" ),
        ATLAS_XORSTR_STATIC( "M4A4" ),
        ATLAS_XORSTR_STATIC( "M4A1-S" ),
        ATLAS_XORSTR_STATIC( "Galil AR" ),
        ATLAS_XORSTR_STATIC( "FAMAS" ),
        ATLAS_XORSTR_STATIC( "AUG" ),
        ATLAS_XORSTR_STATIC( "SG 553" ),
        ATLAS_XORSTR_STATIC( "All machine guns" ),
        ATLAS_XORSTR_STATIC( "M249" ),
        ATLAS_XORSTR_STATIC( "Negev" ),
        ATLAS_XORSTR_STATIC( "AWP" ),
        ATLAS_XORSTR_STATIC( "SSG 08" ),
        ATLAS_XORSTR_STATIC( "All auto snipers" ),
        ATLAS_XORSTR_STATIC( "G3SG1" ),
        ATLAS_XORSTR_STATIC( "SCAR-20" ),
        ATLAS_XORSTR_STATIC( "Zeus x27" ),
    };

    static_assert( sizeof( ROW_LABELS ) / sizeof( *ROW_LABELS ) == RAGE_ROW_COUNT );

    static gui::target_picker::group_t tabs[ RAGE_TAB_COUNT ]{ };

    for ( usize row{ }; row < RAGE_ROW_COUNT; ++row )
    {
        const usize                  index{ rage_tab( RAGE_ROWS[ row ].m_group ) };
        gui::target_picker::group_t &tab{ tabs[ index ] };

        if ( !tab.m_item_count )
        {
            tab = { TAB_LABELS[ index ], &ROW_LABELS[ row ] };
        }

        ++tab.m_item_count;
    }

    gui::tab *aim{ g_gui.add_tab( ATLAS_XORSTR_STATIC( "Ragebot" ), ICON_CROSSHAIR ) };

    gui::sub_tab   *general{ aim->add_subtab( ATLAS_XORSTR_STATIC( "General" ) ) };
    gui::group_box *global{ general->add_group( ICON_CROSSHAIR, ATLAS_XORSTR_STATIC( "Global" ), gui::column_t::left ) };
    gui::group_box *weapon{ general->add_group( ICON_CROSSHAIR, ATLAS_XORSTR_STATIC( "Weapon" ), gui::column_t::right ) };

    config::ragebot_t &rage{ g_cfg.m_ragebot };

    gui::checkbox   *enabled{ global->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Enabled" ), &rage.m_enabled ) };
    gui::checkbox   *auto_shoot{ global->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Auto-shoot" ), &rage.m_auto_shoot ) };
    gui::slider_int *fov{ global->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "FOV" ), &rage.m_fov, 1, 180, ATLAS_XORSTR_STATIC( "deg" ) ) };
    gui::checkbox   *nospread{ global->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Nospread" ), &rage.m_nospread ) };
    gui::combo_box  *shot_log{ global->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Shot log" ), SHOT_LOGS, config::SHOT_LOG_COUNT, rage.m_shot_log ) };

    enabled->add_module< gui::keybind >( string_view{ }, &rage.m_activate_key );
    enabled->set_keywords( ATLAS_XORSTR_STATIC( "ragebot aimbot rage aim" ) );
    auto_shoot->set_tooltip( ATLAS_XORSTR_STATIC( "Fires on its own when a target passes the damage and hitchance checks. Off, the "
                                                  "ragebot only aims the shots you fire." ) )
        ->set_render_on( &rage.m_enabled );
    fov->set_tooltip( ATLAS_XORSTR_STATIC( "How far from your view, in degrees, a point may be. 180 scans everywhere." ) )
        ->set_render_on( &rage.m_enabled );
    nospread->set_tooltip( ATLAS_XORSTR_STATIC( "Turns each shot so its own spread lands on the target, once the bullet impacts show the "
                                                "server fires with the command's seed (sv_usercmd_custom_random_seed 0). Servers that "
                                                "draw their own seed, the default, keep the hitchance." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "nospread no spread seed compensate" ) )
        ->set_render_on( &rage.m_enabled );
    shot_log->set_tooltip( ATLAS_XORSTR_STATIC( "Where each ragebot shot is reported. The console gets the shot and its result, the "
                                                "notifications only the result." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "shot log logger hit miss console screen" ) )
        ->set_render_on( &rage.m_enabled );

    weapon->add< gui::target_picker >( tabs, RAGE_TAB_COUNT, &rage.m_edited_tab, &rage.m_edited_item )
        ->set_marks( &rage_row_marked )
        ->set_keywords( ATLAS_XORSTR_STATIC( "weapon group pistol deagle smg rifle awp scout auto zeus" ) )
        ->set_render_on( &rage.m_enabled );

    for ( usize row{ }; row < RAGE_ROW_COUNT; ++row )
    {
        add_rage_row( weapon, row );
    }
}

static void build_local_visuals( gui::tab *visuals )
{
    gui::sub_tab   *local{ visuals->add_subtab( ATLAS_XORSTR_STATIC( "Local" ) ) };
    gui::group_box *view{ local->add_group( ICON_EYE, ATLAS_XORSTR_STATIC( "Viewmodel" ), gui::column_t::left ) };

    gui::checkbox   *view_offset{ view->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Offset" ), &g_cfg.m_viewmodel.m_enabled ) };
    gui::slider_int *view_forward{ view->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Forward" ), &g_cfg.m_viewmodel.m_forward, -30, 30 ) };
    gui::slider_int *view_right{ view->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Right" ), &g_cfg.m_viewmodel.m_right, -30, 30 ) };
    gui::slider_int *view_up{ view->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Up" ), &g_cfg.m_viewmodel.m_up, -30, 30 ) };
    gui::checkbox   *view_fov{ view->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Field of view" ), &g_cfg.m_viewmodel.m_fov_override ) };
    gui::slider_int *view_fov_degrees{ view->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Degrees" ), &g_cfg.m_viewmodel.m_fov, 30, 120 ) };
    gui::slider_int *view_sway{ view->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Sway" ), &g_cfg.m_viewmodel.m_sway, 0, 200, ATLAS_XORSTR_STATIC( "%" ) ) };

    view_offset->set_tooltip( ATLAS_XORSTR_STATIC( "Moves the gun along your view, eased so a change slides instead of jumping." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "viewmodel offset position gun hands editor" ) );
    view_forward->set_render_on( &g_cfg.m_viewmodel.m_enabled );
    view_right->set_render_on( &g_cfg.m_viewmodel.m_enabled );
    view_up->set_render_on( &g_cfg.m_viewmodel.m_enabled );

    view_fov->set_tooltip( ATLAS_XORSTR_STATIC( "Field of view the gun is drawn with, the game's own is viewmodel_fov." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "viewmodel fov field of view gun" ) );
    view_fov_degrees->set_render_on( &g_cfg.m_viewmodel.m_fov_override );

    view_sway->set_tooltip( ATLAS_XORSTR_STATIC( "Lets the gun trail behind when you turn." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "viewmodel sway lag swing" ) );

    gui::group_box *tracer{ local->add_group( ICON_CROSSHAIR, ATLAS_XORSTR_STATIC( "Bullet tracer" ), gui::column_t::left ) };

    gui::checkbox     *tracer_enabled{ tracer->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Enabled" ), &g_cfg.m_bullet_tracer.m_enabled ) };
    gui::color_picker *tracer_color{ tracer->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Color" ), &g_cfg.m_bullet_tracer.m_color ) };
    gui::color_picker *tracer_hit_color{ tracer->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Hit color" ), &g_cfg.m_bullet_tracer.m_hit_color ) };
    gui::slider_float *tracer_duration{ tracer->add< gui::slider_float >( ATLAS_XORSTR_STATIC( "Duration" ), &g_cfg.m_bullet_tracer.m_duration, 0.5f, 10.0f, 1, ATLAS_XORSTR_STATIC( "s" ) ) };

    tracer_enabled->set_tooltip( ATLAS_XORSTR_STATIC( "Draws a beam from where you fired to where the server's bullet landed, once the server reports it." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "bullet tracer tracers beam impact" ) );
    tracer_color->set_render_on( &g_cfg.m_bullet_tracer.m_enabled );
    tracer_hit_color->set_tooltip( ATLAS_XORSTR_STATIC( "Used when the shot damaged a player." ) )
        ->set_render_on( &g_cfg.m_bullet_tracer.m_enabled );
    tracer_duration->set_render_on( &g_cfg.m_bullet_tracer.m_enabled );

    gui::group_box *hits{ local->add_group( ICON_ZAP, ATLAS_XORSTR_STATIC( "Hitmarker" ), gui::column_t::right ) };

    static const char *const HIT_EVENTS[]{
        ATLAS_XORSTR_STATIC( "Normal" ), ATLAS_XORSTR_STATIC( "Headshot" ),
        ATLAS_XORSTR_STATIC( "Kill" )
    };
    static bool ( *const HIT_EVENT_PREDICATES[ config::HITMARKER_EVENT_COUNT ] )( ){
        &editing_hitmarker_event< 0 >, &editing_hitmarker_event< 1 >,
        &editing_hitmarker_event< 2 >
    };
    static bool ( *const HIT_COLOR_PREDICATES[ config::HITMARKER_EVENT_COUNT ] )( ){
        &editing_hitmarker_color< 0 >, &editing_hitmarker_color< 1 >,
        &editing_hitmarker_color< 2 >
    };
    static const char *const HIT_RENDER_MODES[]{
        ATLAS_XORSTR_STATIC( "World" ), ATLAS_XORSTR_STATIC( "Impact" ),
        ATLAS_XORSTR_STATIC( "2D" )
    };

    gui::checkbox     *hit_enabled{ hits->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Enabled" ), &g_cfg.m_hitmarker.m_enabled ) };
    gui::checkbox     *hit_damage{ hits->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Show damage" ), &g_cfg.m_hitmarker.m_show_damage ) };
    gui::combo_box    *hit_mode{ hits->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Rendering mode" ), HIT_RENDER_MODES,
                                                              config::HITMARKER_MODE_COUNT, &g_cfg.m_hitmarker.m_render_mode ) };
    gui::color_picker *hit_color{ hits->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Base color" ), &g_cfg.m_hitmarker.m_color ) };
    gui::combo_box    *hit_event{ hits->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Event" ), HIT_EVENTS, config::HITMARKER_EVENT_COUNT,
                                                               &g_cfg.m_hitmarker.m_edited_event ) };

    hit_enabled->set_tooltip( ATLAS_XORSTR_STATIC( "Marks damage you deal to players." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "hitmarker hit marker damage indicator" ) );
    hit_mode->set_tooltip( ATLAS_XORSTR_STATIC( "World marks the player's center when the damage arrives. Impact marks the point "
                                                "the server's bullet hit. 2D marks the center of the screen." ) );
    hit_damage->set_render_on( &g_cfg.m_hitmarker.m_enabled );
    hit_mode->set_render_on( &g_cfg.m_hitmarker.m_enabled );
    hit_color->set_render_on( &g_cfg.m_hitmarker.m_enabled );
    hit_event->set_render_on( &g_cfg.m_hitmarker.m_enabled );

    for ( usize index{ }; index < config::HITMARKER_EVENT_COUNT; ++index )
    {
        gui::checkbox     *override_color{ hits->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Override color" ),
                                                                       &g_cfg.m_hitmarker.m_override_color[ index ] ) };
        gui::color_picker *event_color{ hits->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Event color" ),
                                                                        &g_cfg.m_hitmarker.m_event_color[ index ] ) };

        override_color->set_render_on( HIT_EVENT_PREDICATES[ index ] );
        event_color->set_render_on( HIT_COLOR_PREDICATES[ index ] );
    }

    hits->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Hit sound" ), &g_cfg.m_hitmarker.m_hitsound )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "Plays the game's arena switch click when you damage a player." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "hitsound hit sound click audio" ) );
}

static void build_visuals( )
{
    static bool ( *const TARGET_PREDICATES[ config::ESP_TARGET_COUNT ] )( ){
        &editing_target< 0 >,
        &editing_target< 1 >,
    };

    static bool ( *const BLENDED_HEALTH_PREDICATES[ config::ESP_TARGET_COUNT ] )( ){
        &editing_blended_health< 0 >,
        &editing_blended_health< 1 >,
    };

    gui::tab *visuals{ g_gui.add_tab( ATLAS_XORSTR_STATIC( "Visuals" ), ICON_EYE ) };

    gui::sub_tab *entities{ visuals->add_subtab( ATLAS_XORSTR_STATIC( "Entities" ) ) };

    gui::group_box *target{ entities->add_group( ICON_USER, ATLAS_XORSTR_STATIC( "Entities" ), gui::column_t::left ) };
    gui::group_box *elements{ entities->add_group( ICON_EYE, ATLAS_XORSTR_STATIC( "Elements" ), gui::column_t::right ) };

    static const char *const TARGET_PLAYERS[]{ ATLAS_XORSTR_STATIC( "Enemies" ),
                                               ATLAS_XORSTR_STATIC( "Teammates" ) };

    static const gui::target_picker::group_t TARGET_GROUPS[]{
        { ATLAS_XORSTR_STATIC( "Players" ), TARGET_PLAYERS, config::ESP_TARGET_COUNT },
    };

    target->add< gui::target_picker >( TARGET_GROUPS, TARGET_GROUP_COUNT, &g_cfg.m_esp.m_target_group,
                                       &g_cfg.m_esp.m_edited )
        ->set_marks( &target_marked )
        ->set_keywords( ATLAS_XORSTR_STATIC( "esp target enemy team" ) );

    target->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Dormant memory" ), &g_cfg.m_esp.m_dormant_time, 1, 15,
                                    ATLAS_XORSTR_STATIC( "s" ) )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "How long a player the server stopped sending keeps being "
                                            "drawn where it was last known." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "dormant memory pvs last known position fade" ) );

    static const char *const ELEMENT_LABELS[]{
        ATLAS_XORSTR_STATIC( "Box" ), ATLAS_XORSTR_STATIC( "Name" ),
        ATLAS_XORSTR_STATIC( "Distance" ), ATLAS_XORSTR_STATIC( "Health" ),
        ATLAS_XORSTR_STATIC( "Weapon" ), ATLAS_XORSTR_STATIC( "Flags" )
    };

    static const char *const FLAG_LABELS[]{
        ATLAS_XORSTR_STATIC( "Dormant" ), ATLAS_XORSTR_STATIC( "Armor" ),
        ATLAS_XORSTR_STATIC( "Money" ), ATLAS_XORSTR_STATIC( "Defuse kit" ),
        ATLAS_XORSTR_STATIC( "Scoped" ), ATLAS_XORSTR_STATIC( "Defusing" ),
        ATLAS_XORSTR_STATIC( "Flashed" ), ATLAS_XORSTR_STATIC( "Immune" )
    };

    static const char *const HEALTH_STYLES[]{
        ATLAS_XORSTR_STATIC( "Solid" ), ATLAS_XORSTR_STATIC( "Gradient" ),
        ATLAS_XORSTR_STATIC( "Health based" )
    };

    for ( usize index{ }; index < config::ESP_TARGET_COUNT; ++index )
    {
        config::esp_profile_t &profile{ g_cfg.m_esp.m_profiles[ index ] };
        bool ( *const when )( ){ TARGET_PREDICATES[ index ] };

        elements->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Shown" ), ELEMENT_LABELS,
                                         config::ESP_ELEMENT_COUNT, profile.m_elements )
            ->set_render_on( when )
            ->set_keywords( ATLAS_XORSTR_STATIC( "esp box name distance health weapon flags armor scoped defusing" ) );

        const bool *flags_shown{ &profile.m_elements[ static_cast< usize >( config::esp_element_t::flags ) ] };
        const bool *health_shown{ &profile.m_elements[ static_cast< usize >( config::esp_element_t::health ) ] };

        elements->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Flags" ), FLAG_LABELS, config::ESP_FLAG_COUNT,
                                         profile.m_flags )
            ->set_tooltip( ATLAS_XORSTR_STATIC( "Which states are listed beside the box." ) )
            ->set_keywords( ATLAS_XORSTR_STATIC( "flags dormant armor helmet kevlar money cash defuse kit scoped defusing flashed blind immune" ) )
            ->set_render_on( flags_shown )
            ->set_render_on( when );

        gui::color_picker *pickers[]{
            elements->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Box" ), &profile.m_box_color ),
            elements->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Name" ), &profile.m_name_color ),
            elements->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Distance" ), &profile.m_distance_color ),
            elements->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Health" ), &profile.m_health_color ),
            elements->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Weapon" ), &profile.m_weapon_color )
        };

        gui::combo_box    *health_style{ elements->add< gui::combo_box >(
            ATLAS_XORSTR_STATIC( "Health style" ), HEALTH_STYLES, config::HEALTH_STYLE_COUNT,
            &profile.m_health_style ) };
        gui::color_picker *health_low{ elements->add< gui::color_picker >(
            ATLAS_XORSTR_STATIC( "Low health" ), &profile.m_health_low_color ) };
        gui::slider_int   *health_divisions{ elements->add< gui::slider_int >(
            ATLAS_XORSTR_STATIC( "Health divisions" ), &profile.m_health_divisions, 0, 10 ) };

        health_style
            ->set_tooltip( ATLAS_XORSTR_STATIC(
                "Solid paints the bar in the health color. Gradient fades the bar from the low "
                "health color at its base to the health color at its tip. Health based paints the "
                "whole bar in a mix of the two that follows the player's health." ) )
            ->set_keywords( ATLAS_XORSTR_STATIC( "health bar color gradient fade hp based" ) )
            ->set_render_on( health_shown )
            ->set_render_on( when );

        health_low->set_render_on( health_shown )->set_render_on( BLENDED_HEALTH_PREDICATES[ index ] );

        health_divisions
            ->set_tooltip( ATLAS_XORSTR_STATIC(
                "Splits the health bar into equal sections. Set to 0 for a continuous bar." ) )
            ->set_keywords( ATLAS_XORSTR_STATIC( "health separator separators fractions divisions segments" ) )
            ->set_render_on( health_shown )
            ->set_render_on( when );

        for ( gui::color_picker *picker : pickers )
        {
            picker->set_render_on( when );
        }
    }

    gui::group_box *model{ entities->add_group( ICON_SPARKLES, ATLAS_XORSTR_STATIC( "Model" ), gui::column_t::right ) };

    for ( usize index{ }; index < config::ESP_TARGET_COUNT; ++index )
    {
        add_cham_layers( model, g_cfg.m_chams.m_profiles[ index ], TARGET_PREDICATES[ index ] );
        add_cham_tints( model, g_cfg.m_chams.m_profiles[ index ], TARGET_PREDICATES[ index ] );
        add_glow_layers( model, g_cfg.m_glow.m_players[ index ], TARGET_PREDICATES[ index ] );
    }

    build_local_visuals( visuals );
}

template< usize mode >
static bool fakelag_mode_shown( )
{
    return g_cfg.m_anti_hack.m_fakelag_enabled && g_cfg.m_anti_hack.m_fakelag_mode == mode;
}

template< usize state >
static bool editing_anti_aim_state( )
{
    return g_cfg.m_anti_hack.m_anti_aim_enabled && g_cfg.m_anti_hack.m_edited_state == state;
}

template< usize state, usize mode >
static bool anti_aim_real_shown( )
{
    return editing_anti_aim_state< state >( ) && g_cfg.m_anti_hack.m_anti_aim_profiles[ state ].m_real_yaw == mode;
}

template< usize state >
static bool anti_aim_fake_shown( )
{
    return editing_anti_aim_state< state >( ) && g_cfg.m_anti_hack.m_anti_aim_profiles[ state ].m_fake_yaw != config::FAKE_YAW_NONE;
}

template< usize state >
static bool anti_aim_fake_range_shown( )
{
    const usize mode{ g_cfg.m_anti_hack.m_anti_aim_profiles[ state ].m_fake_yaw };
    return editing_anti_aim_state< state >( ) && ( mode == config::FAKE_YAW_JITTER || mode == config::FAKE_YAW_RANDOM );
}

static bool lower_body_offset_shown( )
{
    return g_cfg.m_anti_hack.m_anti_aim_enabled && g_cfg.m_anti_hack.m_break_lower_body;
}

static void build_anti_aim( gui::sub_tab *general )
{
    static const char *const STATES[]{ ATLAS_XORSTR_STATIC( "Standing" ), ATLAS_XORSTR_STATIC( "Moving" ), ATLAS_XORSTR_STATIC( "In air" ) };
    static const char *const YAW_BASES[]{ ATLAS_XORSTR_STATIC( "View" ), ATLAS_XORSTR_STATIC( "At target" ) };
    static const char *const REAL_YAWS[]{ ATLAS_XORSTR_STATIC( "Static" ), ATLAS_XORSTR_STATIC( "Hide head" ), ATLAS_XORSTR_STATIC( "Jitter" ),
                                          ATLAS_XORSTR_STATIC( "Spin" ) };
    static const char *const FAKE_YAWS[]{ ATLAS_XORSTR_STATIC( "None" ), ATLAS_XORSTR_STATIC( "Opposite" ), ATLAS_XORSTR_STATIC( "Static" ),
                                          ATLAS_XORSTR_STATIC( "Jitter" ), ATLAS_XORSTR_STATIC( "Random" ) };
    static const char *const PITCHES[]{ ATLAS_XORSTR_STATIC( "View" ), ATLAS_XORSTR_STATIC( "Down" ), ATLAS_XORSTR_STATIC( "Up" ),
                                        ATLAS_XORSTR_STATIC( "Zero" ) };

    static bool ( *const STATE_SHOWN[ config::ANTI_AIM_STATE_COUNT ] )( ){
        &editing_anti_aim_state< 0 >,
        &editing_anti_aim_state< 1 >,
        &editing_anti_aim_state< 2 >,
    };
    static bool ( *const JITTER_SHOWN[ config::ANTI_AIM_STATE_COUNT ] )( ){
        &anti_aim_real_shown< 0, config::REAL_YAW_JITTER >,
        &anti_aim_real_shown< 1, config::REAL_YAW_JITTER >,
        &anti_aim_real_shown< 2, config::REAL_YAW_JITTER >,
    };
    static bool ( *const SPIN_SHOWN[ config::ANTI_AIM_STATE_COUNT ] )( ){
        &anti_aim_real_shown< 0, config::REAL_YAW_SPIN >,
        &anti_aim_real_shown< 1, config::REAL_YAW_SPIN >,
        &anti_aim_real_shown< 2, config::REAL_YAW_SPIN >,
    };
    static bool ( *const FAKE_SHOWN[ config::ANTI_AIM_STATE_COUNT ] )( ){
        &anti_aim_fake_shown< 0 >,
        &anti_aim_fake_shown< 1 >,
        &anti_aim_fake_shown< 2 >,
    };
    static bool ( *const FAKE_RANGE_SHOWN[ config::ANTI_AIM_STATE_COUNT ] )( ){
        &anti_aim_fake_range_shown< 0 >,
        &anti_aim_fake_range_shown< 1 >,
        &anti_aim_fake_range_shown< 2 >,
    };

    gui::group_box      *group{ general->add_group( ICON_USER, ATLAS_XORSTR_STATIC( "Anti-aim" ), gui::column_t::right ) };
    config::anti_hack_t &anti{ g_cfg.m_anti_hack };

    gui::checkbox *enabled{ group->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Enable anti-aim" ), &anti.m_anti_aim_enabled ) };
    enabled->add_module< gui::keybind >( string_view{ }, &anti.m_anti_aim_key );
    enabled->set_tooltip( ATLAS_XORSTR_STATIC( "The server animates only the first command of each packet: that one carries the real yaw, the "
                                               "rest the fake one other players see." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "anti aim antiaim aa desync fake real yaw" ) );

    group->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Break lower body" ), &anti.m_break_lower_body )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "Standing, turns the command the server realigns the lower body on away from the real yaw, so the "
                                            "lower body other players read never shows it." ) )
        ->set_render_on( &anti.m_anti_aim_enabled )
        ->set_keywords( ATLAS_XORSTR_STATIC( "lby lower body breaker" ) );
    group->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Lower body offset" ), &anti.m_lower_body_offset, -180, 180, ATLAS_XORSTR_STATIC( "deg" ) )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "Where the lower body lands, from the real yaw." ) )
        ->set_render_on( &lower_body_offset_shown );

    group->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Avoid resolver guesses" ), &anti.m_avoid_guesses )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "\"Hide head\" prefers, among the yaws that hide the head as well, one more than 35 deg from facing the "
                                            "enemy, backwards and either side, the yaws simple resolvers try." ) )
        ->set_render_on( &anti.m_anti_aim_enabled )
        ->set_keywords( ATLAS_XORSTR_STATIC( "bruteforce resolver avoid" ) );

    static const char *const LEG_MOVEMENTS[]{ ATLAS_XORSTR_STATIC( "Default" ), ATLAS_XORSTR_STATIC( "Slide" ), ATLAS_XORSTR_STATIC( "Never slide" ) };

    group->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Leg movement" ), LEG_MOVEMENTS, config::LEG_MOVEMENT_COUNT, &anti.m_leg_movement )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "\"Slide\" sends the movement keys opposite to where you move, so the server plays the strafe-change "
                                            "animation the whole time you run. \"Never slide\" sends none, so it never plays." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "leg movement slide walk moonwalk legs breaker strafe" ) );

    group->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Editing" ), STATES, config::ANTI_AIM_STATE_COUNT, &anti.m_edited_state )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "Each movement state has its own settings; moving is faster than 0.1 u/s on the ground, where the "
                                            "server writes the lower body every packet." ) )
        ->set_render_on( &anti.m_anti_aim_enabled );

    for ( usize state{ }; state < config::ANTI_AIM_STATE_COUNT; ++state )
    {
        config::anti_aim_profile_t &profile{ anti.m_anti_aim_profiles[ state ] };

        group->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Yaw base" ), YAW_BASES, config::YAW_BASE_COUNT, &profile.m_yaw_base )
            ->set_tooltip( ATLAS_XORSTR_STATIC( "What the offsets turn from: your view, or the direction to the enemy closest to your crosshair." ) )
            ->set_render_on( STATE_SHOWN[ state ] );
        group->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Real yaw" ), REAL_YAWS, config::REAL_YAW_COUNT, &profile.m_real_yaw )
            ->set_tooltip( ATLAS_XORSTR_STATIC( "\"Hide head\" turns to where the fewest enemies see your head, keeping the offset while none does." ) )
            ->set_render_on( STATE_SHOWN[ state ] );
        group->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Real offset" ), &profile.m_real_offset, -180, 180, ATLAS_XORSTR_STATIC( "deg" ) )
            ->set_render_on( STATE_SHOWN[ state ] );
        group->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Jitter range" ), &profile.m_jitter_range, 0, 180, ATLAS_XORSTR_STATIC( "deg" ) )
            ->set_render_on( JITTER_SHOWN[ state ] );
        group->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Spin speed" ), &profile.m_spin_speed, -90, 90, ATLAS_XORSTR_STATIC( "deg" ) )
            ->set_render_on( SPIN_SHOWN[ state ] );
        group->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Fake yaw" ), FAKE_YAWS, config::FAKE_YAW_COUNT, &profile.m_fake_yaw )
            ->set_tooltip( ATLAS_XORSTR_STATIC( "The yaw other players see. Anything but \"None\" holds at least one command back each packet." ) )
            ->set_render_on( STATE_SHOWN[ state ] );
        group->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Fake offset" ), &profile.m_fake_offset, -180, 180, ATLAS_XORSTR_STATIC( "deg" ) )
            ->set_render_on( FAKE_SHOWN[ state ] );
        group->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Fake range" ), &profile.m_fake_range, 0, 180, ATLAS_XORSTR_STATIC( "deg" ) )
            ->set_render_on( FAKE_RANGE_SHOWN[ state ] );
        group->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Pitch" ), PITCHES, config::PITCH_COUNT, &profile.m_pitch )
            ->set_render_on( STATE_SHOWN[ state ] );
    }
}

static void build_anti_hack( )
{
    static const char *const FAKELAG_MODES[]{ ATLAS_XORSTR_STATIC( "Static" ), ATLAS_XORSTR_STATIC( "Adaptive" ), ATLAS_XORSTR_STATIC( "Random" ) };

    gui::tab       *anti_hack{ g_gui.add_tab( ATLAS_XORSTR_STATIC( "Anti-Hack" ), ICON_USER ) };
    gui::sub_tab   *general{ anti_hack->add_subtab( ATLAS_XORSTR_STATIC( "General" ) ) };
    gui::group_box *fakelag{ general->add_group( ICON_ZAP, ATLAS_XORSTR_STATIC( "Fakelag" ), gui::column_t::left ) };

    config::anti_hack_t &anti{ g_cfg.m_anti_hack };

    gui::checkbox   *enabled{ fakelag->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Enable fakelag" ), &anti.m_fakelag_enabled ) };
    gui::combo_box  *mode{ fakelag->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Fakelag mode" ), FAKELAG_MODES, config::FAKELAG_MODE_COUNT, &anti.m_fakelag_mode ) };
    gui::slider_int *static_amount{ fakelag->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Static amount" ), &anti.m_static_amount, 3, MAX_NEW_COMMANDS ) };
    gui::slider_int *random_min{ fakelag->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Minimum choke" ), &anti.m_random_min, 3, MAX_NEW_COMMANDS ) };
    gui::slider_int *random_max{ fakelag->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Maximum choke" ), &anti.m_random_max, 3, MAX_NEW_COMMANDS ) };
    gui::checkbox   *randomize{ fakelag->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Randomize unchoke time" ), &anti.m_adaptive_randomized_choke ) };

    enabled->add_module< gui::keybind >( string_view{ }, &anti.m_fakelag_key );
    enabled->set_tooltip( ATLAS_XORSTR_STATIC( "Purposely chokes commands to break the game's lag compensation." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "fakelag fake lag choke" ) );
    mode->set_tooltip( ATLAS_XORSTR_STATIC( "Sets the choke mode for fakelag.\n\"Static\" chokes based on a static tick amount.\n\"Adaptive\" will choke the lowest needed to break LC.\n\"Random\" will choke randomly." ) )
        ->set_render_on( &anti.m_fakelag_enabled );
    static_amount->set_tooltip( ATLAS_XORSTR_STATIC( "How many ticks should the fakelag choke before unchoking?" ) )
        ->set_render_on( &fakelag_mode_shown< config::FAKELAG_MODE_STATIC > );
    random_min->set_tooltip( ATLAS_XORSTR_STATIC( "The minimum random choke amount for the fakelag.\nNOTE: If this is greater than the maximum, fakelag WILL NOT RUN!" ) )
        ->set_render_on( &fakelag_mode_shown< config::FAKELAG_MODE_RANDOM > );
    random_max->set_tooltip( ATLAS_XORSTR_STATIC( "The maximum random choke amount for the fakelag.\nNOTE: If this is less than the minimum, fakelag WILL NOT RUN!" ) )
        ->set_render_on( &fakelag_mode_shown< config::FAKELAG_MODE_RANDOM > );
    randomize->set_tooltip( ATLAS_XORSTR_STATIC( "Randomly sets an up to 4 tick window before unchoking from the base LC amount.\nThis can cause some cheats relying on static extrapolation to miss." ) )
        ->set_render_on( &fakelag_mode_shown< config::FAKELAG_MODE_ADAPTIVE > );

    build_anti_aim( general );
}

static bool strafing_with_wasd( )
{
    return g_cfg.m_misc.m_auto_strafe && g_cfg.m_misc.m_auto_strafe_mode == 1;
}

static void build_misc( )
{
    gui::tab *misc{ g_gui.add_tab( ATLAS_XORSTR_STATIC( "Misc" ), ICON_WRENCH ) };

    gui::sub_tab   *general{ misc->add_subtab( ATLAS_XORSTR_STATIC( "General" ) ) };
    gui::group_box *movement{ general->add_group( ICON_USER, ATLAS_XORSTR_STATIC( "Movement" ), gui::column_t::left ) };
    gui::group_box *camera{ general->add_group( ICON_CROSSHAIR, ATLAS_XORSTR_STATIC( "Camera" ), gui::column_t::right ) };

    static const char *const AUTO_STRAFE_MODES[]{ ATLAS_XORSTR_STATIC( "Viewangles" ),
                                                  ATLAS_XORSTR_STATIC( "WASD" ) };

    gui::checkbox     *bhop{ movement->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Bunny hop" ), &g_cfg.m_misc.m_bhop ) };
    gui::checkbox     *strafe{ movement->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Auto strafe" ), &g_cfg.m_misc.m_auto_strafe ) };
    gui::combo_box    *mode{ movement->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Strafe towards" ), AUTO_STRAFE_MODES, 2, &g_cfg.m_misc.m_auto_strafe_mode ) };
    gui::slider_float *retrack{ movement->add< gui::slider_float >( ATLAS_XORSTR_STATIC( "Retrack" ), &g_cfg.m_misc.m_auto_strafe_retrack, 10.0f, 100.0f, 0, ATLAS_XORSTR_STATIC( "%" ) ) };

    bhop->set_tooltip( ATLAS_XORSTR_STATIC( "Jumps again the tick you land, while jump is held." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "bhop auto jump" ) );
    strafe->set_tooltip( ATLAS_XORSTR_STATIC( "Steers the air strafe for you to keep speed." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "air strafe strafer" ) );
    mode->set_tooltip( ATLAS_XORSTR_STATIC( "Whether the strafe follows where you look or the movement keys you hold." ) )
        ->set_render_on( &g_cfg.m_misc.m_auto_strafe );
    retrack->set_render_on( &strafing_with_wasd );

    gui::checkbox   *slow_walk{ movement->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Slow walk" ), &g_cfg.m_misc.m_slow_walk ) };
    gui::slider_int *slow_speed{ movement->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Slow walk speed" ), &g_cfg.m_misc.m_slow_walk_speed, 5, 100, ATLAS_XORSTR_STATIC( "%" ) ) };

    slow_walk->add_module< gui::keybind >( string_view{ }, &g_cfg.m_misc.m_slow_walk_key );
    slow_walk->set_tooltip( ATLAS_XORSTR_STATIC( "Walks at a share of your weapon's top speed while the key is held." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "slow walk slowwalk walk speed" ) );
    slow_speed->set_tooltip( ATLAS_XORSTR_STATIC( "At 34% or less moving adds no inaccuracy to your shots." ) )
        ->set_render_on( &g_cfg.m_misc.m_slow_walk );

    gui::checkbox   *fov{ camera->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Field of view" ), &g_cfg.m_misc.m_fov_override ) };
    gui::slider_int *degrees{ camera->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Degrees" ), &g_cfg.m_misc.m_fov, 60, 140 ) };

    fov->set_tooltip( ATLAS_XORSTR_STATIC( "Overrides the game's field of view." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "fov zoom" ) );
    degrees->set_render_on( &g_cfg.m_misc.m_fov_override );

    gui::checkbox *thirdperson{ camera->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Thirdperson" ), &g_cfg.m_misc.m_thirdperson ) };
    thirdperson->add_module< gui::keybind >( string_view{ }, &g_cfg.m_misc.m_thirdperson_key );

    gui::slider_int *distance{ camera->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Distance" ), &g_cfg.m_misc.m_thirdperson_distance, 30, 300 ) };
    gui::slider_int *side{ camera->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Side offset" ), &g_cfg.m_misc.m_thirdperson_side, -100, 100 ) };
    gui::slider_int *height{ camera->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Height offset" ), &g_cfg.m_misc.m_thirdperson_height, -100, 100 ) };
    gui::checkbox   *collision{ camera->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Camera collision" ), &g_cfg.m_misc.m_thirdperson_collision ) };
    gui::checkbox   *smooth{ camera->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Smooth transition" ), &g_cfg.m_misc.m_thirdperson_smooth ) };
    gui::slider_int *smooth_time{ camera->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Transition time" ), &g_cfg.m_misc.m_thirdperson_smooth_time, 0, 1000, ATLAS_XORSTR_STATIC( "ms" ) ) };

    thirdperson->set_keywords( ATLAS_XORSTR_STATIC( "thirdperson third person camera" ) );
    distance->set_render_on( &g_cfg.m_misc.m_thirdperson );
    side->set_render_on( &g_cfg.m_misc.m_thirdperson );
    height->set_render_on( &g_cfg.m_misc.m_thirdperson );

    collision->set_render_on( &g_cfg.m_misc.m_thirdperson )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "Stops the camera at walls and props instead of letting it pass through them." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "thirdperson camera collision wall clip" ) );
    smooth->set_render_on( &g_cfg.m_misc.m_thirdperson )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "Pulls the camera out of the head over time instead of cutting straight to it." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "thirdperson smooth lerp transition third person" ) );
    smooth_time->set_render_on( &g_cfg.m_misc.m_thirdperson_smooth );

    camera->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Force crosshair" ), &g_cfg.m_misc.m_force_crosshair )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "Draws your crosshair with sniper rifles while unscoped." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "force crosshair sniper awp scout noscope" ) );

    static const char *const REMOVAL_LABELS[]{
        ATLAS_XORSTR_STATIC( "Visual recoil" ),
        ATLAS_XORSTR_STATIC( "Flash" ),
        ATLAS_XORSTR_STATIC( "Scope zoom" ),
        ATLAS_XORSTR_STATIC( "Scope overlay" ),
        ATLAS_XORSTR_STATIC( "Post processing" ),
        ATLAS_XORSTR_STATIC( "Landing bob" ),
    };

    camera->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Removals" ), REMOVAL_LABELS, config::REMOVAL_COUNT, g_cfg.m_misc.m_removals )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "Visual recoil keeps the view still while you spray, your shots still go where the recoil puts them. "
                                            "Flash drops the white screen, scope zoom keeps your field of view while scoped. "
                                            "Scope overlay drops the sniper scope's black lens and blur, leaving thin lines through the center. "
                                            "Landing bob stops the view and the gun dipping when you land." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "removals no recoil viewpunch aimpunch flash flashbang scope zoom overlay noscope lens blur post processing bloom landing bob dip" ) );
}

template< paint_list_t kind, usize index >
static bool skins_paint_shown( )
{
    return g_skins.paint_list_shown( kind, index );
}

template< paint_list_t kind >
struct skins_paint_predicates
{
    template< typename type, type... indices >
    struct table
    {
        static constexpr bool ( *const value[] )( ){ &skins_paint_shown< kind, indices >... };
    };
};

template< paint_list_t kind >
using skins_paint_table = __make_integer_seq< skins_paint_predicates< kind >::template table, usize,
                                              skins::PAINT_LISTS[ static_cast< usize >( kind ) ] >;

static bool skins_editing_weapon( )
{
    return g_skins.editing_weapon( );
}

static bool skins_editing_knife( )
{
    return g_skins.editing_knife( );
}

static bool skins_knife_stattrak( )
{
    return skins_editing_knife( ) && g_cfg.m_skins.m_knife_edit.m_stattrak;
}

static bool skins_weapon_stattrak( )
{
    return skins_editing_weapon( ) && g_cfg.m_skins.m_weapon_edit.m_stattrak;
}

static bool skins_gloves_picked( )
{
    return g_cfg.m_skins.m_enabled && g_cfg.m_skins.m_gloves.m_model > 0;
}

static const char *skins_held_text( )
{
    return g_skins.held_text( );
}

template< paint_list_t kind >
static void add_paint_lists( gui::group_box *group, usize *selected, const char *keywords )
{
    for ( usize index{ }; index < skins::PAINT_LISTS[ static_cast< usize >( kind ) ]; ++index )
    {
        gui::dynamic_list_box *list{ group->add< gui::dynamic_list_box >( selected ) };

        g_skins.bind_paint_list( kind, index, list );

        list->set_keywords( keywords )->set_render_on( skins_paint_table< kind >::value[ index ] );
    }
}

static void add_finish( gui::group_box *group, paint_list_t kind, config::skin_t &edit, bool ( *shown )( ),
                        bool ( *stattrak_shown )( ) )
{
    gui::slider_float *wear{ group->add< gui::slider_float >( ATLAS_XORSTR_STATIC( "Wear" ), &edit.m_wear, 0.0f, 1.0f, 3 ) };
    gui::slider_int   *seed{ group->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Seed" ), &edit.m_seed, 0, 1000 ) };
    gui::checkbox     *stattrak{ group->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "StatTrak" ), &edit.m_stattrak ) };
    gui::slider_int   *kills{ group->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Kills" ), &edit.m_stattrak_count, 0, 99999 ) };
    gui::text_input   *name{ group->add< gui::text_input >( ATLAS_XORSTR_STATIC( "Name tag" ) ) };

    g_skins.bind_name_input( kind, name );

    wear->set_tooltip( ATLAS_XORSTR_STATIC( "How worn the paint looks, from factory new at 0 to battle scarred at 1. "
                                            "0 keeps the paint's own wear." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "wear float factory new battle scarred" ) )
        ->set_render_on( shown );

    seed->set_tooltip( ATLAS_XORSTR_STATIC( "Picks where the paint's pattern lands." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "seed pattern index" ) )
        ->set_render_on( shown );

    stattrak->set_tooltip( ATLAS_XORSTR_STATIC( "Adds the StatTrak counter." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "stattrak kill counter strange" ) )
        ->set_render_on( shown );

    kills->set_tooltip( ATLAS_XORSTR_STATIC( "The number the counter shows." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "stattrak kills count counter" ) )
        ->set_render_on( stattrak_shown );

    name->set_limit( config::SKIN_NAME_LENGTH )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "A name tag, empty keeps the item's own name." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "name tag custom name rename" ) )
        ->set_render_on( shown );
}

static void build_skins_weapons( gui::tab *skins_tab )
{
    gui::sub_tab *weapons{ skins_tab->add_subtab( ATLAS_XORSTR_STATIC( "Weapons" ) ) };

    gui::group_box *weapon{ weapons->add_group( ICON_DOLLAR, ATLAS_XORSTR_STATIC( "Weapon" ), gui::column_t::left ) };
    gui::group_box *paint{ weapons->add_group( ICON_SPARKLES, ATLAS_XORSTR_STATIC( "Paint kit" ), gui::column_t::right ) };

    config::skins_t &cfg{ g_cfg.m_skins };

    gui::checkbox *enabled{ weapon->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Enabled" ), &cfg.m_enabled ) };
    gui::label    *held{ weapon->add< gui::label >( ATLAS_XORSTR_STATIC( "In hand" ), &skins_held_text ) };

    enabled->set_tooltip( ATLAS_XORSTR_STATIC( "Dresses your weapons, knife and gloves with the looks in this tab. "
                                               "Only you see them." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "skins skinchanger skin changer paint kit stattrak knife gloves" ) );

    held->set_render_on( &cfg.m_enabled );

    add_finish( weapon, paint_list_t::weapon, cfg.m_weapon_edit, &skins_editing_weapon, &skins_weapon_stattrak );
    add_paint_lists< paint_list_t::weapon >( paint, &cfg.m_weapon_edit.m_paintkit,
                                             ATLAS_XORSTR_STATIC( "paint kit skin finish pattern weapon gun" ) );
}

static void build_skins_knife( gui::tab *skins_tab )
{
    gui::sub_tab   *knife{ skins_tab->add_subtab( ATLAS_XORSTR_STATIC( "Knife" ) ) };
    gui::group_box *model{ knife->add_group( ICON_SPARKLES, ATLAS_XORSTR_STATIC( "Knife" ), gui::column_t::left ) };
    gui::group_box *paint{ knife->add_group( ICON_EYE, ATLAS_XORSTR_STATIC( "Paint kit" ), gui::column_t::right ) };

    config::skins_t &cfg{ g_cfg.m_skins };

    gui::dynamic_list_box *knives{ model->add< gui::dynamic_list_box >( &cfg.m_knife ) };

    g_skins.bind_knife_list( knives );

    knives->set_tooltip( ATLAS_XORSTR_STATIC( "Swaps your knife's model, animations, HUD and kill feed icon." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "knife model karambit butterfly bayonet m9 flip gut huntsman falchion bowie daggers" ) )
        ->set_render_on( &cfg.m_enabled );

    add_finish( model, paint_list_t::knife, cfg.m_knife_edit, &skins_editing_knife, &skins_knife_stattrak );
    add_paint_lists< paint_list_t::knife >( paint, &cfg.m_knife_edit.m_paintkit,
                                            ATLAS_XORSTR_STATIC( "knife paint kit skin finish fade doppler" ) );
}

static void build_skins_gloves( gui::tab *skins_tab )
{
    gui::sub_tab   *gloves{ skins_tab->add_subtab( ATLAS_XORSTR_STATIC( "Gloves" ) ) };
    gui::group_box *model{ gloves->add_group( ICON_USER, ATLAS_XORSTR_STATIC( "Gloves" ), gui::column_t::left ) };
    gui::group_box *paint{ gloves->add_group( ICON_SPARKLES, ATLAS_XORSTR_STATIC( "Paint kit" ), gui::column_t::right ) };

    config::glove_skin_t &cfg{ g_cfg.m_skins.m_gloves };

    gui::dynamic_list_box *models{ model->add< gui::dynamic_list_box >( &cfg.m_model ) };
    gui::slider_float     *wear{ model->add< gui::slider_float >( ATLAS_XORSTR_STATIC( "Wear" ), &cfg.m_wear, 0.0f, 1.0f, 3 ) };
    gui::slider_int       *seed{ model->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Seed" ), &cfg.m_seed, 0, 1000 ) };

    g_skins.bind_glove_list( models );

    models->set_tooltip( ATLAS_XORSTR_STATIC( "Gives your hands a pair of gloves. They stay through deaths "
                                              "and rounds." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "gloves hands arms sport driver moto specialist hand wraps bloodhound hydra" ) )
        ->set_render_on( &g_cfg.m_skins.m_enabled );

    wear->set_keywords( ATLAS_XORSTR_STATIC( "gloves wear float" ) )
        ->set_render_on( &skins_gloves_picked );

    seed->set_keywords( ATLAS_XORSTR_STATIC( "gloves seed pattern" ) )
        ->set_render_on( &skins_gloves_picked );

    add_paint_lists< paint_list_t::glove >( paint, &cfg.m_paintkit, ATLAS_XORSTR_STATIC( "gloves paint kit finish" ) );
}

static void build_skins( )
{
    gui::tab *skins_tab{ g_gui.add_tab( ATLAS_XORSTR_STATIC( "Skins" ), ICON_DOLLAR ) };

    build_skins_weapons( skins_tab );
    build_skins_knife( skins_tab );
    build_skins_gloves( skins_tab );
}

static void build_settings( )
{
    gui::tab *settings{ g_gui.add_tab( ATLAS_XORSTR_STATIC( "Settings" ), ICON_SETTINGS, true ) };

    gui::sub_tab *general{ settings->add_subtab( ATLAS_XORSTR_STATIC( "General" ) ) };

    gui::group_box *appearance{ general->add_group( ICON_SPARKLES,
                                                    ATLAS_XORSTR_STATIC( "Appearance" ),
                                                    gui::column_t::left ) };

    static const char *const PARTICLE_KINDS[]{ ATLAS_XORSTR_STATIC( "Off" ),
                                               ATLAS_XORSTR_STATIC( "Snow" ),
                                               ATLAS_XORSTR_STATIC( "Network" ),
                                               ATLAS_XORSTR_STATIC( "Halloween" ) };

    gui::color_picker *accent{ appearance->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Menu accent" ), &g_cfg.m_misc.m_accent, false ) };
    gui::color_picker *shadow{ appearance->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Shadow accent" ), &g_cfg.m_misc.m_shadow ) };
    gui::checkbox     *menu_blur{ appearance->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Background blur" ), &g_cfg.m_misc.m_menu_blur ) };
    gui::combo_box    *particles{ appearance->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Menu particles" ), PARTICLE_KINDS, gui::MENU_PARTICLES_COUNT, &g_cfg.m_misc.m_menu_particles ) };
    gui::checkbox     *menu_sounds{ appearance->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Menu sounds" ), &g_cfg.m_misc.m_menu_sounds ) };
    gui::slider_int   *sound_volume{ appearance->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Sound volume" ), &g_cfg.m_misc.m_menu_sound_volume, 0, 100, ATLAS_XORSTR_STATIC( "%" ) ) };
    appearance->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Draw watermark" ), &g_cfg.m_misc.m_draw_watermark );

    accent->set_on_change( &apply_accent )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "Repaints the whole menu around this colour." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "theme colour color accent skin" ) );

    shadow->set_tooltip( ATLAS_XORSTR_STATIC( "Colour and strength of the drop shadow the menu "
                                              "casts on the game." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "shadow drop glow colour color" ) );

    menu_blur->set_tooltip( ATLAS_XORSTR_STATIC( "Blurs the game itself while the menu is open. "
                                                 "The frosted panel under the menu is always on." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "blur background frost menu" ) );

    particles->set_tooltip( ATLAS_XORSTR_STATIC( "Drifts over the background while the menu is "
                                                 "open." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "particles snow network constellation halloween bats pumpkin spooky menu" ) );

    menu_sounds->set_tooltip( ATLAS_XORSTR_STATIC( "Plays a sound when the menu reacts to you and when a "
                                                   "notification comes in." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "sound audio click hover ui menu notification" ) );

    sound_volume->set_render_on( &g_cfg.m_misc.m_menu_sounds )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "Loudness of the menu and notification sounds." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "sound audio volume loudness ui menu notification" ) );

#if defined( ATLAS_DEV )
    gui::sub_tab   *debug{ settings->add_subtab( ATLAS_XORSTR_STATIC( "Debug" ) ) };
    gui::group_box *animation{ debug->add_group( ICON_USER, ATLAS_XORSTR_STATIC( "Animation" ), gui::column_t::left ) };

    gui::checkbox *server_hitboxes{ animation->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Server hitboxes" ), &g_cfg.m_debug.m_server_hitboxes ) };
    gui::checkbox *client_hitboxes{ animation->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Client hitboxes" ), &g_cfg.m_debug.m_client_hitboxes ) };

    server_hitboxes->add_module< gui::color_picker >( ATLAS_XORSTR_STATIC( "Server" ), &g_cfg.m_debug.m_server_hitbox_color );
    client_hitboxes->add_module< gui::color_picker >( ATLAS_XORSTR_STATIC( "Client" ), &g_cfg.m_debug.m_client_hitbox_color );

    server_hitboxes->set_tooltip( ATLAS_XORSTR_STATIC( "On a server you host, the hitboxes the server animated. They show its last tick, "
                                                       "so a moving player's boxes can sit a tick from ours." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "debug server hitbox hitboxes bones animation listen" ) );
    client_hitboxes->set_tooltip( ATLAS_XORSTR_STATIC( "The hitboxes of the skeleton the game set up for every player, drawn over "
                                                       "the server's: where the two match only this colour shows." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "debug client hitbox hitboxes bones animation" ) );

    gui::group_box *penetration{ debug->add_group( ICON_CROSSHAIR, ATLAS_XORSTR_STATIC( "Penetration" ), gui::column_t::left ) };

    penetration->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Autowall" ), &g_cfg.m_debug.m_autowall )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "Fires the autowall along your aim every tick and labels each wall and the final hit with "
                                            "the bullet's damage there, the numbers sv_showimpacts_penetration prints on a server "
                                            "you host. A player hit also shows the hitgroup and the health it takes." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "debug autowall penetration wallbang damage bullet" ) );

    gui::group_box *lag_compensation{ debug->add_group( ICON_LOADER, ATLAS_XORSTR_STATIC( "Lag compensation" ), gui::column_t::right ) };

    gui::checkbox *lag_records{ lag_compensation->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Records" ), &g_cfg.m_debug.m_lag_records ) };
    gui::checkbox *lag_server{ lag_compensation->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Server rewind" ), &g_cfg.m_debug.m_lag_server ) };

    lag_records->add_module< gui::color_picker >( ATLAS_XORSTR_STATIC( "Newest" ), &g_cfg.m_debug.m_lag_newest_color );
    lag_records->add_module< gui::color_picker >( ATLAS_XORSTR_STATIC( "Older" ), &g_cfg.m_debug.m_lag_record_color );

    lag_records->set_tooltip( ATLAS_XORSTR_STATIC( "The skeleton of every record the server would still rewind to for a shot sent "
                                                   "now: the tick it asks for lands exactly on the record, inside the server's 0.2 s "
                                                   "trust window and the second of history it keeps." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "debug lag compensation lagcomp backtrack records rewind" ) );
    lag_server->set_tooltip( ATLAS_XORSTR_STATIC( "On a server you host, rewinds the server through its own lag compensation to one of our "
                                                  "records per player each snapshot and draws its hitboxes over the record's, with the "
                                                  "distances in the console." ) )
        ->set_keywords( ATLAS_XORSTR_STATIC( "debug lag compensation lagcomp backtrack server rewind hitboxes listen" ) );
#endif // ATLAS_DEV
}

watermark_values_t watermark_values( )
{
    watermark_values_t values{ };

    values.m_watermark_position = &g_cfg.m_misc.m_watermark_position;

    values.m_user = g_api.user( );

    if ( !g_csgo.m_engine->IsInGame( ) )
    {
        return values;
    }

    INetChannelInfo *net_channel{ g_csgo.m_engine->GetNetChannelInfo( ) };

    if ( !net_channel )
    {
        return values;
    }

    values.m_latency   = static_cast< i32 >( net_channel->GetLatency( FLOW_OUTGOING ) * 1000.0f );
    values.m_connected = true;

    return values;
}

void sync_menu_theme( )
{
    g_theme.m_shadow          = g_cfg.m_misc.m_shadow;
    g_theme.m_blur_background = g_cfg.m_misc.m_menu_blur;
    g_theme.m_particles       = g_cfg.m_misc.m_menu_particles;
    g_theme.m_sounds          = g_cfg.m_misc.m_menu_sounds;
    g_theme.m_sound_volume    = static_cast< float >( g_cfg.m_misc.m_menu_sound_volume ) / 100.0f;
}

bool register_menu( )
{
#if defined( ATLAS_DEV )
    if ( !g_gui.init( ATLAS_XORSTR_STATIC( "skeet but good" ) ) )
    {
        return false;
    }
#else
    if ( !g_gui.init( ATLAS_XORSTR_STATIC( "atlas" ) ) )
    {
        return false;
    }
#endif // ATLAS_DEV

#if defined( ATLAS_DEV )
    g_gui.set_status( ATLAS_XORSTR_STATIC( "debug" ), g_api.user( ) );
#else
    g_gui.set_status( ATLAS_XORSTR_STATIC( "beta" ), g_api.user( ) );
#endif

    apply_accent( );
    sync_menu_theme( );

    build_ragebot( );
    build_visuals( );
    build_anti_hack( );
    build_misc( );
    build_skins( );
    build_settings( );

    return true;
}
