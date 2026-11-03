#include "player_data.hpp"

ATLAS_NO_DESTROY player_data g_player_data{ };

float player_data::window( )
{
    return static_cast< float >( g_cfg.m_esp.m_dormant_time );
}

static bool player_slot( i32 index )
{
    return index >= 1 && index < MAX_TRACKED_PLAYERS && index <= g_csgo.m_globals->maxClients;
}

player_record_t *player_data::player( i32 index )
{
    return player_slot( index ) ? &this->m_players[ index ] : nullptr;
}

void player_data::remember( i32 index, const vec3 &origin )
{
    player_record_t *record{ this->player( index ) };
    if ( !record || !this->connected( index ) || !this->alive( index ) )
    {
        return;
    }

    record->m_dormant_origin = origin;
    record->m_dormant_expire = g_csgo.m_globals->curtime + window( );
}

void player_data::on_start_sound( const StartSoundParams_t &params )
{
    if ( !params.fromserver || params.origin.is_zero( ) )
    {
        return;
    }

    if ( !player_slot( params.soundsource ) || params.soundsource == g_csgo.m_engine->GetLocalPlayer( ) )
    {
        return;
    }

    IClientEntity *client_entity{ g_csgo.m_entity_list->GetClientEntity( params.soundsource ) };
    if ( !client_entity || !client_entity->As< C_CSPlayer * >( )->IsDormant( ) )
    {
        return;
    }

    this->remember( params.soundsource, params.origin );
}

void player_data::update( )
{
    if ( !g_csgo.m_engine->IsInGame( ) )
    {
        return;
    }

    const i32 highest{ math::smaller( g_csgo.m_globals->maxClients, MAX_TRACKED_PLAYERS - 1 ) };

    for ( i32 index{ 1 }; index <= highest; ++index )
    {
        player_record_t &record{ this->m_players[ index ] };
        IClientEntity   *client_entity{ g_csgo.m_entity_list->GetClientEntity( index ) };
        C_CSPlayer      *player{ client_entity ? client_entity->As< C_CSPlayer * >( ) : nullptr };

        if ( record.m_player != player )
        {
            record          = player_record_t{ };
            record.m_player = player;
        }

        if ( !player )
        {
            continue;
        }

        const bool dormant{ player->IsDormant( ) };

        if ( dormant && !record.m_dormant )
        {
            this->remember( index, player->GetAbsOrigin( ) );
        }

        record.m_dormant = dormant;

        if ( !dormant || !this->connected( index ) || !this->alive( index ) )
        {
            record.m_dormant_expire = 0.0f;
        }
    }
}

void player_data::on_round_start( )
{
    for ( player_record_t &record : this->m_players )
    {
        record.m_dormant_expire = 0.0f;
    }
}

bool player_data::dormant_origin( i32 index, vec3 &origin, float &remaining ) const
{
    if ( !player_slot( index ) )
    {
        return false;
    }

    const player_record_t &record{ this->m_players[ index ] };
    const float            now{ g_csgo.m_globals->curtime };

    if ( !this->connected( index ) || !this->alive( index ) || record.m_dormant_expire <= now )
    {
        return false;
    }

    origin    = record.m_dormant_origin;
    remaining = record.m_dormant_expire - now;
    return true;
}

bool player_data::alive( i32 index ) const
{
    C_PlayerResource *resource{ *g_csgo.m_player_resource };
    return resource && player_slot( index ) && resource->m_bAlive( )[ index ];
}

bool player_data::connected( i32 index ) const
{
    C_PlayerResource *resource{ *g_csgo.m_player_resource };
    return resource && player_slot( index ) && resource->m_bConnected( )[ index ];
}

i32 player_data::team( i32 index ) const
{
    C_PlayerResource *resource{ *g_csgo.m_player_resource };
    return resource && player_slot( index ) ? resource->m_iTeam( )[ index ] : TEAM_UNASSIGNED;
}
