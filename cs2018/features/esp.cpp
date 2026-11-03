#include "esp.hpp"
#include "player_data.hpp"

ATLAS_NO_DESTROY esp g_esp{ };

inline constexpr i32 MAX_HEALTH{ 100 };

inline constexpr float UNITS_TO_METERS{ 0.0254f };

inline constexpr float ESP_ALPHA_RESPONSE{ 140.0f };
inline constexpr float DEATH_FADE_RESPONSE{ 250.0f };
inline constexpr float HEALTH_FILL_RESPONSE{ 90.0f };
inline constexpr float FLAG_IN_RESPONSE{ 110.0f };
inline constexpr float FLAG_STEP{ 9.0f };

inline constexpr config::esp_anchor_t ELEMENT_ANCHORS[ config::ESP_ELEMENT_COUNT ]{
    config::esp_anchor_t::top, config::esp_anchor_t::top, config::esp_anchor_t::bottom,
    config::esp_anchor_t::left, config::esp_anchor_t::bottom, config::esp_anchor_t::right
};

inline constexpr config::esp_align_t ELEMENT_ALIGNS[ config::ESP_ELEMENT_COUNT ]{
    config::esp_align_t::center, config::esp_align_t::center, config::esp_align_t::center,
    config::esp_align_t::center, config::esp_align_t::center, config::esp_align_t::start
};

rect_t esp_layout_t::claim_bar( config::esp_anchor_t anchor )
{
    const usize side{ static_cast< usize >( anchor ) };
    const i32   cursor{ this->m_bar[ side ] };

    this->m_bar[ side ] += ESP_BAR_STEP;

    switch ( anchor )
    {
        case config::esp_anchor_t::top:
            return rect_t{ this->m_box.left, this->m_box.top - cursor - ESP_BAR_WIDTH,
                           this->m_box.right, this->m_box.top - cursor };
        case config::esp_anchor_t::bottom:
            return rect_t{ this->m_box.left, this->m_box.bottom + cursor, this->m_box.right,
                           this->m_box.bottom + cursor + ESP_BAR_WIDTH };
        case config::esp_anchor_t::right:
            return rect_t{ this->m_box.right + cursor, this->m_box.top,
                           this->m_box.right + cursor + ESP_BAR_WIDTH, this->m_box.bottom };
        default:
            return rect_t{ this->m_box.left - cursor - ESP_BAR_WIDTH, this->m_box.top,
                           this->m_box.left - cursor, this->m_box.bottom };
    }
}

point_t esp_layout_t::claim_block( config::esp_anchor_t anchor, config::esp_align_t align,
                                   i32 height )
{
    const usize side{ static_cast< usize >( anchor ) };
    if ( anchor == config::esp_anchor_t::top || anchor == config::esp_anchor_t::bottom )
    {
        align = config::esp_align_t::center;
    }

    i32 &stack{ this->m_stack[ side ][ static_cast< usize >( align ) ] };

    if ( anchor == config::esp_anchor_t::top || anchor == config::esp_anchor_t::bottom )
    {
        if ( !stack )
        {
            stack = this->m_bar[ side ];
        }

        const i32 center_x{ this->m_box.left + ( this->m_box.right - this->m_box.left ) / 2 };
        const i32 y{ anchor == config::esp_anchor_t::top ? this->m_box.top - stack - height
                                                         : this->m_box.bottom + stack };

        stack += height + ESP_BLOCK_GAP;
        return point_t{ center_x, y };
    }

    const i32 span{ this->m_box.bottom - this->m_box.top - height };
    const i32 y{ this->m_box.top + ( align == config::esp_align_t::start ? 0 : align == config::esp_align_t::center ? span / 2
                                                                                                                    : span ) +
                 stack };

    stack += height + ESP_BLOCK_GAP;

    return point_t{ anchor == config::esp_anchor_t::left
                        ? this->m_box.left - this->m_bar[ static_cast< usize >( anchor ) ]
                        : this->m_box.right + this->m_bar[ static_cast< usize >( anchor ) ],
                    y };
}

