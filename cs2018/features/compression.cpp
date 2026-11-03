#include "compression.hpp"

ATLAS_NO_DESTROY netvar_compression g_compression{ };

constexpr i32 UNMODELLED_ENCODINGS{ SPROP_NOSCALE | SPROP_NORMAL | SPROP_COORD_MP | SPROP_COORD_MP_LOWPRECISION | SPROP_COORD_MP_INTEGRAL |
                                    SPROP_CELL_COORD | SPROP_CELL_COORD_LOWPRECISION | SPROP_CELL_COORD_INTEGRAL };

ATLAS_FORCE_INLINE i32 round_to_code( float value )
{
    return static_cast< i32 >( value < 0.0f ? value - 0.5f : value + 0.5f );
}

ATLAS_FORCE_INLINE usize component_count( SendPropType type )
{
    switch ( type )
    {
        case DPT_Vector: return 3;
        case DPT_VectorXY: return 2;
        default: return 1;
    }
}

ATLAS_FORCE_INLINE bool overflows( float multiplier, double range, u32 top )
{
    return static_cast< double >( multiplier ) * range > static_cast< double >( top );
}

ATLAS_FORCE_INLINE float range_multiplier( i32 bits, float range )
{
    constexpr float SQUEEZES[]{ 0.9999f, 0.99f, 0.9f, 0.8f, 0.7f };

    const u32    top{ ( 1u << static_cast< u32 >( bits ) ) - 1u };
    const double wide{ range };

    float multiplier{ static_cast< float >( static_cast< double >( top ) / wide ) };

    if ( wide <= 0.001 )
    {
        multiplier = static_cast< float >( top );
    }

    if ( !overflows( multiplier, wide, top ) )
    {
        return multiplier;
    }

    for ( const float squeeze : SQUEEZES )
    {
        multiplier = static_cast< float >( static_cast< double >( top ) / wide ) * squeeze;
        if ( !overflows( multiplier, wide, top ) )
        {
            return multiplier;
        }
    }

    return 0.0f;
}

ATLAS_FORCE_INLINE quantiser_t build_quantiser( const SendProp &prop )
{
    quantiser_t out{ };

    if ( prop.m_Flags & UNMODELLED_ENCODINGS )
    {
        return out;
    }

    if ( prop.m_Flags & SPROP_COORD )
    {
        out.m_encoding = prop_encoding::coord;
        return out;
    }

    const float range{ prop.m_fHighValue - prop.m_fLowValue };
    if ( prop.m_nBits <= 0 ||
         prop.m_nBits >= 32 ||
         range <= 0.0f )
    {
        return out;
    }

    const float multiplier{ range_multiplier( prop.m_nBits, range ) };
    if ( multiplier <= 0.0f )
    {
        return out;
    }

    out.m_encoding     = prop_encoding::scaled;
    out.m_low          = prop.m_fLowValue;
    out.m_high         = prop.m_fHighValue;
    out.m_multiplier   = multiplier;
    out.m_decode_scale = static_cast< float >( ( 1u << static_cast< u32 >( prop.m_nBits ) ) - 1u ) / range;

    return out;
}

ATLAS_FORCE_INLINE const SendProp *matching_send_prop( const SendTable *send, const char *name )
{
    for ( i32 index{ }; index < send->m_nProps; ++index )
    {
        const SendProp &prop{ send->m_pProps[ index ] };
        if ( str::equals( prop.m_pVarName, name ) )
        {
            return &prop;
        }
    }

    return nullptr;
}

ATLAS_FORCE_INLINE bool is_predicted( const flattenedoffsets_t &networked, i32 offset )
{
    for ( i32 index{ }; index < networked.m_nSize; ++index )
    {
        const typedescription_t &field{ networked.m_Flattened[ index ] };
        const i32                start{ field.flatOffset[ TD_OFFSET_NORMAL ] };

        if ( offset >= start && offset < start + field.fieldSizeInBytes )
        {
            return true;
        }
    }

    return false;
}

static void gather_excludes( const SendTable *send, exclude_list_t &out )
{
    for ( i32 index{ }; index < send->m_nProps; ++index )
    {
        const SendProp &prop{ send->m_pProps[ index ] };

        if ( prop.m_Flags & SPROP_EXCLUDE )
        {
            if ( out.m_count == MAX_PROP_EXCLUDES )
            {
                DBGOUT( "compression: exclude list full at %u, the model keeps dead props", static_cast< u32 >( MAX_PROP_EXCLUDES ) );
                return;
            }

            out.m_items[ out.m_count++ ] = prop_exclude_t{ prop.m_pExcludeDTName, prop.m_pVarName };
            continue;
        }

        if ( prop.m_Type == DPT_DataTable && prop.m_pDataTable )
        {
            gather_excludes( prop.m_pDataTable, out );
        }
    }
}

