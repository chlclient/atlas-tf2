#include "prediction.hpp"
#include "datamaps.hpp"

ATLAS_NO_DESTROY engine_prediction g_prediction{ };

static void suppress_events( bool suppress )
{
    for ( IPredictionSystem *system{ g_l4d2.m_prediction_systems }; system; system = system->m_pNextSystem )
    {
        system->m_bSuppressEvent = suppress;
    }
}

bool datamap_copy_t::ensure( usize needed )
{
    if ( this->m_data && this->m_size == needed )
    {
        return true;
    }

    void *block{ this->m_data ? g_l4d2.m_alloc->Realloc( this->m_data, needed ) : g_l4d2.m_alloc->Alloc( needed ) };
    if ( !block )
    {
        this->release( );
        return false;
    }

    this->m_data = static_cast< u8 * >( block );
    this->m_size = needed;

    atl::memset( this->m_data, 0, needed );

    return true;
}

void datamap_copy_t::release( )
{
    if ( !this->m_data )
    {
        return;
    }

    g_l4d2.m_alloc->Free( this->m_data );

    this->m_data = nullptr;
    this->m_size = 0;
}

static bool save_entity_map( C_BaseEntity *entity, datamap_copy_t &buffer )
{
    datamap_t *map{ entity->GetPredDescMap( ) };

    if ( !map || map->m_nPackedSize <= 0 || !buffer.ensure( static_cast< usize >( map->m_nPackedSize ) ) )
    {
        return false;
    }

    CPredictionCopy copy{ PC_EVERYTHING, buffer.m_data, PC_DATA_PACKED, entity, PC_DATA_NORMAL };
    copy.TransferData( ATLAS_XORSTR( "ATL_PREDICTION_SAVE" ).get( ), entity->entindex( ), map );
    return true;
}

static void restore_entity_map( C_BaseEntity *entity, const datamap_copy_t &buffer )
{
    CPredictionCopy copy{ PC_EVERYTHING, entity, PC_DATA_NORMAL, buffer.m_data, PC_DATA_PACKED };
    copy.TransferData( ATLAS_XORSTR( "ATL_PREDICTION_RESTORE" ).get( ), entity->entindex( ), entity->GetPredDescMap( ) );
}

bool engine_prediction::save_data_maps( )
{
    if ( !save_entity_map( this->m_player, this->m_player_buffer ) )
    {
        return false;
    }

    if ( this->m_weapon && !save_entity_map( this->m_weapon, this->m_weapon_buffer ) )
    {
        return false;
    }

    for ( usize index{ }; index < MAX_VIEWMODELS; ++index )
    {
        if ( this->m_viewmodels[ index ] && !save_entity_map( this->m_viewmodels[ index ], this->m_viewmodel_buffers[ index ] ) )
        {
            return false;
        }
    }

    return true;
}

void engine_prediction::restore_data_maps( )
{
    restore_entity_map( this->m_player, this->m_player_buffer );

    if ( this->m_weapon )
    {
        restore_entity_map( this->m_weapon, this->m_weapon_buffer );
    }

    for ( usize index{ }; index < MAX_VIEWMODELS; ++index )
    {
        if ( this->m_viewmodels[ index ] )
        {
            restore_entity_map( this->m_viewmodels[ index ], this->m_viewmodel_buffers[ index ] );
        }
    }
}

bool engine_prediction::begin_weapon( )
{
    if ( this->m_weapon_entered )
    {
        return false;
    }

    this->m_weapon_entered = true;
    return true;
}

void engine_prediction::post_think( )
{
    this->m_player->ItemPostFrame( );
}

void engine_prediction::update( )
{
    g_l4d2.m_CL_RunPrediction.call_as< void >( );
}

bool engine_prediction::start( )
{
    if ( this->m_player || !g_cl.m_cmd || !g_cl.m_local || !g_cl.m_local->IsAlive( ) )
    {
        return false;
    }

    if ( g_cl.m_cmd->weaponselect != 0 )
    {
        return false;
    }

    this->m_player = g_cl.m_local;
    this->m_weapon = g_cl.m_weapon;
    for ( usize index{ }; index < MAX_VIEWMODELS; ++index )
    {
        this->m_viewmodels[ index ] = ( &this->m_player->m_hViewModel( ) )[ index ].Get( );
    }

    if ( !this->save_data_maps( ) )
    {
        this->m_player = nullptr;
        this->m_weapon = nullptr;
        return false;
    }

    this->m_backup.store( );

    g_l4d2.m_prediction->GetLocalViewAngles( this->m_local_view_angles );

    this->m_slot                 = g_l4d2.m_engine->GetActiveSplitScreenPlayerSlot( );
    this->m_in_prediction        = g_l4d2.m_prediction->m_bInPrediction;
    this->m_first_time_predicted = g_l4d2.m_prediction->m_Split[ this->m_slot ].m_bFirstTimePredicted;

    g_l4d2.m_prediction->m_Split[ this->m_slot ].m_bFirstTimePredicted = false;
    g_l4d2.m_prediction->m_bInPrediction                               = true;

    suppress_events( true );

    g_l4d2.m_move_helper->SetHost( this->m_player );

    this->m_weapon_entered = false;
    this->m_is_active      = true;
    g_l4d2.m_prediction->RunCommand( this->m_player, g_cl.m_cmd, g_l4d2.m_move_helper );
    this->m_is_active = false;

    g_l4d2.m_move_helper->SetHost( nullptr );

    suppress_events( false );

    return true;
}

void engine_prediction::end( )
{
    if ( !this->m_player )
    {
        return;
    }

    this->restore_data_maps( );
    g_l4d2.m_prediction->SetLocalViewAngles( this->m_local_view_angles );

    this->m_backup.restore( );

    g_l4d2.m_prediction->m_bInPrediction                               = this->m_in_prediction;
    g_l4d2.m_prediction->m_Split[ this->m_slot ].m_bFirstTimePredicted = this->m_first_time_predicted;

    this->m_player = nullptr;
    this->m_weapon = nullptr;
}

void engine_prediction::shutdown( )
{
    this->m_player_buffer.release( );
    this->m_weapon_buffer.release( );
    for ( usize index{ }; index < MAX_VIEWMODELS; ++index )
    {
        this->m_viewmodel_buffers[ index ].release( );
        this->m_viewmodels[ index ] = nullptr;
    }

    this->m_is_active = false;
    this->m_player    = nullptr;
    this->m_weapon    = nullptr;
}