render::text_align esp_layout_t::align_for( config::esp_anchor_t anchor )
{
    switch ( anchor )
    {
        case config::esp_anchor_t::left:
            return render::text_align::right;
        case config::esp_anchor_t::right:
            return render::text_align::left;
        default:
            return render::text_align::center;
    }
}

static const char *weapon_label( i16 definition_index )
{
    switch ( definition_index )
    {
        case WEAPON_DEAGLE: return ATLAS_XORSTR_STATIC( "DEAGLE" );
        case WEAPON_ELITE: return ATLAS_XORSTR_STATIC( "DUALIES" );
        case WEAPON_FIVESEVEN: return ATLAS_XORSTR_STATIC( "FIVE-SEVEN" );
        case WEAPON_GLOCK: return ATLAS_XORSTR_STATIC( "GLOCK" );
        case WEAPON_AK47: return ATLAS_XORSTR_STATIC( "AK-47" );
        case WEAPON_AUG: return ATLAS_XORSTR_STATIC( "AUG" );
        case WEAPON_AWP: return ATLAS_XORSTR_STATIC( "AWP" );
        case WEAPON_FAMAS: return ATLAS_XORSTR_STATIC( "FAMAS" );
        case WEAPON_G3SG1: return ATLAS_XORSTR_STATIC( "G3SG1" );
        case WEAPON_GALILAR: return ATLAS_XORSTR_STATIC( "GALIL" );
        case WEAPON_M249: return ATLAS_XORSTR_STATIC( "M249" );
        case WEAPON_M4A1: return ATLAS_XORSTR_STATIC( "M4A4" );
        case WEAPON_MAC10: return ATLAS_XORSTR_STATIC( "MAC-10" );
        case WEAPON_P90: return ATLAS_XORSTR_STATIC( "P90" );
        case WEAPON_UMP45: return ATLAS_XORSTR_STATIC( "UMP-45" );
        case WEAPON_XM1014: return ATLAS_XORSTR_STATIC( "XM1014" );
        case WEAPON_BIZON: return ATLAS_XORSTR_STATIC( "BIZON" );
        case WEAPON_MAG7: return ATLAS_XORSTR_STATIC( "MAG-7" );
        case WEAPON_NEGEV: return ATLAS_XORSTR_STATIC( "NEGEV" );
        case WEAPON_SAWEDOFF: return ATLAS_XORSTR_STATIC( "SAWED-OFF" );
        case WEAPON_TEC9: return ATLAS_XORSTR_STATIC( "TEC-9" );
        case WEAPON_TASER: return ATLAS_XORSTR_STATIC( "ZEUS" );
        case WEAPON_HKP2000: return ATLAS_XORSTR_STATIC( "P2000" );
        case WEAPON_MP7: return ATLAS_XORSTR_STATIC( "MP7" );
        case WEAPON_MP9: return ATLAS_XORSTR_STATIC( "MP9" );
        case WEAPON_NOVA: return ATLAS_XORSTR_STATIC( "NOVA" );
        case WEAPON_P250: return ATLAS_XORSTR_STATIC( "P250" );
        case WEAPON_SCAR20: return ATLAS_XORSTR_STATIC( "SCAR-20" );
        case WEAPON_SG556: return ATLAS_XORSTR_STATIC( "SG 553" );
        case WEAPON_SSG08: return ATLAS_XORSTR_STATIC( "SSG 08" );
        case WEAPON_FLASHBANG: return ATLAS_XORSTR_STATIC( "FLASHBANG" );
        case WEAPON_HEGRENADE: return ATLAS_XORSTR_STATIC( "HE GRENADE" );
        case WEAPON_SMOKEGRENADE: return ATLAS_XORSTR_STATIC( "SMOKE" );
        case WEAPON_MOLOTOV: return ATLAS_XORSTR_STATIC( "MOLOTOV" );
        case WEAPON_DECOY: return ATLAS_XORSTR_STATIC( "DECOY" );
        case WEAPON_INCGRENADE: return ATLAS_XORSTR_STATIC( "INCENDIARY" );
        case WEAPON_C4: return ATLAS_XORSTR_STATIC( "C4" );
        case WEAPON_HEALTHSHOT: return ATLAS_XORSTR_STATIC( "HEALTHSHOT" );
        case WEAPON_M4A1_SILENCER: return ATLAS_XORSTR_STATIC( "M4A1-S" );
        case WEAPON_USP_SILENCER: return ATLAS_XORSTR_STATIC( "USP-S" );
        case WEAPON_CZ75A: return ATLAS_XORSTR_STATIC( "CZ75-AUTO" );
        case WEAPON_REVOLVER: return ATLAS_XORSTR_STATIC( "R8 REVOLVER" );
        case WEAPON_TAGRENADE: return ATLAS_XORSTR_STATIC( "TA GRENADE" );
        case WEAPON_KNIFEGG:
        case WEAPON_KNIFE:
        case WEAPON_KNIFE_T:
        case WEAPON_BAYONET:
        case WEAPON_KNIFE_FLIP:
        case WEAPON_KNIFE_GUT:
        case WEAPON_KNIFE_KARAMBIT:
        case WEAPON_KNIFE_M9_BAYONET:
        case WEAPON_KNIFE_TACTICAL:
        case WEAPON_KNIFE_FALCHION:
        case WEAPON_KNIFE_SURVIVAL_BOWIE:
        case WEAPON_KNIFE_BUTTERFLY:
        case WEAPON_KNIFE_PUSH: return ATLAS_XORSTR_STATIC( "KNIFE" );
        default: return nullptr;
    }
}

