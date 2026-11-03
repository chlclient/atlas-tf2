#include "esp.hpp"

ATLAS_NO_DESTROY esp g_esp{ };

ATLAS_FORCE_INLINE const char *get_player_name( C_TerrorPlayer *player )
{
    switch ( player->m_zombieClass( ) )
    {
        case ZOMBIE_CLASS_SMOKER: return ATLAS_XORSTR_STATIC( "Smoker" );
        case ZOMBIE_CLASS_BOOMER: return ATLAS_XORSTR_STATIC( "Boomer" );
        case ZOMBIE_CLASS_HUNTER: return ATLAS_XORSTR_STATIC( "Hunter" );
        case ZOMBIE_CLASS_SPITTER: return ATLAS_XORSTR_STATIC( "Spitter" );
        case ZOMBIE_CLASS_JOCKEY: return ATLAS_XORSTR_STATIC( "Jockey" );
        case ZOMBIE_CLASS_CHARGER: return ATLAS_XORSTR_STATIC( "Charger" );
        case ZOMBIE_CLASS_TANK: return ATLAS_XORSTR_STATIC( "Tank" );
        default: return ATLAS_XORSTR_STATIC( "Survivor" );
    }
}

ATLAS_FORCE_INLINE color_t get_infected_color( usize kind )
{
    switch ( kind )
    {
        case config::INFECTED_WITCH: return color_t{ 255, 120, 220, 255 };
        default: return color_t{ 200, 200, 200, 255 };
    }
}

ATLAS_FORCE_INLINE color_t get_health_color( f32 ratio )
{
    const u8 red{ static_cast< u8 >( math::smaller( 510.0f * ( 1.0f - ratio ), 200.0f ) ) };
    const u8 green{ static_cast< u8 >( math::smaller( 510.0f * ratio, 200.0f ) ) };
    return color_t{ red, green, 32, 255 };
}

void esp_target_t::add_flag( const char *label, color_t color )
{
    Assert( this->m_flag_count < ESP_FLAG_CAPACITY );

    this->m_flags[ this->m_flag_count ] = esp_flag_t{ label, color };
    ++this->m_flag_count;
}

void esp::paint( )
{
    if ( !this->should_run( ) )
    {
        return;
    }

    C_TerrorPlayer *local{ g_cl.m_local };

    const i32 highest{ g_l4d2.m_entity_list->GetHighestEntityIndex( ) };
    for ( i32 index{ 1 }; index <= highest; ++index )
    {
        IClientEntity *entity{ g_l4d2.m_entity_list->GetClientEntity( index ) };
        if ( !entity )
        {
            continue;
        }

        esp_target_t target{ };
        if ( !this->get_target( entity->As< C_BaseEntity * >( ), local, target ) )
        {
            continue;
        }

        this->draw_target( target );
    }
}

bool esp::should_run( ) const
{
    return g_l4d2.m_engine->IsInGame( ) && g_cl.m_local;
}

bool esp::get_target( C_BaseEntity *entity, C_TerrorPlayer *local, esp_target_t &target ) const
{
    if ( entity == local ||
         entity->IsDormant( ) )
    {
        return false;
    }

    switch ( entity->GetClientClass( )->GetClassID( ) )
    {
        case EL4D2ClassID::Infected: return this->get_infected_target( entity->As< C_Infected * >( ), config::INFECTED_COMMON, target );
        case EL4D2ClassID::Witch: return this->get_infected_target( entity->As< C_Infected * >( ), config::INFECTED_WITCH, target );

        case EL4D2ClassID::CTerrorPlayer:
        case EL4D2ClassID::SurvivorBot:
        case EL4D2ClassID::Boomer:
        case EL4D2ClassID::Charger:
        case EL4D2ClassID::Hunter:
        case EL4D2ClassID::Jockey:
        case EL4D2ClassID::Smoker:
        case EL4D2ClassID::Spitter:
        case EL4D2ClassID::Tank: return this->get_player_target( entity->As< C_TerrorPlayer * >( ), local, target );

        default: return false;
    }
}

