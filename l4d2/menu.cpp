#include "menu.hpp"

#include "link.hpp"
#include "gui/particles.hpp"

#include "features/chams.hpp"

enum : usize
{
    TARGET_GROUP_PLAYERS  = 0,
    TARGET_GROUP_INFECTED = 1,
    TARGET_GROUP_COUNT    = 2,
};

template< usize target >
static bool editing_target( )
{
    return g_cfg.m_esp.m_target_group == TARGET_GROUP_PLAYERS &&
           g_cfg.m_esp.m_edited == target;
}

template< usize infected >
static bool editing_infected( )
{
    return g_cfg.m_esp.m_target_group == TARGET_GROUP_INFECTED &&
           g_cfg.m_esp.m_edited == infected;
}

static bool target_marked( usize group, usize item )
{
    if ( group == TARGET_GROUP_PLAYERS )
    {
        if ( item >= config::ESP_TARGET_COUNT )
        {
            return false;
        }

        const config::esp_profile_t &profile{ g_cfg.m_esp.m_profiles[ item ] };

        for ( bool element : profile.m_elements )
        {
            if ( element )
            {
                return true;
            }
        }

        return !profile.m_chams.off( ) || !g_cfg.m_glow.m_players[ item ].off( );
    }

    if ( item >= config::INFECTED_COUNT )
    {
        return false;
    }

    for ( bool element : g_cfg.m_esp.m_infected_elements[ item ] )
    {
        if ( element )
        {
            return true;
        }
    }

    return !g_cfg.m_chams.m_infected[ item ].off( ) || !g_cfg.m_glow.m_infected[ item ].off( );
}

static void apply_accent( )
{
    g_theme.set_accent( g_theme.m_accent );
}

static const char *const *chams_modes( )
{
    static const char *const MODES[]{
        ATLAS_XORSTR_STATIC( "Off" ), ATLAS_XORSTR_STATIC( "Flat" ),
        ATLAS_XORSTR_STATIC( "Wireframe" )
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

    for ( gui::combo_box *layer : layers )
    {
        layer->set_render_on( when );
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
        tint->set_render_on( when );
    }
}

static void add_glow_layers( gui::group_box *group, config::glow_profile_t &profile,
                             bool ( *when )( ) )
{
    gui::slider_int   *outline{ group->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Glow" ),
                                                               &profile.m_stencil, 0, 10 ) };
    gui::checkbox     *blur{ group->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Glow blur" ),
                                                          &profile.m_blur ) };
    gui::color_picker *tint{ group->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Glow tint" ),
                                                              &profile.m_color ) };

    outline->set_render_on( when );
    blur->set_render_on( when );
    tint->set_render_on( when );
}