static bool element_on( const config::esp_profile_t &profile, config::esp_element_t element )
{
    return profile.m_elements[ static_cast< usize >( element ) ];
}

static bool profile_active( const config::esp_profile_t &profile )
{
    for ( const bool enabled : profile.m_elements )
    {
        if ( enabled )
        {
            return true;
        }
    }

    return false;
}

static color_t modulate( color_t value, u8 opacity )
{
    value.a = static_cast< u8 >( ( static_cast< u32 >( value.a ) * opacity + 127u ) / 255u );
    return value;
}

static i32 scale_to_height( i32 height, float fraction )
{
    const float scaled{ static_cast< float >( height ) * math::clamp( fraction, 0.0f, 1.0f ) };
    return static_cast< i32 >( scaled + 0.5f );
}

static rect_t bar_fill( const rect_t &bar, bool vertical, i32 length, i32 from )
{
    return vertical ? rect_t{ bar.left, bar.bottom - length, bar.right, bar.bottom - from }
                    : rect_t{ bar.left + from, bar.top, bar.left + length, bar.bottom };
}

static void draw_bar_divisions( const rect_t &bar, bool vertical, i32 divisions, u8 opacity )
{
    if ( divisions < 2 )
    {
        return;
    }

    const i32     span{ vertical ? bar.bottom - bar.top : bar.right - bar.left };
    const color_t separator{ modulate( ESP_BAR_BACKGROUND, opacity ) };

    for ( i32 index{ 1 }; index < divisions; ++index )
    {
        const i32 offset{ ( span * index + divisions / 2 ) / divisions };

        if ( vertical )
        {
            const i32 y{ bar.top + offset };
            g_draw.box( rect_t{ bar.left, y, bar.right, y + 1 }, separator, true );
        } else
        {
            const i32 x{ bar.left + offset };
            g_draw.box( rect_t{ x, bar.top, x + 1, bar.bottom }, separator, true );
        }
    }
}

