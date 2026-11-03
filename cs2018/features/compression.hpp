#pragma once

#include "link.hpp"

inline constexpr usize MAX_COMPRESSED_PROPS{ 32 };
inline constexpr usize MAX_PROP_EXCLUDES{ 96 };

inline constexpr usize COMPRESSION_RING_SIZE{ 64 };
inline constexpr i32   COMPRESSION_RING_MASK{ static_cast< i32 >( COMPRESSION_RING_SIZE ) - 1 };

enum class prop_encoding : u8
{
    none = 0,
    coord,
    scaled,
};

struct quantiser_t
{
    prop_encoding m_encoding{ prop_encoding::none };

    float m_low{ };
    float m_high{ };
    float m_multiplier{ };
    float m_decode_scale{ };

    i32  encode( float value ) const;
    i32  invert( float decoded ) const;
    bool holds( float candidate, i32 code ) const;
};

struct compressed_prop_t
{
    const char  *m_name{ };
    i32          m_offset{ };
    SendPropType m_type{ };
    quantiser_t  m_quantiser{ };
};

struct prop_exclude_t
{
    const char *m_table{ };
    const char *m_prop{ };
};

struct exclude_list_t
{
    prop_exclude_t m_items[ MAX_PROP_EXCLUDES ]{ };
    usize          m_count{ };

    bool contains( const char *table, const char *prop ) const;
};

class netvar_compression
{
public:
    bool init( );
    void on_run_simulation( C_BasePlayer *player, i32 command );
    void on_packet_start( i32 acked_command );
    void restore( );
    void clear( );

private:
    struct snapshot_t
    {
        i32  m_command{ };
        bool m_valid{ };
        vec3 m_values[ MAX_COMPRESSED_PROPS ]{ };
    };

    struct restored_t
    {
        float m_value{ };
        i32   m_code{ };
        bool  m_valid{ };
    };

    bool ensure_built( );
    void collect( RecvTable *recv, SendTable *send, const exclude_list_t &excluded, const flattenedoffsets_t &networked,
                  i32 base_offset );

    RecvTable              *m_table{ };
    DataTableRecvVarProxyFn m_static_proxy{ };

    compressed_prop_t m_props[ MAX_COMPRESSED_PROPS ]{ };
    usize             m_count{ };
    bool              m_built{ };

    snapshot_t m_ring[ COMPRESSION_RING_SIZE ]{ };
    restored_t m_restored[ MAX_COMPRESSED_PROPS ][ 3 ]{ };

    i32  m_acked{ };
    bool m_armed{ };
};

extern netvar_compression g_compression;
