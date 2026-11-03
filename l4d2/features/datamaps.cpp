#include "datamaps.hpp"

ATLAS_NO_DESTROY datamap_editor g_datamap{ };

struct datamap_edge_t
{
    datamap_t *m_map{ };
    datamap_t *m_base{ };
};

static constexpr usize DESC_CAPACITY{ 32 };
static constexpr usize MAP_CAPACITY{ 64 };
static constexpr usize ENTITY_CAPACITY{ 64 };
static constexpr usize EDGE_CAPACITY{ 4096 };
static constexpr usize CLASS_NAME_CAPACITY{ 64 };
static constexpr i32   FIELD_CAPACITY{ 4096 };
static constexpr i32   CHAIN_DEPTH{ 32 };

static typedescription_t s_gun_desc[ DESC_CAPACITY ]{ };

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

static bool is_datamap_class_name( const char *name )
{
    if ( !g_l4d2.m_client_dll.contains( name, 2 ) || name[ 0 ] != 'C' )
    {
        return false;
    }

    for ( usize index{ }; index < CLASS_NAME_CAPACITY; ++index )
    {
        const char c{ name[ index ] };

        if ( c == '\0' )
        {
            return index > 1;
        }

        const bool word{ ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) || c == '_' };
        if ( !word || !g_l4d2.m_client_dll.contains( name + index + 1, 1 ) )
        {
            return false;
        }
    }

    return false;
}

static bool is_named_map( const datamap_t *map, const char *class_name, const char *first_field )
{
    if ( map->dataNumFields <= 0 )
    {
        return false;
    }

    if ( !g_l4d2.m_client_dll.contains( map->dataDesc, sizeof( typedescription_t ) * static_cast< usize >( map->dataNumFields ) ) )
    {
        return false;
    }

    if ( !str::equals( map->dataClassName, class_name ) )
    {
        return false;
    }

    const char *name{ map->dataDesc[ 0 ].fieldName };
    return g_l4d2.m_client_dll.contains( name, str::length( first_field ) + 1 ) && str::equals( name, first_field );
}

static bool chains_to( const datamap_t *map, const datamap_t *root )
{
    for ( i32 depth{ }; map && depth < CHAIN_DEPTH; map = map->baseMap, ++depth )
    {
        if ( map == root )
        {
            return true;
        }
    }

    return false;
}

static bool contains_map( datamap_t *const *maps, usize count, const datamap_t *map )
{
    for ( usize index{ }; index < count; ++index )
    {
        if ( maps[ index ] == map )
        {
            return true;
        }
    }

    return false;
}

static bool has_field( const datamap_t *map, i32 offset )
{
    for ( i32 index{ }; index < map->dataNumFields; ++index )
    {
        if ( map->dataDesc[ index ].fieldOffset == offset )
        {
            return true;
        }
    }

    return false;
}

static datamap_t *scan_data( const char *class_name, const char *first_field, datamap_edge_t *edges, usize &edge_count )
{
    const void *section{ };
    usize       size{ };

    if ( !g_l4d2.m_client_dll.section( ATLAS_XORSTR( ".data" ).get( ), &section, &size ) || size < sizeof( datamap_t ) )
    {
        return nullptr;
    }

    const u8 *begin{ static_cast< const u8 * >( section ) };
    const u8 *end{ begin + size };

    const auto in_data = [ begin, end ]( const void *pointer )
    {
        const u8 *bytes{ static_cast< const u8 * >( pointer ) };
        return bytes >= begin && bytes + sizeof( datamap_t ) <= end;
    };

    datamap_t *root{ };

    for ( const u8 *cursor{ begin }; cursor + sizeof( datamap_t ) <= end; cursor += sizeof( u32 ) )
    {
        datamap_t *map{ reinterpret_cast< datamap_t * >( const_cast< u8 * >( cursor ) ) };

        if ( map->dataNumFields < 0 || map->dataNumFields > FIELD_CAPACITY || !is_datamap_class_name( map->dataClassName ) )
        {
            continue;
        }

        if ( !root && is_named_map( map, class_name, first_field ) )
        {
            root = map;
        }

        if ( map->baseMap && in_data( map->baseMap ) && edge_count < EDGE_CAPACITY )
        {
            edges[ edge_count++ ] = { map, map->baseMap };
        }
    }

    return root;
}

