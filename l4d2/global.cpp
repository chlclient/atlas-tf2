#include "global.hpp"

#include "config.hpp"

ATLAS_NO_DESTROY l4d2    g_l4d2{ };
ATLAS_NO_DESTROY client  g_cl{ };
ATLAS_NO_DESTROY api     g_api{ };
ATLAS_NO_DESTROY netvars g_netvars{ };
ATLAS_NO_DESTROY hooks   g_hooks{ };
ATLAS_NO_DESTROY config  g_cfg{ };

void client::update_local_data( )
{
    IClientEntity *local_entity{ g_l4d2.m_entity_list->GetClientEntity( g_l4d2.m_engine->GetLocalPlayer( ) ) };

    this->m_local  = local_entity ? local_entity->As< C_TerrorPlayer * >( ) : nullptr;
    this->m_weapon = nullptr;

    if ( !this->m_local )
    {
        return;
    }

    if ( C_BaseEntity * active_weapon{ this->m_local->m_hActiveWeapon( ).Get( ) } )
    {
        this->m_weapon = active_weapon->As< C_BaseCombatWeapon * >( );
    }
}

void client::fix_movement( CUserCmd *cmd, const vec3 &original_angles )
{
    const float original_yaw{ DEG2RAD( original_angles.y ) };
    const float current_yaw{ DEG2RAD( cmd->viewangles.y ) };

    const float world_x{ cmd->forwardmove * math::cos( original_yaw ) + cmd->sidemove * math::sin( original_yaw ) };
    const float world_y{ cmd->forwardmove * math::sin( original_yaw ) - cmd->sidemove * math::cos( original_yaw ) };

    cmd->forwardmove = world_x * math::cos( current_yaw ) + world_y * math::sin( current_yaw );
    cmd->sidemove    = world_x * math::sin( current_yaw ) - world_y * math::cos( current_yaw );
}

void client::update_w2s( )
{
    atl::memcpy( this->m_w2s, g_l4d2.m_engine->WorldToScreenMatrix( ), sizeof( VMatrix ) );
    g_l4d2.m_engine->GetScreenSize( this->m_screen_width, this->m_screen_height );
}

bool client::world_to_screen( const vec3 &world, vec2 &screen ) const
{
    const auto &m = this->m_w2s;

    const float w{ m[ 3 ][ 0 ] * world.x + m[ 3 ][ 1 ] * world.y + m[ 3 ][ 2 ] * world.z + m[ 3 ][ 3 ] };
    if ( w <= 0.001f )
    {
        return false;
    }

    const float cx{ m[ 0 ][ 0 ] * world.x + m[ 0 ][ 1 ] * world.y + m[ 0 ][ 2 ] * world.z + m[ 0 ][ 3 ] };
    const float cy{ m[ 1 ][ 0 ] * world.x + m[ 1 ][ 1 ] * world.y + m[ 1 ][ 2 ] * world.z + m[ 1 ][ 3 ] };

    screen.x = 0.5f * ( 1.f + cx / w ) * static_cast< float >( this->m_screen_width );
    screen.y = 0.5f * ( 1.f - cy / w ) * static_cast< float >( this->m_screen_height );

    return true;
}

bool client::clip_segment_to_screen( const vec3 &start, const vec3 &end, vec2 &screen_start, vec2 &screen_end ) const
{
    constexpr float NEAR_W{ 0.01f };

    const auto &m = this->m_w2s;

    const auto projected_w{ [ & ]( const vec3 &point )
    {
        return m[ 3 ][ 0 ] * point.x + m[ 3 ][ 1 ] * point.y + m[ 3 ][ 2 ] * point.z + m[ 3 ][ 3 ];
    } };

    const float start_w{ projected_w( start ) };
    const float end_w{ projected_w( end ) };

    if ( start_w < NEAR_W && end_w < NEAR_W )
    {
        return false;
    }

    vec3 visible_start{ start };
    vec3 visible_end{ end };

    if ( start_w < NEAR_W )
    {
        visible_start = start + ( end - start ) * ( ( NEAR_W - start_w ) / ( end_w - start_w ) );
    } else if ( end_w < NEAR_W )
    {
        visible_end = end + ( start - end ) * ( ( NEAR_W - end_w ) / ( start_w - end_w ) );
    }

    return this->world_to_screen( visible_start, screen_start ) &&
           this->world_to_screen( visible_end, screen_end );
}

