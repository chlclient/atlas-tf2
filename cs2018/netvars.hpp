#pragma once

#include "sdk.hpp"

inline constexpr usize VARS_CAPACITY{ 8192 };
inline constexpr u32   VARS_MASK{ static_cast< u32 >( VARS_CAPACITY - 1 ) };

// (table_hash ^ prop_hash) -> offset
class netvars
{
public:
    bool load( );

    i32 get_offset( hash_t table, hash_t prop ) const;

    RecvProp *get_prop( hash_t table, hash_t prop ) const;

private:
    struct entry
    {
        hash_t          m_key{ };
        i32             m_offset{ };
        const RecvProp *m_prop{ };
    };

    void dump_recv_table( RecvTable *table, hash_t table_hash, i32 base_offset );
    void insert( hash_t key, i32 offset, const RecvProp *prop );

    const entry *find( hash_t key ) const;

    entry m_entries[ VARS_CAPACITY ]{ };
    usize m_count{ };
    bool  m_overflow{ };
};

extern netvars g_netvars;

#define NETVAR( type, name, table, prop )                                                    \
    type &name( )                                                                            \
    {                                                                                        \
        static const i32 s_off{ g_netvars.get_offset( HASH_CT( table ), HASH_CT( prop ) ) }; \
        return *reinterpret_cast< type * >( reinterpret_cast< u8 * >( this ) + s_off );      \
    }

#define NETVAR_PTR( type, name, table, prop )                                                \
    type *name( )                                                                            \
    {                                                                                        \
        static const i32 s_off{ g_netvars.get_offset( HASH_CT( table ), HASH_CT( prop ) ) }; \
        return reinterpret_cast< type * >( reinterpret_cast< u8 * >( this ) + s_off );       \
    }

#define CLASS_VAR( type, name, offset )                                                  \
    type &name( )                                                                        \
    {                                                                                    \
        return *reinterpret_cast< type * >( reinterpret_cast< u8 * >( this ) + offset ); \
    }