bool esp::get_player_target( C_TerrorPlayer *player, C_TerrorPlayer *local, esp_target_t &target ) const
{
    if ( !player->IsAlive( ) ||
         player->m_isGhost( ) )
    {
        return false;
    }

    const config::esp_target_t   kind{ player->m_iTeamNum( ) == local->m_iTeamNum( ) ? config::esp_target_t::team : config::esp_target_t::enemy };
    const config::esp_profile_t &profile{ g_cfg.m_esp.m_profiles[ static_cast< usize >( kind ) ] };

    bool shown{ };
    for ( usize element{ }; element < config::ESP_ELEMENT_COUNT; ++element )
    {
        target.m_elements[ element ] = profile.m_elements[ element ];
        shown |= profile.m_elements[ element ];
    }

    if ( !shown )
    {
        return false;
    }

    if ( !this->get_bounds( player, target.m_bounds ) )
    {
        return false;
    }

    target.m_box_color  = profile.m_box_color;
    target.m_name_color = profile.m_name_color;
    target.m_name       = get_player_name( player );
    target.m_health     = player->m_iHealth( );
    target.m_max_health = player->m_iMaxHealth( );

    if ( player->m_isIncapacitated( ) )
    {
        target.add_flag( ATLAS_XORSTR_STATIC( "INCAP" ), color_t{ 255, 70, 70, 255 } );
    }

    if ( player->m_isHangingFromLedge( ) )
    {
        target.add_flag( ATLAS_XORSTR_STATIC( "LEDGE" ), color_t{ 255, 160, 60, 255 } );
    }

    if ( player->m_bIsOnThirdStrike( ) )
    {
        target.add_flag( ATLAS_XORSTR_STATIC( "B&W" ), color_t{ 230, 230, 230, 255 } );
    }

    if ( player->m_bAdrenalineActive( ) )
    {
        target.add_flag( ATLAS_XORSTR_STATIC( "ADRENALINE" ), color_t{ 255, 220, 60, 255 } );
    }

    if ( player->IsPinned( ) )
    {
        target.add_flag( ATLAS_XORSTR_STATIC( "PINNED" ), color_t{ 200, 50, 200, 255 } );
    }

    return true;
}

bool esp::get_infected_target( C_Infected *infected, usize kind, esp_target_t &target ) const
{
    const bool ( &elements )[ config::INFECTED_ELEMENT_COUNT ]{ g_cfg.m_esp.m_infected_elements[ kind ] };

    target.m_elements[ static_cast< usize >( config::esp_element_t::box ) ]   = elements[ config::INFECTED_ELEMENT_BOX ];
    target.m_elements[ static_cast< usize >( config::esp_element_t::name ) ]  = elements[ config::INFECTED_ELEMENT_NAME ];
    target.m_elements[ static_cast< usize >( config::esp_element_t::flags ) ] = elements[ config::INFECTED_ELEMENT_FLAGS ];

    if ( !elements[ config::INFECTED_ELEMENT_BOX ] &&
         !elements[ config::INFECTED_ELEMENT_NAME ] &&
         !elements[ config::INFECTED_ELEMENT_FLAGS ] )
    {
        return false;
    }

    if ( !infected->IsAlive( ) )
    {
        return false;
    }

    if ( !this->get_bounds( infected, target.m_bounds ) )
    {
        return false;
    }

    target.m_box_color  = get_infected_color( kind );
    target.m_name_color = color_t{ 255, 255, 255, 255 };
    target.m_name       = kind == config::INFECTED_WITCH ? ATLAS_XORSTR_STATIC( "Witch" ) : ATLAS_XORSTR_STATIC( "Infected" );

    if ( infected->m_bIsBurning( ) )
    {
        target.add_flag( ATLAS_XORSTR_STATIC( "BURNING" ), color_t{ 255, 140, 40, 255 } );
    }

    return true;
}