static void build_visuals( )
{
    static bool ( *const TARGET_PREDICATES[ config::ESP_TARGET_COUNT ] )( ){
        &editing_target< 0 >,
        &editing_target< 1 >,
    };

    static bool ( *const INFECTED_PREDICATES[ config::INFECTED_COUNT ] )( ){
        &editing_infected< 0 >,
        &editing_infected< 1 >,
    };

    gui::tab *visuals{ g_gui.add_tab( ATLAS_XORSTR_STATIC( "Visuals" ), ICON_EYE ) };

    gui::sub_tab *entities{ visuals->add_subtab( ATLAS_XORSTR_STATIC( "Entities" ) ) };

    gui::group_box *target{ entities->add_group( ICON_USER, ATLAS_XORSTR_STATIC( "Entities" ), gui::column_t::left ) };
    gui::group_box *elements{ entities->add_group( ICON_EYE, ATLAS_XORSTR_STATIC( "Elements" ), gui::column_t::left ) };
    gui::group_box *model{ entities->add_group( ICON_SPARKLES, ATLAS_XORSTR_STATIC( "Model" ), gui::column_t::right ) };

    static const char *const TARGET_PLAYERS[]{ ATLAS_XORSTR_STATIC( "Enemies" ),
                                               ATLAS_XORSTR_STATIC( "Teammates" ) };
    static const char *const TARGET_INFECTED[]{ ATLAS_XORSTR_STATIC( "Common" ),
                                                ATLAS_XORSTR_STATIC( "Witch" ) };

    static const gui::target_picker::group_t TARGET_GROUPS[]{
        { ATLAS_XORSTR_STATIC( "Players" ), TARGET_PLAYERS, config::ESP_TARGET_COUNT },
        { ATLAS_XORSTR_STATIC( "Infected" ), TARGET_INFECTED, config::INFECTED_COUNT },
    };

    target->add< gui::target_picker >( TARGET_GROUPS, TARGET_GROUP_COUNT, &g_cfg.m_esp.m_target_group,
                                       &g_cfg.m_esp.m_edited )
        ->set_marks( &target_marked );

    static const char *const ELEMENT_LABELS[]{
        ATLAS_XORSTR_STATIC( "Box" ), ATLAS_XORSTR_STATIC( "Name" ),
        ATLAS_XORSTR_STATIC( "Health" ), ATLAS_XORSTR_STATIC( "Flags" )
    };

    for ( usize index{ }; index < config::ESP_TARGET_COUNT; ++index )
    {
        config::esp_profile_t &profile{ g_cfg.m_esp.m_profiles[ index ] };
        bool ( *const when )( ){ TARGET_PREDICATES[ index ] };

        elements->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Shown" ), ELEMENT_LABELS,
                                         config::ESP_ELEMENT_COUNT, profile.m_elements )
            ->set_render_on( when );

        gui::color_picker *pickers[]{
            elements->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Box" ), &profile.m_box_color ),
            elements->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Name" ), &profile.m_name_color )
        };

        for ( gui::color_picker *picker : pickers )
        {
            picker->set_render_on( when );
        }

        add_cham_layers( model, profile.m_chams, when );
        add_glow_layers( model, g_cfg.m_glow.m_players[ index ], when );
        add_cham_tints( model, profile.m_chams, when );
    }

    static const char *const INFECTED_LABELS[]{
        ATLAS_XORSTR_STATIC( "Box" ), ATLAS_XORSTR_STATIC( "Name" ),
        ATLAS_XORSTR_STATIC( "Flags" )
    };

    for ( usize index{ }; index < config::INFECTED_COUNT; ++index )
    {
        bool ( *const when )( ){ INFECTED_PREDICATES[ index ] };

        elements->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Shown" ), INFECTED_LABELS,
                                         config::INFECTED_ELEMENT_COUNT, g_cfg.m_esp.m_infected_elements[ index ] )
            ->set_render_on( when );

        add_cham_layers( model, g_cfg.m_chams.m_infected[ index ], when );
        add_glow_layers( model, g_cfg.m_glow.m_infected[ index ], when );
        add_cham_tints( model, g_cfg.m_chams.m_infected[ index ], when );
    }
}

static bool hitscan_duration_shown( )
{
    return g_cfg.m_hitscan.m_draw_hitboxes || g_cfg.m_hitscan.m_draw_shot;
}