static u8 entry_alpha( const visual_record_t &visual )
{
    if ( !visual.m_dormant )
    {
        return 255;
    }

    const float ramp{ static_cast< float >( g_cfg.m_esp.m_dormant_time ) * 0.84f };
    if ( ramp <= 0.0f || visual.m_dormant_left >= ramp )
    {
        return 128;
    }

    return static_cast< u8 >( 128.0f * math::clamp( visual.m_dormant_left / ramp, 0.0f, 1.0f ) );
}

static bool calc_bbox( C_CSPlayer *player, const vec3 &origin, rect_t &bounds )
{
    const vec3 mins{ origin + player->m_vecMins( ) };
    const vec3 maxs{ origin + player->m_vecMaxs( ) };

    const vec3 corners[ 8 ]{
        { mins.x, mins.y, mins.z },
        { mins.x, mins.y, maxs.z },
        { mins.x, maxs.y, mins.z },
        { mins.x, maxs.y, maxs.z },
        { maxs.x, mins.y, mins.z },
        { maxs.x, mins.y, maxs.z },
        { maxs.x, maxs.y, mins.z },
        { maxs.x, maxs.y, maxs.z },
    };

    vec2 first{ };
    if ( !g_cl.world_to_screen( corners[ 0 ], first ) )
    {
        return false;
    }

    float min_x{ first.x };
    float max_x{ first.x };
    float min_y{ first.y };
    float max_y{ first.y };

    for ( usize index{ 1 }; index < 8; ++index )
    {
        vec2 screen{ };
        if ( !g_cl.world_to_screen( corners[ index ], screen ) )
        {
            return false;
        }

        min_x = math::smaller( min_x, screen.x );
        max_x = math::bigger( max_x, screen.x );
        min_y = math::smaller( min_y, screen.y );
        max_y = math::bigger( max_y, screen.y );
    }

    bounds.left   = static_cast< i32 >( min_x );
    bounds.top    = static_cast< i32 >( min_y );
    bounds.right  = static_cast< i32 >( max_x );
    bounds.bottom = static_cast< i32 >( max_y );
    return true;
}

void esp::think( )
{
    this->m_entities.clear( );

    if ( !g_csgo.m_engine->IsInGame( ) || !g_cl.m_local )
    {
        return;
    }

    const i32  max_clients{ math::smaller( g_csgo.m_engine->GetMaxClients( ), MAX_PLAYERS ) };
    const i32  local_index{ g_csgo.m_engine->GetLocalPlayer( ) };
    const i32  local_team{ g_cl.m_local->m_iTeamNum( ) };
    const vec3 local_origin{ g_cl.m_local->GetAbsOrigin( ) };

    for ( i32 index{ 1 }; index <= max_clients; ++index )
    {
        visual_record_t &visual{ this->m_records[ index ] };
        IClientEntity   *entity{ g_csgo.m_entity_list->GetClientEntity( index ) };

        if ( !entity || index == local_index )
        {
            visual = visual_record_t{ };
            continue;
        }

        auto *player{ entity->As< C_CSPlayer * >( ) };

        if ( visual.m_player != player )
        {
            visual          = visual_record_t{ };
            visual.m_player = player;
        }

        const bool dormant{ player->IsDormant( ) };
        const i32  team{ dormant ? g_player_data.team( index ) : player->m_iTeamNum( ) };
        const bool dead{ dormant ? !g_player_data.alive( index ) : !player->IsAlive( ) };
        vec3       origin{ };

        visual.m_dormant = dormant;

        if ( team < TEAM_TERRORIST ||
             ( dormant && ( dead || !g_player_data.dormant_origin( index, origin, visual.m_dormant_left ) ) ) ||
             ( dead && visual.m_alpha <= 0.0f ) )
        {
            continue;
        }

        const config::esp_target_t target{ team == local_team ? config::esp_target_t::team
                                                              : config::esp_target_t::enemy };

        if ( !profile_active( g_cfg.m_esp.m_profiles[ static_cast< usize >( target ) ] ) )
        {
            continue;
        }

        if ( !dormant )
        {
            origin = player->GetAbsOrigin( );
        }

        const float distance{ ( origin - local_origin ).length( ) };
        this->m_entities.push_back( esp_entity_t{ player, index, distance, target, dormant, dead } );
    }

    atl::sort( this->m_entities, []( const esp_entity_t &first, const esp_entity_t &second )
    {
        return first.m_distance > second.m_distance;
    } );
}