bool exclude_list_t::contains( const char *table, const char *prop ) const
{
    for ( usize index{ }; index < this->m_count; ++index )
    {
        if ( str::equals( this->m_items[ index ].m_table, table ) && str::equals( this->m_items[ index ].m_prop, prop ) )
        {
            return true;
        }
    }

    return false;
}

i32 quantiser_t::encode( float value ) const
{
    if ( this->m_encoding == prop_encoding::coord )
    {
        const i32 magnitude{ static_cast< i32 >( math::abs( value ) * static_cast< float >( COORD_DENOMINATOR ) ) };
        return value < 0.0f ? -magnitude : magnitude;
    }

    const float product{ ( value - this->m_low ) * this->m_multiplier };
    i32         code{ static_cast< i32 >( product ) };
    const float fraction{ product - static_cast< float >( code ) };

    if ( fraction > 0.5f || ( fraction == 0.5f && ( code & 1 ) ) )
    {
        ++code;
    }

    return code;
}

i32 quantiser_t::invert( float decoded ) const
{
    if ( this->m_encoding == prop_encoding::coord )
    {
        return round_to_code( decoded * static_cast< float >( COORD_DENOMINATOR ) );
    }

    return round_to_code( ( decoded - this->m_low ) * this->m_decode_scale );
}

bool quantiser_t::holds( float candidate, i32 code ) const
{
    if ( this->m_encoding == prop_encoding::scaled && ( candidate < this->m_low || candidate > this->m_high ) )
    {
        return false;
    }

    return this->encode( candidate ) == code;
}

bool netvar_compression::init( )
{
    for ( ClientClass *iter{ g_csgo.m_client->GetAllClasses( ) }; iter; iter = iter->m_pNext )
    {
        if ( str::equals( iter->m_pNetworkName, ATLAS_XORSTR( "CCSPlayer" ).get( ) ) )
        {
            this->m_table = iter->m_pRecvTable;
            break;
        }
    }

    if ( !this->m_table )
    {
        return false;
    }

    const RecvProp &base{ this->m_table->m_pProps[ 0 ] };
    Assert( str::equals( base.m_pVarName, "baseclass" ) );

    this->m_static_proxy = base.m_DataTableProxyFn;
    return true;
}

void netvar_compression::collect( RecvTable *recv, SendTable *send, const exclude_list_t &excluded, const flattenedoffsets_t &networked,
                                  i32 base_offset )
{
    for ( i32 index{ }; index < recv->m_nProps; ++index )
    {
        const RecvProp &prop{ recv->m_pProps[ index ] };

        if ( excluded.contains( send->m_pNetTableName, prop.m_pVarName ) )
        {
            continue;
        }

        const SendProp *mirror{ matching_send_prop( send, prop.m_pVarName ) };
        if ( !mirror )
        {
            continue;
        }

        const i32 offset{ base_offset + prop.m_Offset };

        if ( prop.m_RecvType == DPT_DataTable )
        {
            if ( prop.m_DataTableProxyFn == this->m_static_proxy && mirror->m_pDataTable )
            {
                this->collect( prop.m_pDataTable, mirror->m_pDataTable, excluded, networked, offset );
            }

            continue;
        }

        if ( prop.m_RecvType != DPT_Float && prop.m_RecvType != DPT_Vector && prop.m_RecvType != DPT_VectorXY )
        {
            continue;
        }

        if ( !is_predicted( networked, offset ) )
        {
            continue;
        }

        const quantiser_t quantiser{ build_quantiser( *mirror ) };
        if ( quantiser.m_encoding == prop_encoding::none )
        {
            continue;
        }

        bool seen{ };
        for ( usize existing{ }; existing < this->m_count; ++existing )
        {
            if ( this->m_props[ existing ].m_offset == offset )
            {
                seen = true;
                break;
            }
        }

        if ( seen )
        {
            continue;
        }

        if ( this->m_count == MAX_COMPRESSED_PROPS )
        {
            DBGOUT( "compression: more than %u props, dropping the rest", static_cast< u32 >( MAX_COMPRESSED_PROPS ) );
            return;
        }

        compressed_prop_t &entry{ this->m_props[ this->m_count++ ] };
        entry.m_name      = prop.m_pVarName;
        entry.m_offset    = offset;
        entry.m_type      = prop.m_RecvType;
        entry.m_quantiser = quantiser;
    }
}