bool esp::get_bounds( C_BaseEntity *entity, rect_t &bounds ) const
{
    const vec3 origin{ entity->m_vecAbsOrigin( ) };
    const vec3 mins{ origin + entity->m_vecMins( ) };
    const vec3 maxs{ origin + entity->m_vecMaxs( ) };

    const vec3 corners[ 8 ]{
        vec3{ mins.x, mins.y, mins.z },
        vec3{ maxs.x, mins.y, mins.z },
        vec3{ mins.x, maxs.y, mins.z },
        vec3{ maxs.x, maxs.y, mins.z },
        vec3{ mins.x, mins.y, maxs.z },
        vec3{ maxs.x, mins.y, maxs.z },
        vec3{ mins.x, maxs.y, maxs.z },
        vec3{ maxs.x, maxs.y, maxs.z },
    };

    vec2 low{ };
    vec2 high{ };
    for ( usize index{ }; index < 8; ++index )
    {
        vec2 screen{ };
        if ( !g_cl.world_to_screen( corners[ index ], screen ) )
        {
            return false;
        }

        if ( index == 0 )
        {
            low  = screen;
            high = screen;
            continue;
        }

        low.x  = math::smaller( screen.x, low.x );
        low.y  = math::smaller( screen.y, low.y );
        high.x = math::bigger( screen.x, high.x );
        high.y = math::bigger( screen.y, high.y );
    }

    bounds = rect_t{ static_cast< i32 >( low.x ), static_cast< i32 >( low.y ),
                     static_cast< i32 >( high.x ), static_cast< i32 >( high.y ) };

    return true;
}

void esp::draw_target( const esp_target_t &target ) const
{
    if ( target.shows( config::esp_element_t::box ) )
    {
        this->draw_box( target );
    }

    if ( target.shows( config::esp_element_t::health ) && target.m_max_health > 0 )
    {
        this->draw_health_bar( target );
    }

    if ( target.shows( config::esp_element_t::name ) )
    {
        this->draw_name( target );
    }

    if ( target.shows( config::esp_element_t::flags ) )
    {
        this->draw_flags( target );
    }
}

void esp::draw_box( const esp_target_t &target ) const
{
    const rect_t &box{ target.m_bounds };
    const color_t shadow{ 0, 0, 0, 255 };

    g_draw.box( rect_t{ box.left - 1, box.top - 1, box.right + 1, box.bottom + 1 }, shadow );
    g_draw.box( box, target.m_box_color );
    g_draw.box( rect_t{ box.left + 1, box.top + 1, box.right - 1, box.bottom - 1 }, shadow );
}

void esp::draw_health_bar( const esp_target_t &target ) const
{
    const rect_t &box{ target.m_bounds };
    const rect_t  bar{ box.left - ESP_BAR_GAP - ESP_BAR_WIDTH, box.top, box.left - ESP_BAR_GAP, box.bottom + 1 };

    const i32 span{ bar.bottom - bar.top };
    if ( span < 4 )
    {
        return;
    }

    const f32 ratio{ math::clamp( static_cast< f32 >( target.m_health ) / static_cast< f32 >( target.m_max_health ), 0.0f, 1.0f ) };
    const i32 fill{ static_cast< i32 >( static_cast< f32 >( span ) * ratio + 0.5f ) };

    g_draw.box( rect_t{ bar.left - 1, bar.top - 1, bar.right + 1, bar.bottom + 1 }, ESP_BAR_BACKGROUND, true );

    if ( fill > 0 )
    {
        g_draw.box( rect_t{ bar.left, bar.bottom - fill, bar.right, bar.bottom }, get_health_color( ratio ), true );
    }
}

void esp::draw_name( const esp_target_t &target ) const
{
    const rect_t &box{ target.m_bounds };
    const point_t position{ box.left + ( box.right - box.left ) / 2, box.top - ESP_BLOCK_GAP - ESP_NAME_HEIGHT };

    g_draw.text( ATLAS_XORSTR_STATIC( "ESP Name" ), position, target.m_name_color, target.m_name, true, render::text_align::center );
}

void esp::draw_flags( const esp_target_t &target ) const
{
    const rect_t &box{ target.m_bounds };

    for ( usize index{ }; index < target.m_flag_count; ++index )
    {
        const esp_flag_t &flag{ target.m_flags[ index ] };
        const point_t     position{ box.right + ESP_BAR_GAP, box.top + static_cast< i32 >( index ) * ESP_SMALL_HEIGHT };

        g_draw.text( ATLAS_XORSTR_STATIC( "ESP Small" ), position, flag.m_color, flag.m_label, true, render::text_align::left );
    }
}
