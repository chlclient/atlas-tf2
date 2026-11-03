#pragma once

#include "link.hpp"

struct data_field_t
{
    fieldtype_t m_type{ };
    i32         m_offset{ };
    short       m_flags{ };
};

struct named_field_t
{
    const char  *m_id{ };
    data_field_t m_field{ };
};

class datamap_editor
{
public:
    static bool init( );

    static i32 packed_offset( const datamap_t *map, hash_t name );

    template< typename T, typename F >
    static void patch_predicted( C_BaseEntity *entity, T &field, hash_t name, F apply )
    {
        apply( field );

        const datamap_t *map{ entity->GetPredDescMap( ) };
        const i32        offset{ packed_offset( map, name ) };
        u8              *original{ entity->m_pOriginalData( ) };

        if ( !map || !original || offset < 0 || offset + static_cast< i32 >( sizeof( T ) ) > map->m_nPackedSize )
        {
            return;
        }

        const auto patch_slot{ [ & ]( u8 *slot ) {
            T value{ };
            atl::memcpy( &value, slot + offset, sizeof( T ) );
            apply( value );
            atl::memcpy( slot + offset, &value, sizeof( T ) );
        } };

        patch_slot( original );

        u8 **slots{ entity->m_pIntermediateData( ) };
        for ( usize index{ }; index < MULTIPLAYER_BACKUP; ++index )
        {
            if ( slots[ index ] )
            {
                patch_slot( slots[ index ] );
            }
        }
    }
};

extern datamap_editor g_datamap;