void esp::paint( )
{
    const u32 delta{ gui::animation_delta( this->m_last_tick ) };

    if ( !g_cl.m_local || !g_csgo.m_engine->IsInGame( ) )
    {
        return;
    }

    for ( const esp_entity_t &entry : this->m_entities )
    {
        if ( g_csgo.m_entity_list->GetClientEntity( entry.m_index ) != entry.m_ent )
        {
            continue;
        }

        visual_record_t &visual{ this->m_records[ entry.m_index ] };

        vec3 origin{ };
        if ( !entry.m_dormant )
        {
            origin = entry.m_ent->GetAbsOrigin( );
        } else if ( !g_player_data.dormant_origin( entry.m_index, origin, visual.m_dormant_left ) )
        {
            continue;
        }

        rect_t bounds{ };
        if ( !calc_bbox( entry.m_ent, origin, bounds ) || !g_cl.is_on_screen( bounds ) )
        {
            continue;
        }

        this->draw_player( entry, bounds, visual, delta );
    }
}

void esp::draw_player( const esp_entity_t &entry, const rect_t &bounds, visual_record_t &visual, u32 delta )
{
    const config::esp_profile_t &profile{
        g_cfg.m_esp.m_profiles[ static_cast< usize >( entry.m_target ) ]
    };

    const u8 faded{ entry.m_dead ? static_cast< u8 >( 0 ) : entry_alpha( visual ) };

    const float response{ entry.m_dead ? DEATH_FADE_RESPONSE : ESP_ALPHA_RESPONSE };

    visual.m_alpha = math::animate( visual.m_alpha, static_cast< float >( faded ) / 255.0f, delta, response );

    const u8 opacity{ static_cast< u8 >( math::clamp( visual.m_alpha, 0.0f, 1.0f ) * 255.0f ) };

    if ( !opacity )
    {
        return;
    }

    C_CSPlayer *player{ entry.m_ent };

    if ( element_on( profile, config::esp_element_t::box ) )
    {
        const color_t shadow{ modulate( { 0, 0, 0, profile.m_box_color.a }, opacity ) };
        g_draw.box( rect_t{ bounds.left - 1, bounds.top - 1, bounds.right + 1, bounds.bottom + 1 }, shadow );
        g_draw.box( bounds, modulate( profile.m_box_color, opacity ) );
        g_draw.box( rect_t{ bounds.left + 1, bounds.top + 1, bounds.right - 1, bounds.bottom - 1 }, shadow );
    }

    esp_layout_t layout{ bounds };

    if ( element_on( profile, config::esp_element_t::health ) )
    {
        this->draw_health_bar( layout, ELEMENT_ANCHORS[ static_cast< usize >( config::esp_element_t::health ) ],
                               profile, visual, entry.m_dead ? 0 : player->m_iHealth( ), delta, opacity );
    }

    for ( usize slot{ }; slot < config::ESP_ELEMENT_COUNT; ++slot )
    {
        const auto element{ static_cast< config::esp_element_t >( slot ) };

        if ( !element_on( profile, element ) )
        {
            continue;
        }

        const config::esp_anchor_t anchor{ ELEMENT_ANCHORS[ slot ] };
        const config::esp_align_t  align{ ELEMENT_ALIGNS[ slot ] };

        switch ( element )
        {
            case config::esp_element_t::name:
            {
                player_info_t info{ };
                if ( g_csgo.m_engine->GetPlayerInfo( entry.m_index, &info ) )
                {
                    const point_t at{ layout.claim_block( anchor, align, ESP_NAME_HEIGHT ) };

                    g_draw.text( ATLAS_XORSTR( "ESP Name" ).get( ), at, modulate( profile.m_name_color, opacity ),
                                 info.name, true, esp_layout_t::align_for( anchor ) );
                }

                break;
            }

            case config::esp_element_t::distance:
            {
                char text[ 12 ]{ };
                str::format_to( text, sizeof( text ), ATLAS_XORSTR( "[{}M]" ).get( ),
                                static_cast< i32 >( entry.m_distance * UNITS_TO_METERS + 0.5f ) );

                const point_t at{ layout.claim_block( anchor, align, ESP_SMALL_HEIGHT ) };

                g_draw.text( ATLAS_XORSTR( "ESP Small" ).get( ), at, modulate( profile.m_distance_color, opacity ),
                             text, true, esp_layout_t::align_for( anchor ) );
                break;
            }

            case config::esp_element_t::weapon:
            {
                C_BaseCombatWeapon *weapon{ entry.m_dormant || entry.m_dead ? nullptr : player->m_hActiveWeapon( ).Get( ) };
                const char         *name{ weapon ? weapon_label( weapon->m_iItemDefinitionIndex( ) ) : nullptr };

                if ( name )
                {
                    const point_t at{ layout.claim_block( anchor, align, ESP_SMALL_HEIGHT ) };

                    g_draw.text( ATLAS_XORSTR( "ESP Small" ).get( ), at, modulate( profile.m_weapon_color, opacity ),
                                 name, true, esp_layout_t::align_for( anchor ) );
                }
                break;
            }

            case config::esp_element_t::flags:
            {
                this->draw_flags( entry, profile, layout, anchor, align, visual, delta, opacity );
                break;
            }

            default:
                break;
        }
    }
}