bool netvar_compression::ensure_built( )
{
    if ( this->m_built )
    {
        return true;
    }

    const optimized_datamap_t *flattened{ g_csgo.m_C_CSPlayer_datamap->m_pOptimizedDataMap };
    if ( !flattened )
    {
        return false;
    }

    Assert( this->m_table->m_pDecoder != nullptr );
    SendTable *send{ this->m_table->m_pDecoder->m_pClientSendTable };

    exclude_list_t excluded{ };
    gather_excludes( send, excluded );

    this->collect( this->m_table, send, excluded, flattened->m_Info[ PC_NETWORKED_ONLY ], 0 );
    this->m_built = true;

#if defined( ATLAS_DEV )
    DBGOUT( "compression: %u props modelled, %u excluded", static_cast< u32 >( this->m_count ), static_cast< u32 >( excluded.m_count ) );

    for ( usize index{ }; index < this->m_count; ++index )
    {
        const compressed_prop_t &prop{ this->m_props[ index ] };
        DBGOUT( "compression:   +0x%04x %s %s", prop.m_offset, prop.m_name, prop.m_quantiser.m_encoding == prop_encoding::coord ? "coord" : "scaled" );
    }
#endif // ATLAS_DEV

    return true;
}

void netvar_compression::clear( )
{
    this->m_count = 0;
    this->m_built = false;
    this->m_armed = false;

    for ( snapshot_t &snapshot : this->m_ring )
    {
        snapshot.m_valid = false;
    }

    for ( restored_t( &components )[ 3 ] : this->m_restored )
    {
        for ( restored_t &component : components )
        {
            component.m_valid = false;
        }
    }
}

void netvar_compression::on_run_simulation( C_BasePlayer *player, i32 command )
{
    if ( !this->ensure_built( ) )
    {
        return;
    }

    snapshot_t &snapshot{ this->m_ring[ command & COMPRESSION_RING_MASK ] };
    snapshot.m_command = command;
    snapshot.m_valid   = true;

    const u8 *base{ reinterpret_cast< const u8 * >( player ) };

    for ( usize prop{ }; prop < this->m_count; ++prop )
    {
        const compressed_prop_t &entry{ this->m_props[ prop ] };
        const float             *field{ reinterpret_cast< const float * >( base + entry.m_offset ) };

        for ( usize component{ }; component < component_count( entry.m_type ); ++component )
        {
            snapshot.m_values[ prop ][ component ] = field[ component ];
        }
    }
}

void netvar_compression::on_packet_start( i32 acked_command )
{
    this->m_acked = acked_command;
    this->m_armed = true;
}

void netvar_compression::restore( )
{
    if ( !this->m_armed )
    {
        return;
    }

    this->m_armed = false;

    const snapshot_t &snapshot{ this->m_ring[ this->m_acked & COMPRESSION_RING_MASK ] };
    if ( !snapshot.m_valid || snapshot.m_command != this->m_acked )
    {
        return;
    }

    IClientEntity *local{ g_csgo.m_entity_list->GetClientEntity( g_csgo.m_engine->GetLocalPlayer( ) ) };
    if ( !local )
    {
        return;
    }

    u8 *base{ reinterpret_cast< u8 * >( local->As< C_BasePlayer * >( ) ) };

    for ( usize prop{ }; prop < this->m_count; ++prop )
    {
        const compressed_prop_t &entry{ this->m_props[ prop ] };
        float                   *field{ reinterpret_cast< float * >( base + entry.m_offset ) };

        for ( usize component{ }; component < component_count( entry.m_type ); ++component )
        {
            const float decoded{ field[ component ] };
            const float candidate{ snapshot.m_values[ prop ][ component ] };
            if ( candidate == decoded )
            {
                continue;
            }

            restored_t &last{ this->m_restored[ prop ][ component ] };
            const i32   code{ last.m_valid && last.m_value == decoded ? last.m_code : entry.m_quantiser.invert( decoded ) };
            if ( !entry.m_quantiser.holds( candidate, code ) )
            {
                continue;
            }

            field[ component ] = candidate;
            last               = restored_t{ candidate, code, true };
        }
    }
}