void client::draw_3d_line( const vec3 &start, const vec3 &end, const color_t &color ) const
{
    vec2 screen_start{ };
    vec2 screen_end{ };

    if ( color.a != 0 && this->clip_segment_to_screen( start, end, screen_start, screen_end ) )
    {
        g_draw.line( screen_start, screen_end, color );
    }
}

usize client::aim_target( C_BaseEntity *entity ) const
{
    switch ( entity->GetClientClass( )->GetClassID( ) )
    {
        case EL4D2ClassID::Infected: return entity->As< C_Infected * >( )->IsAlive( ) ? config::TARGET_COMMON : config::TARGET_COUNT;
        case EL4D2ClassID::Witch: return entity->As< C_Infected * >( )->IsAlive( ) ? config::TARGET_WITCH : config::TARGET_COUNT;

        case EL4D2ClassID::CTerrorPlayer:
        case EL4D2ClassID::SurvivorBot:
        case EL4D2ClassID::Boomer:
        case EL4D2ClassID::Charger:
        case EL4D2ClassID::Hunter:
        case EL4D2ClassID::Jockey:
        case EL4D2ClassID::Smoker:
        case EL4D2ClassID::Spitter:
        case EL4D2ClassID::Tank:
        {
            auto *player{ entity->As< C_TerrorPlayer * >( ) };

            if ( player == this->m_local ||
                 !player->IsAlive( ) ||
                 player->m_isGhost( ) ||
                 player->m_iTeamNum( ) == this->m_local->m_iTeamNum( ) )
            {
                return config::TARGET_COUNT;
            }

            return static_cast< usize >( config::esp_target_t::enemy );
        }

        default: return config::TARGET_COUNT;
    }
}

usize client::render_target( C_BaseEntity *entity ) const
{
    if ( entity->m_fEffects( ) & EF_NODRAW )
    {
        return config::TARGET_COUNT;
    }

    for ( i32 depth{ }; depth < 4; ++depth )
    {
        switch ( entity->GetClientClass( )->GetClassID( ) )
        {
            case EL4D2ClassID::Infected: return entity->As< C_Infected * >( )->IsAlive( ) ? config::TARGET_COMMON : config::TARGET_COUNT;
            case EL4D2ClassID::Witch: return entity->As< C_Infected * >( )->IsAlive( ) ? config::TARGET_WITCH : config::TARGET_COUNT;

            case EL4D2ClassID::CTerrorPlayer:
            case EL4D2ClassID::SurvivorBot:
            case EL4D2ClassID::Boomer:
            case EL4D2ClassID::Charger:
            case EL4D2ClassID::Hunter:
            case EL4D2ClassID::Jockey:
            case EL4D2ClassID::Smoker:
            case EL4D2ClassID::Spitter:
            case EL4D2ClassID::Tank:
            {
                auto *player{ entity->As< C_TerrorPlayer * >( ) };

                if ( player == this->m_local ||
                     !player->IsAlive( ) ||
                     player->m_isGhost( ) )
                {
                    return config::TARGET_COUNT;
                }

                return static_cast< usize >( player->m_iTeamNum( ) == this->m_local->m_iTeamNum( ) ? config::esp_target_t::team : config::esp_target_t::enemy );
            }

            default: break;
        }

        entity = entity->m_hMoveParent( ).Get( );
        if ( !entity )
        {
            return config::TARGET_COUNT;
        }
    }

    return config::TARGET_COUNT;
}
