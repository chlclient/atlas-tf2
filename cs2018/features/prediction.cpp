#include "prediction.hpp"

ATLAS_NO_DESTROY engine_prediction g_prediction{ };

// m_vecBaseVelocity's step: 20 bits over -1000..1000, which has no code for 0
// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/server/player.cpp#L8279
constexpr float BASE_VELOCITY_STEP{ 2000.0f / static_cast< float >( ( 1 << 20 ) - 1 ) };

static RecvVarProxyFn s_base_velocity_proxy{ };

static void suppress_events( bool suppress )
{
    for ( IPredictionSystem *system{ g_csgo.m_prediction_systems }; system; system = system->m_pNextSystem )
    {
        system->m_bSuppressEvent = suppress;
    }
}

static typedescription_t *find_flattened_field( const datamap_t *map, hash_t name )
{
    if ( !map->m_pOptimizedDataMap )
    {
        return nullptr;
    }

    for ( flattenedoffsets_t &chain : map->m_pOptimizedDataMap->m_Info )
    {
        for ( i32 index{ }; index < chain.m_nSize; ++index )
        {
            typedescription_t &field{ chain.m_Flattened[ index ] };
            if ( field.fieldName && HASH_RT( field.fieldName ) == name )
            {
                return &field;
            }
        }
    }

    return nullptr;
}

bool datamap_copy_t::ensure( usize needed )
{
    if ( this->m_data && this->m_size == needed )
    {
        return true;
    }

    void *block{ this->m_data ? g_csgo.m_alloc->Realloc( this->m_data, needed ) : g_csgo.m_alloc->Alloc( needed ) };
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

    g_csgo.m_alloc->Free( this->m_data );

    this->m_data = nullptr;
    this->m_size = 0;
}

static bool save_entity_map( C_BaseEntity *entity, datamap_copy_t &buffer )
{
    datamap_t *map{ entity->GetPredDescMap( ) };

    if ( map->m_nPackedSize <= 0 || !buffer.ensure( static_cast< usize >( map->m_nPackedSize ) ) )
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

static void base_velocity_proxy( const CRecvProxyData *data, void *entity, void *out )
{
    if ( g_cl.m_is_running )
    {
        CRecvProxyData *changed{ const_cast< CRecvProxyData * >( data ) };
        for ( float &component : changed->m_Vector )
        {
            if ( math::abs( component ) < BASE_VELOCITY_STEP )
            {
                component = 0.0f;
            }
        }
    }

    s_base_velocity_proxy( data, entity, out );
}

bool engine_prediction::install_proxies( )
{
    RecvProp *base_velocity{ g_netvars.get_prop( HASH_CT( "DT_BasePlayer" ), HASH_CT( "m_vecBaseVelocity" ) ) };
    if ( !base_velocity )
    {
        return false;
    }

    s_base_velocity_proxy    = base_velocity->m_ProxyFn;
    base_velocity->m_ProxyFn = &base_velocity_proxy;
    return true;
}

void engine_prediction::post_think( )
{
    this->m_player->ItemPostFrame( );
}

void engine_prediction::update( )
{
    g_csgo.m_CL_RunPrediction.call_as< void >( );
}

bool engine_prediction::start( )
{
    if ( g_cl.m_cmd->weaponselect != 0 )
    {
        return false;
    }

    this->m_player = g_cl.m_local;
    this->m_weapon = this->m_player->m_hActiveWeapon( ).Get( );
    for ( usize index{ }; index < MAX_VIEWMODELS; ++index )
    {
        this->m_viewmodels[ index ] = ( &this->m_player->m_hViewModel( ) )[ index ].Get( );
    }

    if ( !this->m_viewmodel_sequence && this->m_viewmodels[ 0 ] )
    {
        this->m_viewmodel_sequence = find_flattened_field( this->m_viewmodels[ 0 ]->GetPredDescMap( ), HASH_CT( "m_nSequence" ) );
        if ( this->m_viewmodel_sequence )
        {
            this->m_viewmodel_sequence->flags |= FTYPEDESC_NOERRORCHECK;
        }
    }

    if ( !this->save_data_maps( ) )
    {
        this->m_player = nullptr;
        this->m_weapon = nullptr;
        return false;
    }

    this->m_backup.store( );

    g_csgo.m_prediction->GetLocalViewAngles( this->m_local_view_angles );

    if ( this->m_weapon )
    {
        this->m_last_fire_bullet_time = this->m_weapon->m_flLastClientFireBulletTime( );
    }

    this->m_in_prediction        = g_csgo.m_prediction->m_bInPrediction;
    this->m_first_time_predicted = g_csgo.m_prediction->m_bFirstTimePredicted;

    g_csgo.m_prediction->m_bFirstTimePredicted = false;
    g_csgo.m_prediction->m_bInPrediction       = true;

    suppress_events( true );

    g_csgo.m_move_helper->SetHost( this->m_player );

    this->m_player->m_vecPreviouslyPredictedOrigin( ) = this->m_player->m_vecOrigin( );

    this->m_is_active = true;
    g_csgo.m_prediction->RunCommand( this->m_player, g_cl.m_cmd, g_csgo.m_move_helper );
    this->m_is_active = false;

    g_csgo.m_move_helper->SetHost( nullptr );

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
    g_csgo.m_prediction->SetLocalViewAngles( this->m_local_view_angles );

    if ( this->m_weapon )
    {
        this->m_weapon->m_flLastClientFireBulletTime( ) = this->m_last_fire_bullet_time;
    }

    this->m_backup.restore( );

    g_csgo.m_prediction->m_bInPrediction       = this->m_in_prediction;
    g_csgo.m_prediction->m_bFirstTimePredicted = this->m_first_time_predicted;

    this->m_player = nullptr;
    this->m_weapon = nullptr;
}