void esp::draw_health_bar( esp_layout_t &layout, config::esp_anchor_t anchor, const config::esp_profile_t &profile,
                           visual_record_t &visual, i32 health, u32 delta, u8 opacity )
{
    const bool vertical{ anchor == config::esp_anchor_t::left ||
                         anchor == config::esp_anchor_t::right };
    rect_t     bar{ layout.claim_bar( anchor ) };

    if ( vertical )
    {
        ++bar.bottom;
    }

    const i32 span{ vertical ? bar.bottom - bar.top : bar.right - bar.left };

    if ( span < 4 )
    {
        return;
    }

    const float ratio{ math::clamp( static_cast< float >( health ) / static_cast< float >( MAX_HEALTH ), 0.0f, 1.0f ) };

    if ( !visual.m_seeded )
    {
        visual.m_fill   = ratio;
        visual.m_lag    = ratio;
        visual.m_ratio  = ratio;
        visual.m_seeded = true;
    }

    if ( ratio < visual.m_ratio )
    {
        visual.m_lag_hold = 0.2f;
    }

    visual.m_ratio = ratio;
    visual.m_fill  = math::animate( visual.m_fill, ratio, delta, HEALTH_FILL_RESPONSE );

    if ( visual.m_lag <= visual.m_fill )
    {
        visual.m_lag      = visual.m_fill;
        visual.m_lag_hold = 0.0f;
    } else if ( visual.m_lag_hold > 0.0f )
    {
        visual.m_lag_hold -= static_cast< float >( delta ) / 1000.0f;
    } else
    {
        visual.m_lag = math::animate( visual.m_lag, visual.m_fill, delta, 210.0f );
    }

    g_draw.box( rect_t{ bar.left - 1, bar.top - 1, bar.right + 1, bar.bottom + 1 },
                modulate( ESP_BAR_BACKGROUND, opacity ), true );

    const i32 fill{ scale_to_height( span, visual.m_fill ) };
    const i32 lag{ scale_to_height( span, visual.m_lag ) };

    if ( lag > fill )
    {
        g_draw.box( bar_fill( bar, vertical, lag, fill ), modulate( color_t{ 85, 22, 22 }, opacity ), true );
    }

    const usize   style{ profile.m_health_style };
    const color_t head_color{ style == config::HEALTH_STYLE_HEALTH
                                  ? profile.m_health_low_color.blend( profile.m_health_color, visual.m_fill )
                                  : profile.m_health_color };
    const color_t base_color{ style == config::HEALTH_STYLE_GRADIENT ? profile.m_health_low_color : head_color };

    if ( fill > 0 )
    {
        g_draw.gradient( bar_fill( bar, vertical, fill, 0 ),
                         modulate( vertical ? head_color : base_color, opacity ),
                         modulate( vertical ? base_color : head_color, opacity ), vertical );
    }

    draw_bar_divisions( bar, vertical, profile.m_health_divisions, opacity );

    visual.m_hp_pop = math::animate( visual.m_hp_pop, health < MAX_HEALTH ? 1.0f : 0.0f, delta, 170.0f );

    if ( visual.m_hp_pop <= 0.02f )
    {
        return;
    }

    char text[ 12 ]{ };
    str::format_to( text, sizeof( text ), ATLAS_XORSTR( "{}HP" ).get( ), health );

    const float   eased{ math::smooth_step( visual.m_hp_pop ) };
    const u8      alpha{ static_cast< u8 >( eased * static_cast< float >( opacity ) ) };
    const color_t text_color{ modulate( head_color, alpha ) };
    const i32     slide{ static_cast< i32 >( ( 1.0f - eased ) * 5.0f + 0.5f ) };

    if ( !vertical )
    {
        g_draw.text( ATLAS_XORSTR( "ESP Small" ).get( ),
                     point_t{ bar.left - 2 - slide, bar.top - ESP_SMALL_HEIGHT / 2 }, text_color,
                     text, true, render::text_align::right );
        return;
    }

    const i32 y{ math::clamp( bar.bottom - fill - 4, bar.top - 1, bar.bottom - 9 ) };

    if ( anchor == config::esp_anchor_t::right )
    {
        g_draw.text( ATLAS_XORSTR( "ESP Small" ).get( ), point_t{ bar.right + 2 + slide, y },
                     text_color, text, true, render::text_align::left );
        return;
    }

    g_draw.text( ATLAS_XORSTR( "ESP Small" ).get( ), point_t{ bar.left - 2 - slide, y }, text_color,
                 text, true, render::text_align::right );
}