static void build_aimbot( )
{
    gui::tab *aimbot{ g_gui.add_tab( ATLAS_XORSTR_STATIC( "Aimbot" ), ICON_CROSSHAIR ) };

    gui::sub_tab *general{ aimbot->add_subtab( ATLAS_XORSTR_STATIC( "Hitscan" ) ) };

    gui::group_box *global{ general->add_group( ICON_CROSSHAIR, ATLAS_XORSTR_STATIC( "Global" ), gui::column_t::left ) };

    gui::checkbox *enabled{ global->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Enabled" ), &g_cfg.m_hitscan.m_enabled ) };
    enabled->add_module< gui::keybind >( string_view{ }, &g_cfg.m_hitscan.m_activate_key );

    global->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Silent" ), &g_cfg.m_hitscan.m_psilent )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "Holds the shot one tick so it leaves together with the next command, and others never see the snap." ) );

    global->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Field of view" ), &g_cfg.m_hitscan.m_fov, 1, 180 );
    global->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Draw field of view" ), &g_cfg.m_hitscan.m_draw_fov )
        ->add_module< gui::color_picker >( ATLAS_XORSTR_STATIC( "Field of view" ), &g_cfg.m_hitscan.m_fov_color );

    static const char *const TARGET_LABELS[]{ ATLAS_XORSTR_STATIC( "Players" ),
                                              ATLAS_XORSTR_STATIC( "Common infected" ),
                                              ATLAS_XORSTR_STATIC( "Witch" ) };

    static bool *const TARGET_FLAGS[]{ &g_cfg.m_hitscan.m_players, &g_cfg.m_hitscan.m_common, &g_cfg.m_hitscan.m_witch };

    gui::group_box *targets{ general->add_group( ICON_PLAYERS, ATLAS_XORSTR_STATIC( "Targets" ), gui::column_t::right ) };
    for ( usize index{ }; index < 3; ++index )
    {
        targets->add< gui::checkbox >( TARGET_LABELS[ index ], TARGET_FLAGS[ index ] );
    }

    static const char *const HITBOX_LABELS[]{ ATLAS_XORSTR_STATIC( "Head" ),
                                              ATLAS_XORSTR_STATIC( "Chest" ),
                                              ATLAS_XORSTR_STATIC( "Stomach" ),
                                              ATLAS_XORSTR_STATIC( "Arms" ),
                                              ATLAS_XORSTR_STATIC( "Legs" ) };

    gui::group_box *hitboxes{ general->add_group( ICON_ZAP, ATLAS_XORSTR_STATIC( "Hitboxes" ), gui::column_t::left ) };

    hitboxes->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Priority" ), HITBOX_LABELS, config::HITSCAN_GROUP_COUNT, &g_cfg.m_hitscan.m_priority )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "Tried first every shot. The hitboxes below are the fallbacks when it is not visible." ) );
    hitboxes->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Fallbacks" ), HITBOX_LABELS, config::HITSCAN_GROUP_COUNT, g_cfg.m_hitscan.m_hitboxes );
    hitboxes->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Backtrack" ), &g_cfg.m_hitscan.m_backtrack )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "Also tries the target's older positions the server still rewinds to, and shoots from the one that is visible." ) );

    gui::group_box *visuals{ general->add_group( ICON_EYE, ATLAS_XORSTR_STATIC( "Visuals" ), gui::column_t::right ) };

    gui::checkbox *draw_hitboxes{ visuals->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Hitboxes" ), &g_cfg.m_hitscan.m_draw_hitboxes ) };

    draw_hitboxes->set_tooltip( ATLAS_XORSTR_STATIC( "Draws the hitboxes of the pose the last shot was placed against, the aimed one in its own color." ) )
        ->add_module< gui::color_picker >( ATLAS_XORSTR_STATIC( "Hitboxes" ), &g_cfg.m_hitscan.m_hitbox_color );
    draw_hitboxes->add_module< gui::color_picker >( ATLAS_XORSTR_STATIC( "Target" ), &g_cfg.m_hitscan.m_target_color );

    visuals->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Shoot position" ), &g_cfg.m_hitscan.m_draw_shot )
        ->set_tooltip( ATLAS_XORSTR_STATIC( "Draws the line from the shoot position to the point the last shot was sent at." ) )
        ->add_module< gui::color_picker >( ATLAS_XORSTR_STATIC( "Shot" ), &g_cfg.m_hitscan.m_shot_color );
    visuals->add< gui::slider_float >( ATLAS_XORSTR_STATIC( "Duration" ), &g_cfg.m_hitscan.m_draw_duration, 0.2f, 5.0f, 1, ATLAS_XORSTR_STATIC( "s" ) )
        ->set_render_on( &hitscan_duration_shown );
}

static bool strafing_with_wasd( )
{
    return g_cfg.m_misc.m_auto_strafe && g_cfg.m_misc.m_auto_strafe_mode == 1;
}

static void build_misc( )
{
    gui::tab *misc{ g_gui.add_tab( ATLAS_XORSTR_STATIC( "Misc" ), ICON_WRENCH ) };

    gui::sub_tab *general{ misc->add_subtab( ATLAS_XORSTR_STATIC( "General" ) ) };

    static const char *const AUTO_STRAFE_MODES[]{ ATLAS_XORSTR_STATIC( "Viewangles" ),
                                                  ATLAS_XORSTR_STATIC( "WASD" ) };

    gui::group_box *movement{ general->add_group( ICON_WRENCH, ATLAS_XORSTR_STATIC( "Movement" ), gui::column_t::left ) };
    movement->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Bunny hop" ), &g_cfg.m_misc.m_bhop );
    movement->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Auto strafe" ), &g_cfg.m_misc.m_auto_strafe );
    movement->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Strafe towards" ), AUTO_STRAFE_MODES, 2, &g_cfg.m_misc.m_auto_strafe_mode )
        ->set_render_on( &g_cfg.m_misc.m_auto_strafe );
    movement->add< gui::slider_float >( ATLAS_XORSTR_STATIC( "Retrack" ), &g_cfg.m_misc.m_auto_strafe_retrack, 10.0f, 100.0f, 0, ATLAS_XORSTR_STATIC( "%" ) )
        ->set_render_on( &strafing_with_wasd );

    gui::group_box *accuracy{ general->add_group( ICON_WRENCH, ATLAS_XORSTR_STATIC( "Accuracy" ), gui::column_t::left ) };
    accuracy->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "No spread" ), &g_cfg.m_misc.m_no_spread );

    static const char *const REMOVAL_LABELS[]{ ATLAS_XORSTR_STATIC( "Viewpunch" ),
                                               ATLAS_XORSTR_STATIC( "Push" ) };

    gui::group_box *camera{ general->add_group( ICON_EYE, ATLAS_XORSTR_STATIC( "Camera" ), gui::column_t::right ) };
    camera->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Field of view" ), &g_cfg.m_misc.m_fov_override );
    camera->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Degrees" ), &g_cfg.m_misc.m_fov, 60, 140 )
        ->set_render_on( &g_cfg.m_misc.m_fov_override );
    camera->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Removals" ), REMOVAL_LABELS, config::REMOVAL_COUNT, g_cfg.m_misc.m_removals );

    gui::group_box *view{ general->add_group( ICON_EYE, ATLAS_XORSTR_STATIC( "Viewmodel" ), gui::column_t::right ) };
    view->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Offset" ), &g_cfg.m_viewmodel.m_enabled );
    view->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Forward" ), &g_cfg.m_viewmodel.m_forward, -30, 30 )
        ->set_render_on( &g_cfg.m_viewmodel.m_enabled );
    view->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Right" ), &g_cfg.m_viewmodel.m_right, -30, 30 )
        ->set_render_on( &g_cfg.m_viewmodel.m_enabled );
    view->add< gui::slider_int >( ATLAS_XORSTR_STATIC( "Up" ), &g_cfg.m_viewmodel.m_up, -30, 30 )
        ->set_render_on( &g_cfg.m_viewmodel.m_enabled );
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
                                               ATLAS_XORSTR_STATIC( "Network" ) };

    appearance->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Menu accent" ), &g_theme.m_accent, false )->set_on_change( &apply_accent );
    appearance->add< gui::color_picker >( ATLAS_XORSTR_STATIC( "Shadow accent" ), &g_theme.m_shadow );
    appearance->add< gui::checkbox >( ATLAS_XORSTR_STATIC( "Background blur" ), &g_theme.m_blur_background );
    appearance->add< gui::combo_box >( ATLAS_XORSTR_STATIC( "Menu particles" ), PARTICLE_KINDS, gui::MENU_PARTICLES_COUNT, &g_theme.m_particles );
}

bool register_menu( )
{
#if defined( ATLAS_DEV )
    if ( !g_gui.init( ATLAS_XORSTR_STATIC( "tritium v2" ) ) )
    {
        return false;
    }

    g_gui.set_status( ATLAS_XORSTR_STATIC( "debug" ), g_api.user( ) );
#else
    if ( !g_gui.init( ATLAS_XORSTR_STATIC( "atlas" ) ) )
    {
        return false;
    }

    g_gui.set_status( ATLAS_XORSTR_STATIC( "beta" ), g_api.user( ) );
#endif // ATLAS_DEV

    build_aimbot( );
    build_visuals( );
    build_misc( );
    build_settings( );

    return true;
}