static usize collect_maps( const char *class_name, const char *first_field, datamap_t **out )
{
    static datamap_edge_t edges[ EDGE_CAPACITY ]{ };
    usize                 edge_count{ };

    datamap_t *root{ scan_data( class_name, first_field, edges, edge_count ) };
    if ( !root )
    {
        return 0;
    }

    usize count{ };
    out[ count++ ] = root;

    for ( usize walked{ }; walked < count; ++walked )
    {
        for ( usize index{ }; index < edge_count; ++index )
        {
            if ( edges[ index ].m_base != out[ walked ] || contains_map( out, count, edges[ index ].m_map ) )
            {
                continue;
            }

            if ( count == MAP_CAPACITY )
            {
                DBGOUT( "collect_maps: more than %i maps chain to %s", static_cast< i32 >( MAP_CAPACITY ), class_name );
                return 0;
            }

            out[ count++ ] = edges[ index ].m_map;
        }
    }

    return count;
}

static bool collect_predictable( const datamap_t *root, C_BaseEntity **out, usize &count )
{
    const i32 highest{ g_l4d2.m_entity_list->GetHighestEntityIndex( ) };

    for ( i32 index{ 1 }; index <= highest; ++index )
    {
        IClientEntity *client_entity{ g_l4d2.m_entity_list->GetClientEntity( index ) };
        if ( !client_entity )
        {
            continue;
        }

        C_BaseEntity *entity{ client_entity->As< C_BaseEntity * >( ) };
        if ( !entity->m_pOriginalData( ) || !chains_to( entity->GetPredDescMap( ), root ) )
        {
            continue;
        }

        if ( count == ENTITY_CAPACITY )
        {
            return false;
        }

        out[ count++ ] = entity;
    }

    return true;
}

static bool reallocate_intermediate_data( C_BaseEntity *entity )
{
    datamap_t *map{ entity->GetPredDescMap( ) };
    if ( !map )
    {
        return false;
    }

    CPredictionCopy prepare{ PC_EVERYTHING, entity, PC_DATA_NORMAL, entity, PC_DATA_NORMAL };
    prepare.TransferData( ATLAS_XORSTR( "ATL_DATAMAP_PREPARE" ).get( ), entity->entindex( ), map );

    const usize needed{ static_cast< usize >( math::bigger( map->m_nPackedSize, 4 ) ) };

    u8 *&original{ entity->m_pOriginalData( ) };
    u8 **slots{ entity->m_pIntermediateData( ) };

    g_l4d2.m_alloc->Free( original );
    original = nullptr;

    for ( usize slot{ }; slot < MULTIPLAYER_BACKUP; ++slot )
    {
        g_l4d2.m_alloc->Free( slots[ slot ] );
        slots[ slot ] = nullptr;
    }

    const auto fresh = [ entity, map, needed ]( ) -> u8 *
    {
        u8 *block{ static_cast< u8 * >( g_l4d2.m_alloc->Alloc( needed ) ) };
        if ( !block )
        {
            return nullptr;
        }

        atl::memset( block, 0, needed );

        CPredictionCopy copy{ PC_EVERYTHING, block, PC_DATA_PACKED, entity, PC_DATA_NORMAL };
        copy.TransferData( ATLAS_XORSTR( "ATL_DATAMAP_SEED" ).get( ), entity->entindex( ), map );
        return block;
    };

    original = fresh( );
    for ( usize slot{ }; slot < MULTIPLAYER_BACKUP; ++slot )
    {
        slots[ slot ] = fresh( );
    }

    return original != nullptr;
}

static void splice_fields( datamap_t *root, const named_field_t *fields, usize count, typedescription_t *storage )
{
    const i32 existing{ root->dataNumFields };

    for ( i32 index{ }; index < existing; ++index )
    {
        storage[ index ] = root->dataDesc[ index ];
    }

    for ( usize index{ }; index < count; ++index )
    {
        const named_field_t &entry{ fields[ index ] };
        typedescription_t   &field{ storage[ static_cast< usize >( existing ) + index ] };

        field                  = typedescription_t{ };
        field.fieldType        = entry.m_field.m_type;
        field.fieldName        = entry.m_id;
        field.fieldOffset      = entry.m_field.m_offset;
        field.fieldSize        = 1;
        field.flags            = entry.m_field.m_flags;
        field.fieldSizeInBytes = get_size( entry.m_field.m_type );
    }

    root->dataDesc      = storage;
    root->dataNumFields = existing + static_cast< i32 >( count );
}