static float flash_left( C_CSPlayer *player )
{
    const float &duration{ player->m_flFlashDuration( ) };
    if ( duration <= 0.0f )
    {
        return 0.0f;
    }

    const float flash_time{ *reinterpret_cast< const float * >( reinterpret_cast< const u8 * >( &duration ) - 0x14 ) };

    return math::clamp( ( flash_time - g_csgo.m_globals->curtime ) / duration, 0.0f, 1.0f );
}

static u32 flag_mask( const esp_entity_t &entry )
{
    if ( entry.m_dormant )
    {
        return 1u << config::ESP_FLAG_DORMANT;
    }

    if ( entry.m_dead )
    {
        return 0u;
    }

    C_CSPlayer *player{ entry.m_ent };
    u32         mask{ };

    if ( player->m_ArmorValue( ) > 0 )
    {
        mask |= 1u << config::ESP_FLAG_ARMOR;
    }

    mask |= 1u << config::ESP_FLAG_MONEY;

    if ( player->m_bHasDefuser( ) )
    {
        mask |= 1u << config::ESP_FLAG_KIT;
    }

    if ( player->m_bIsScoped( ) )
    {
        mask |= 1u << config::ESP_FLAG_SCOPED;
    }

    if ( player->m_bIsDefusing( ) )
    {
        mask |= 1u << config::ESP_FLAG_DEFUSING;
    }

    if ( flash_left( player ) > 0.0f )
    {
        mask |= 1u << config::ESP_FLAG_FLASHED;
    }

    if ( player->m_bGunGameImmunity( ) )
    {
        mask |= 1u << config::ESP_FLAG_IMMUNE;
    }

    return mask;
}

