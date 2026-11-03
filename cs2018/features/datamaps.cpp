#include "datamaps.hpp"

ATLAS_NO_DESTROY datamap_editor g_datamap{ };

static i32 get_size( fieldtype_t type )
{
    switch ( type )
    {
        case FIELD_BOOLEAN: return 1;
        case FIELD_FLOAT:
        case FIELD_INTEGER:
        default: return 4;
    }
}

static bool initialize_datamap( const named_field_t *fields, usize count, datamap_t *datamap )
{
    const i32 existing{ datamap->dataNumFields };

    typedescription_t *desc{ static_cast< typedescription_t * >( g_csgo.m_alloc->Alloc( ( static_cast< usize >( existing ) + count ) * sizeof( typedescription_t ) ) ) };
    if ( !desc )
    {
        return false;
    }

    atl::memcpy( desc, datamap->dataDesc, static_cast< usize >( existing ) * sizeof( typedescription_t ) );

    for ( usize index{ }; index < count; ++index )
    {
        typedescription_t &field{ desc[ static_cast< usize >( existing ) + index ] };

        field                  = typedescription_t{ };
        field.fieldType        = fields[ index ].m_field.m_type;
        field.fieldName        = fields[ index ].m_id;
        field.fieldOffset      = fields[ index ].m_field.m_offset;
        field.fieldSize        = 1;
        field.flags            = fields[ index ].m_field.m_flags;
        field.fieldSizeInBytes = get_size( field.fieldType );
    }

    datamap->dataDesc      = desc;
    datamap->dataNumFields = existing + static_cast< i32 >( count );

    return true;
}

bool datamap_editor::init( )
{
    datamap_t *player_map{ g_csgo.m_C_CSPlayer_datamap };

    const named_field_t player_fields[]{
        { ATLAS_XORSTR_STATIC( "m_flVelocityModifier" ),
          { FIELD_FLOAT, g_netvars.get_offset( HASH_CT( "DT_CSPlayer" ), HASH_CT( "m_flVelocityModifier" ) ), FTYPEDESC_INSENDTABLE | FTYPEDESC_NOERRORCHECK } },
    };

    if ( !initialize_datamap( player_fields, sizeof( player_fields ) / sizeof( player_fields[ 0 ] ), player_map ) )
    {
        return false;
    }

    this->m_is_stale = player_map->m_pOptimizedDataMap != nullptr;

    return true;
}

void datamap_editor::on_check_init_predictable( )
{
    if ( !this->m_is_stale )
    {
        return;
    }

    this->m_is_stale = false;

    datamap_t *player_map{ g_csgo.m_C_CSPlayer_datamap };
    player_map->m_pOptimizedDataMap = nullptr;
    player_map->m_nPackedSize       = 0;

    IClientEntity *local{ g_csgo.m_entity_list->GetClientEntity( g_csgo.m_engine->GetLocalPlayer( ) ) };
    if ( local && local->As< C_BaseEntity * >( )->m_pOriginalData( ) )
    {
        local->As< C_BaseEntity * >( )->ShutdownPredictable( );
    }
}