#if defined( ATLAS_DEV )
static void dump_datamap( const datamap_t *map, i32 previous )
{
    DBGOUT( "Datamap: 0x%x, %s", reinterpret_cast< uptr >( map ), map->dataClassName );
    DBGOUT( "dataNumFields: %i, old: %i", map->dataNumFields, previous );

    for ( i32 index{ }; index < map->dataNumFields; ++index )
    {
        const typedescription_t &field{ map->dataDesc[ index ] };
        DBGOUT( "%s %i %i %i %i", field.fieldName, static_cast< i32 >( field.fieldType ), field.fieldOffset, field.fieldSize, field.fieldSizeInBytes );
    }
}
#endif

static bool initialize_datamap( const named_field_t *fields, usize count, datamap_t *const *maps, usize map_count,
                                typedescription_t *storage, usize capacity )
{
    datamap_t *root{ maps[ 0 ] };
    const i32  existing{ root->dataNumFields };

    if ( static_cast< usize >( existing ) + count > capacity )
    {
        DBGOUT( "initialize_datamap: %i descriptors do not fit in %i", existing + static_cast< i32 >( count ), static_cast< i32 >( capacity ) );
        return false;
    }

    for ( usize index{ }; index < count; ++index )
    {
        if ( fields[ index ].m_field.m_offset <= 0 || has_field( root, fields[ index ].m_field.m_offset ) )
        {
            DBGOUT( "initialize_datamap: %s has a bad or duplicate offset %i", fields[ index ].m_id, fields[ index ].m_field.m_offset );
            return false;
        }
    }

    C_BaseEntity *entities[ ENTITY_CAPACITY ]{ };
    usize         entity_count{ };

    if ( !collect_predictable( root, entities, entity_count ) )
    {
        DBGOUT( "initialize_datamap: more than %i predictable entities", static_cast< i32 >( ENTITY_CAPACITY ) );
        return false;
    }

    splice_fields( root, fields, count, storage );

    for ( usize index{ }; index < map_count; ++index )
    {
        maps[ index ]->m_pOptimizedDataMap = nullptr;
        maps[ index ]->m_nPackedSize       = 0;
    }

    for ( usize index{ }; index < entity_count; ++index )
    {
        reallocate_intermediate_data( entities[ index ] );
    }

#if defined( ATLAS_DEV )
    dump_datamap( root, existing );
    DBGOUT( "%i maps reset, %i entities reallocated", static_cast< i32 >( map_count ), static_cast< i32 >( entity_count ) );
#endif

    return true;
}

i32 datamap_editor::packed_offset( const datamap_t *map, hash_t name )
{
    if ( !map || !map->m_pOptimizedDataMap )
    {
        return -1;
    }

    for ( const flattened_chain_t &chain : map->m_pOptimizedDataMap->m_chains )
    {
        for ( i32 index{ }; index < chain.m_count; ++index )
        {
            const typedescription_t &field{ chain.m_fields[ index ] };
            if ( field.fieldName && HASH_RT( field.fieldName ) == name )
            {
                return field.flatOffset[ TD_OFFSET_PACKED ];
            }
        }
    }

    return -1;
}

bool datamap_editor::init( )
{
    const i32 upgrades{ g_netvars.get_offset( HASH_CT( "DT_TerrorGun" ), HASH_CT( "m_upgradeBitVec" ) ) };
    if ( upgrades == 0 )
    {
        DBGOUT( "datamap_editor::init: DT_TerrorGun::m_upgradeBitVec has no offset" );
        return false;
    }

    const named_field_t gun_fields[]{
        { ATLAS_XORSTR_STATIC( "m_flSpread" ), { FIELD_FLOAT, upgrades + 0x1C, FTYPEDESC_NOERRORCHECK } },
    };

    datamap_t  *maps[ MAP_CAPACITY ]{ };
    const usize map_count{ collect_maps( ATLAS_XORSTR( "CTerrorGun" ).get( ), ATLAS_XORSTR( "m_isDualWielding" ).get( ), maps ) };

    if ( map_count == 0 )
    {
        DBGOUT( "datamap_editor::init: no CTerrorGun prediction map found in client.dll .data" );
        return false;
    }

    if ( has_field( maps[ 0 ], gun_fields[ 0 ].m_field.m_offset ) )
    {
        DBGOUT( "datamap_editor::init: CTerrorGun already predicts offset %i", gun_fields[ 0 ].m_field.m_offset );
        return true;
    }

    return initialize_datamap( gun_fields, sizeof( gun_fields ) / sizeof( gun_fields[ 0 ] ), maps, map_count,
                               s_gun_desc, DESC_CAPACITY );
}