void esp::draw_flags( const esp_entity_t &entry, const config::esp_profile_t &profile, esp_layout_t &layout,
                      config::esp_anchor_t anchor, config::esp_align_t align, visual_record_t &visual, u32 delta,
                      u8 opacity )
{
    static const color_t FLAG_COLORS[ config::ESP_FLAG_COUNT ]{
        { 220, 220, 220 },
        { 240, 240, 240 },
        { 120, 220, 120 },
        { 90, 150, 255 },
        { 60, 215, 235 },
        { 255, 70, 70 },
        { 255, 255, 255 },
        { 255, 220, 60 },
    };

    const u32 mask{ flag_mask( entry ) };

    float height{ };

    for ( usize index{ }; index < config::ESP_FLAG_COUNT; ++index )
    {
        const bool active{ profile.m_flags[ index ] && ( ( mask >> index ) & 1u ) != 0u };
        float     &pop{ visual.m_flag_pop[ index ] };

        pop = active ? math::animate( pop, 1.0f, delta, FLAG_IN_RESPONSE ) : 0.0f;

        if ( pop >= 0.02f )
        {
            height += FLAG_STEP * pop;
        }
    }

    if ( height < 1.0f )
    {
        return;
    }

    const point_t            block{ layout.claim_block( anchor, align, static_cast< i32 >( height ) ) };
    const render::text_align text_align{ esp_layout_t::align_for( anchor ) };
    const bool               leftwards{ anchor == config::esp_anchor_t::left };

    float y_cursor{ static_cast< float >( block.y ) };

    for ( usize index{ }; index < config::ESP_FLAG_COUNT; ++index )
    {
        const float pop{ visual.m_flag_pop[ index ] };

        if ( pop < 0.02f )
        {
            continue;
        }

        const float eased{ math::smooth_step( pop ) };

        const float fade{ index == config::ESP_FLAG_FLASHED ? flash_left( entry.m_ent ) : 1.0f };
        const u8    alpha{ static_cast< u8 >( eased * fade * static_cast< float >( opacity ) ) };

        const float slide{ ( 1.0f - eased ) * 4.0f };
        const i32   x{ static_cast< i32 >( static_cast< float >( block.x ) + ( leftwards ? slide : -slide ) + 0.5f ) };
        const i32   y{ static_cast< i32 >( y_cursor + 0.5f ) };

        char        text[ 16 ]{ };
        const char *label{ text };

        switch ( index )
        {
            case config::ESP_FLAG_DORMANT: label = ATLAS_XORSTR_STATIC( "DORMANT" ); break;
            case config::ESP_FLAG_ARMOR: label = entry.m_ent->m_bHasHelmet( ) ? ATLAS_XORSTR_STATIC( "HK" ) : ATLAS_XORSTR_STATIC( "K" ); break;
            case config::ESP_FLAG_MONEY: str::format_to( text, sizeof( text ), ATLAS_XORSTR( "${}" ).get( ), entry.m_ent->m_iAccount( ) ); break;
            case config::ESP_FLAG_KIT: label = ATLAS_XORSTR_STATIC( "KIT" ); break;
            case config::ESP_FLAG_SCOPED: label = ATLAS_XORSTR_STATIC( "SCOPED" ); break;
            case config::ESP_FLAG_DEFUSING: label = ATLAS_XORSTR_STATIC( "DEFUSING" ); break;
            case config::ESP_FLAG_FLASHED: label = ATLAS_XORSTR_STATIC( "FLASHED" ); break;
            case config::ESP_FLAG_IMMUNE: label = ATLAS_XORSTR_STATIC( "IMMUNE" ); break;
            default: break;
        }

        g_draw.text( ATLAS_XORSTR( "ESP Small" ).get( ), point_t{ x, y }, modulate( FLAG_COLORS[ index ], alpha ),
                     label, true, text_align );

        y_cursor += FLAG_STEP * pop;
    }
}
