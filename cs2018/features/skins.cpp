#include "skins.hpp"

#include "gui/gui.hpp"
#include "runtime/algorithm.hpp"

ATLAS_NO_DESTROY skins g_skins{ };

constexpr usize GAME_FILE_MAX{ 32u * 1024u * 1024u };
constexpr usize LABEL_MAX{ 64 };
constexpr usize PREFAB_DEPTH_MAX{ 8 };
constexpr usize SEQUENCES_MAX{ 32 };

constexpr usize DRESSING_ATTRIBUTES{ 5 };

struct kv_span_t
{
    const char *m_data{ };
    usize       m_size{ };
};

struct prefab_t
{
    hash_t    m_name{ };
    kv_span_t m_body{ };
};

struct sequence_t
{
    hash_t m_label{ };
    hash_t m_activity{ };
    i32    m_weight{ };
};

struct item_t
{
    i32   m_definition{ };
    usize m_label{ str::npos };
    usize m_token{ str::npos };
    usize m_model_player{ str::npos };
    usize m_model_world{ str::npos };
    usize m_death_name{ str::npos };
    usize m_first_sequence{ };
    usize m_sequence_count{ };
    usize m_first_kit{ };
    usize m_kit_count{ };
};

struct kit_t
{
    i32   m_id{ };
    usize m_label{ str::npos };
    usize m_token{ str::npos };
    usize m_name{ str::npos };
};

struct pending_t
{
    hash_t m_hash{ };
    usize *m_label{ };
};

ATLAS_NO_DESTROY static atl::vector< char >       s_text{ };
ATLAS_NO_DESTROY static atl::vector< prefab_t >   s_prefabs{ };
ATLAS_NO_DESTROY static atl::vector< item_t >     s_weapons{ };
ATLAS_NO_DESTROY static atl::vector< item_t >     s_knives{ };
ATLAS_NO_DESTROY static atl::vector< item_t >     s_stock_knives{ };
ATLAS_NO_DESTROY static atl::vector< item_t >     s_gloves{ };
ATLAS_NO_DESTROY static atl::vector< kit_t >      s_weapon_kits{ };
ATLAS_NO_DESTROY static atl::vector< kit_t >      s_glove_kits{ };
ATLAS_NO_DESTROY static atl::vector< sequence_t > s_sequences{ };
ATLAS_NO_DESTROY static atl::vector< u32 >        s_pairs{ };
ATLAS_NO_DESTROY static atl::vector< usize >      s_item_kits{ };

static u16 s_attributes[ DRESSING_ATTRIBUTES ]{ };
static i32 s_quality_unusual{ -1 };
static i32 s_quality_strange{ -1 };

static ClientClass   *s_wearable_class{ };
static RecvVarProxyFn s_sequence_proxy{ };
static u32            s_random{ 0x9E3779B9u };

static u32 next_random( )
{
    s_random ^= s_random << 13;
    s_random ^= s_random >> 17;
    s_random ^= s_random << 5;
    return s_random;
}

static const char *text( usize offset )
{
    return s_text.data( ) + offset;
}

static usize store( const char *data, usize size )
{
    const usize offset{ s_text.size( ) };

    for ( usize index{ }; index < size; ++index )
    {
        s_text.push_back( data[ index ] );
    }

    s_text.push_back( '\0' );
    return offset;
}

static char *read_game_file( const char *relative, usize &size )
{
    FileHandle_t file{ g_csgo.m_filesystem->Open( relative, ATLAS_XORSTR( "rb" ).get( ), ATLAS_XORSTR( "GAME" ).get( ) ) };

    if ( !file )
    {
        return nullptr;
    }

    const u32 bytes{ g_csgo.m_filesystem->Size( file ) };

    if ( !bytes || bytes > GAME_FILE_MAX )
    {
        g_csgo.m_filesystem->Close( file );
        return nullptr;
    }

    auto *data{ static_cast< char * >( atl::malloc( bytes ) ) };

    if ( !data )
    {
        g_csgo.m_filesystem->Close( file );
        return nullptr;
    }

    const i32 read{ g_csgo.m_filesystem->Read( data, static_cast< i32 >( bytes ), file ) };

    g_csgo.m_filesystem->Close( file );

    if ( read <= 0 )
    {
        atl::free( data );
        return nullptr;
    }

    size = static_cast< usize >( read );
    return data;
}

static bool is_layout( char value )
{
    return str::is_space( value ) || value == '\r' || value == '\n';
}

static usize token_end( const char *data, usize size, usize at )
{
    usize end{ at + 1 };

    while ( end < size && data[ end ] != '"' )
    {
        ++end;
    }

    return end;
}

static bool next_pair( kv_span_t span, usize &from, kv_span_t &key, kv_span_t &value, bool &block )
{
    for ( usize at{ from }; at < span.m_size; ++at )
    {
        if ( span.m_data[ at ] == '/' && at + 1 < span.m_size && span.m_data[ at + 1 ] == '/' )
        {
            while ( at < span.m_size && span.m_data[ at ] != '\n' )
            {
                ++at;
            }

            continue;
        }

        if ( span.m_data[ at ] != '"' )
        {
            continue;
        }

        const usize end{ token_end( span.m_data, span.m_size, at ) };
        usize       next{ end + 1 };

        while ( next < span.m_size && is_layout( span.m_data[ next ] ) )
        {
            ++next;
        }

        if ( next >= span.m_size )
        {
            return false;
        }

        key = kv_span_t{ span.m_data + at + 1, end - at - 1 };

        if ( span.m_data[ next ] == '{' )
        {
            usize depth{ };
            usize close{ next };

            while ( close < span.m_size )
            {
                if ( span.m_data[ close ] == '{' )
                {
                    ++depth;
                } else if ( span.m_data[ close ] == '}' && --depth == 0 )
                {
                    break;
                }

                ++close;
            }

            if ( close >= span.m_size )
            {
                return false;
            }

            value = kv_span_t{ span.m_data + next + 1, close - next - 1 };
            block = true;
            from  = close + 1;
            return true;
        }

        if ( span.m_data[ next ] != '"' )
        {
            at = end;
            continue;
        }

        const usize value_end{ token_end( span.m_data, span.m_size, next ) };

        value = kv_span_t{ span.m_data + next + 1, value_end - next - 1 };
        block = false;
        from  = value_end + 1;
        return true;
    }

    return false;
}

static bool span_equals( kv_span_t span, const char *value )
{
    const usize length{ str::length( value ) };
    return span.m_size == length && str::equals( span.m_data, length, value, length );
}

static hash_t span_hash( kv_span_t span )
{
    return hash::make_hash( span.m_data, span.m_size, true );
}

static i32 span_int( kv_span_t span )
{
    i32 value{ };

    for ( usize index{ }; index < span.m_size && str::is_digit( span.m_data[ index ] ); ++index )
    {
        value = value * 10 + ( span.m_data[ index ] - '0' );
    }

    return value;
}

static bool find_key( kv_span_t span, const char *key, kv_span_t &value, bool &block )
{
    usize     from{ };
    kv_span_t name{ };

    while ( next_pair( span, from, name, value, block ) )
    {
        if ( span_equals( name, key ) )
        {
            return true;
        }
    }

    return false;
}

static bool find_value( kv_span_t span, const char *key, kv_span_t &value )
{
    bool block{ };
    return find_key( span, key, value, block ) && !block;
}

static const prefab_t *find_prefab( kv_span_t name )
{
    const hash_t wanted{ span_hash( name ) };

    for ( const prefab_t &prefab : s_prefabs )
    {
        if ( prefab.m_name == wanted )
        {
            return &prefab;
        }
    }

    return nullptr;
}

static bool resolve_path( kv_span_t span, const char *const *path, usize count, kv_span_t &value )
{
    for ( usize step{ }; step < count; ++step )
    {
        bool block{ };

        if ( !find_key( span, path[ step ], value, block ) || block != ( step + 1 < count ) )
        {
            return false;
        }

        span = value;
    }

    return true;
}

static bool resolve( kv_span_t definition, const char *const *path, usize count, kv_span_t &value )
{
    for ( usize depth{ }; depth < PREFAB_DEPTH_MAX; ++depth )
    {
        if ( resolve_path( definition, path, count, value ) )
        {
            return true;
        }

        kv_span_t prefab_name{ };
        if ( !find_value( definition, ATLAS_XORSTR( "prefab" ).get( ), prefab_name ) )
        {
            return false;
        }

        usize first{ };
        while ( first < prefab_name.m_size && prefab_name.m_data[ first ] != ' ' )
        {
            ++first;
        }

        const prefab_t *prefab{ find_prefab( kv_span_t{ prefab_name.m_data, first } ) };
        if ( !prefab )
        {
            return false;
        }

        definition = prefab->m_body;
    }

    return false;
}

static bool resolve_field( kv_span_t definition, const char *key, kv_span_t &value )
{
    const char *const path[]{ key };
    return resolve( definition, path, 1, value );
}

static usize store_field( kv_span_t definition, const char *key )
{
    kv_span_t value{ };
    return resolve_field( definition, key, value ) && value.m_size ? store( value.m_data, value.m_size ) : str::npos;
}

static usize store_token( kv_span_t definition, const char *key )
{
    kv_span_t value{ };

    if ( !resolve_field( definition, key, value ) || value.m_size < 2 || value.m_data[ 0 ] != '#' )
    {
        return str::npos;
    }

    return store( value.m_data + 1, value.m_size - 1 );
}

static void parse_item( kv_span_t key, kv_span_t body )
{
    kv_span_t   slot{ };
    kv_span_t   paintable{ };
    kv_span_t   name{ };
    const char *capabilities[]{ ATLAS_XORSTR_STATIC( "capabilities" ), ATLAS_XORSTR_STATIC( "paintable" ) };

    if ( !resolve_field( body, ATLAS_XORSTR( "item_slot" ).get( ), slot ) )
    {
        return;
    }

    const bool is_paintable{ resolve( body, capabilities, 2, paintable ) && span_equals( paintable, ATLAS_XORSTR( "1" ).get( ) ) };
    const bool is_named{ find_value( body, ATLAS_XORSTR( "name" ).get( ), name ) &&
                         str::starts_with( name.m_data, name.m_size, ATLAS_XORSTR( "weapon_" ).get( ), 7 ) };
    const bool is_melee{ span_equals( slot, ATLAS_XORSTR( "melee" ).get( ) ) };
    const bool is_glove{ span_equals( slot, ATLAS_XORSTR( "clothing" ).get( ) ) && is_paintable };
    const bool is_weapon{ is_paintable && !is_melee && is_named };

    if ( !is_melee && !is_glove && !is_weapon )
    {
        return;
    }

    item_t item{ };
    item.m_definition   = span_int( key );
    item.m_token        = store_token( body, ATLAS_XORSTR( "item_name" ).get( ) );
    item.m_model_player = store_field( body, ATLAS_XORSTR( "model_player" ).get( ) );
    item.m_model_world  = store_field( body, ATLAS_XORSTR( "model_world" ).get( ) );

    if ( is_melee )
    {
        if ( is_named )
        {
            item.m_death_name = store( name.m_data + 7, name.m_size - 7 );
        }

        ( is_paintable ? s_knives : s_stock_knives ).push_back( item );
    } else
    {
        ( is_glove ? s_gloves : s_weapons ).push_back( item );
    }
}

static void parse_kit( kv_span_t key, kv_span_t body )
{
    kv_span_t value{ };
    kit_t     kit{ };

    kit.m_id    = span_int( key );
    kit.m_token = store_token( body, ATLAS_XORSTR( "description_tag" ).get( ) );
    kit.m_name  = store_field( body, ATLAS_XORSTR( "name" ).get( ) );

    if ( !kit.m_id || kit.m_token == str::npos || kit.m_name == str::npos )
    {
        return;
    }

    ( find_value( body, ATLAS_XORSTR( "vmt_path" ).get( ), value ) ? s_glove_kits : s_weapon_kits ).push_back( kit );
}

static void parse_attribute( kv_span_t key, kv_span_t body )
{
    static const char *const NAMES[ DRESSING_ATTRIBUTES ]{
        ATLAS_XORSTR_STATIC( "set item texture prefab" ), ATLAS_XORSTR_STATIC( "set item texture seed" ),
        ATLAS_XORSTR_STATIC( "set item texture wear" ), ATLAS_XORSTR_STATIC( "kill eater" ),
        ATLAS_XORSTR_STATIC( "kill eater score type" )
    };

    kv_span_t name{ };
    if ( !find_value( body, ATLAS_XORSTR( "name" ).get( ), name ) )
    {
        return;
    }

    for ( usize index{ }; index < DRESSING_ATTRIBUTES; ++index )
    {
        if ( span_equals( name, NAMES[ index ] ) )
        {
            s_attributes[ index ] = static_cast< u16 >( span_int( key ) );
        }
    }
}

static void parse_quality( kv_span_t key, kv_span_t body )
{
    kv_span_t value{ };
    if ( !find_value( body, ATLAS_XORSTR( "value" ).get( ), value ) )
    {
        return;
    }

    if ( span_equals( key, ATLAS_XORSTR( "unusual" ).get( ) ) )
    {
        s_quality_unusual = span_int( value );
    } else if ( span_equals( key, ATLAS_XORSTR( "strange" ).get( ) ) )
    {
        s_quality_strange = span_int( value );
    }
}

static void parse_icons( kv_span_t key, kv_span_t body )
{
    if ( !span_equals( key, ATLAS_XORSTR( "weapon_icons" ).get( ) ) )
    {
        return;
    }

    usize     from{ };
    kv_span_t icon{ };
    kv_span_t fields{ };
    bool      block{ };

    while ( next_pair( body, from, icon, fields, block ) )
    {
        s_pairs.push_back( static_cast< u32 >( span_int( icon ) ) & ~3u );
    }
}

static void parse_items_game( const char *data, usize size )
{
    kv_span_t root{ };
    kv_span_t key{ };
    bool      block{ };
    usize     from{ };

    if ( !next_pair( kv_span_t{ data, size }, from, key, root, block ) || !block )
    {
        return;
    }

    kv_span_t section{ };
    from = 0;

    while ( next_pair( root, from, key, section, block ) )
    {
        if ( block && span_equals( key, ATLAS_XORSTR( "prefabs" ).get( ) ) )
        {
            usize     inner{ };
            kv_span_t name{ };
            kv_span_t body{ };

            while ( next_pair( section, inner, name, body, block ) )
            {
                if ( block )
                {
                    s_prefabs.push_back( prefab_t{ span_hash( name ), body } );
                }
            }
        }
    }

    from = 0;

    while ( next_pair( root, from, key, section, block ) )
    {
        if ( !block )
        {
            continue;
        }

        void ( *parse )( kv_span_t, kv_span_t ){ };

        if ( span_equals( key, ATLAS_XORSTR( "items" ).get( ) ) )
        {
            parse = &parse_item;
        } else if ( span_equals( key, ATLAS_XORSTR( "paint_kits" ).get( ) ) )
        {
            parse = &parse_kit;
        } else if ( span_equals( key, ATLAS_XORSTR( "attributes" ).get( ) ) )
        {
            parse = &parse_attribute;
        } else if ( span_equals( key, ATLAS_XORSTR( "qualities" ).get( ) ) )
        {
            parse = &parse_quality;
        } else if ( span_equals( key, ATLAS_XORSTR( "alternate_icons2" ).get( ) ) )
        {
            parse = &parse_icons;
        } else
        {
            continue;
        }

        usize     inner{ };
        kv_span_t name{ };
        kv_span_t body{ };

        while ( next_pair( section, inner, name, body, block ) )
        {
            if ( block )
            {
                parse( name, body );
            }
        }
    }
}

static usize narrow( const wchar_t *value, usize length, char *out, usize capacity )
{
    usize written{ };

    for ( usize index{ }; index < length; ++index )
    {
        u32 code{ value[ index ] };

        if ( code >= 0xD800 && code <= 0xDFFF )
        {
            continue;
        }

        if ( code == L'\\' && index + 1 < length )
        {
            code = value[ ++index ];
        }

        const usize needed{ code < 0x80 ? 1u : ( code < 0x800 ? 2u : 3u ) };

        if ( written + needed >= capacity )
        {
            break;
        }

        if ( needed == 1 )
        {
            out[ written++ ] = static_cast< char >( code );
        } else if ( needed == 2 )
        {
            out[ written++ ] = static_cast< char >( 0xC0 | ( code >> 6 ) );
            out[ written++ ] = static_cast< char >( 0x80 | ( code & 0x3F ) );
        } else
        {
            out[ written++ ] = static_cast< char >( 0xE0 | ( code >> 12 ) );
            out[ written++ ] = static_cast< char >( 0x80 | ( ( code >> 6 ) & 0x3F ) );
            out[ written++ ] = static_cast< char >( 0x80 | ( code & 0x3F ) );
        }
    }

    out[ written ] = '\0';
    return written;
}

static void add_pending( atl::vector< pending_t > &pending, usize token, usize *label )
{
    if ( token != str::npos )
    {
        pending.push_back( pending_t{ HASH_RT( text( token ) ), label } );
    }
}

static void localize( const wchar_t *data, usize length, atl::vector< pending_t > &pending )
{
    atl::sort( pending, []( const pending_t &left, const pending_t &right )
    { return left.m_hash < right.m_hash; } );

    for ( usize at{ }; at < length; ++at )
    {
        if ( data[ at ] != L'"' )
        {
            continue;
        }

        usize key_end{ at + 1 };
        while ( key_end < length && data[ key_end ] != L'"' && data[ key_end ] != L'\n' )
        {
            ++key_end;
        }

        usize value{ key_end + 1 };
        while ( value < length && ( data[ value ] == L' ' || data[ value ] == L'\t' ) )
        {
            ++value;
        }

        if ( key_end >= length || data[ key_end ] != L'"' || value >= length || data[ value ] != L'"' )
        {
            at = key_end;
            continue;
        }

        usize value_end{ value + 1 };
        while ( value_end < length && data[ value_end ] != L'"' && data[ value_end ] != L'\n' )
        {
            value_end += data[ value_end ] == L'\\' ? 2 : 1;
        }

        const hash_t key_hash{ hash::make_hash( data + at + 1, key_end - at - 1, true ) };
        usize        low{ };
        usize        high{ pending.size( ) };

        while ( low < high )
        {
            const usize middle{ ( low + high ) / 2 };

            if ( pending[ middle ].m_hash < key_hash )
            {
                low = middle + 1;
            } else
            {
                high = middle;
            }
        }

        usize label{ str::npos };

        for ( ; low < pending.size( ) && pending[ low ].m_hash == key_hash; ++low )
        {
            if ( *pending[ low ].m_label != str::npos )
            {
                continue;
            }

            if ( label == str::npos )
            {
                char        name[ LABEL_MAX ]{ };
                const usize written{ narrow( data + value + 1, math::smaller( value_end, length ) - value - 1, name, sizeof( name ) ) };
                label = store( name, written );
            }

            *pending[ low ].m_label = label;
        }

        at = value_end;
    }
}

static const char *studio_string( const char *data, usize size, usize offset )
{
    return offset < size && str::bounded_length( data + offset, size - offset ) < size - offset ? data + offset : nullptr;
}

static i32 studio_int( const char *data, usize offset )
{
    i32 value{ };
    atl::memcpy( &value, data + offset, sizeof( value ) );
    return value;
}

static bool studio_header( const char *data, usize size )
{
    return size >= STUDIO_HEADER_SIZE && studio_int( data, 0 ) == STUDIO_ID && studio_int( data, 4 ) == STUDIO_VERSION;
}

static void read_sequences( item_t &item )
{
    usize size{ };
    char *model{ item.m_model_player != str::npos ? read_game_file( text( item.m_model_player ), size ) : nullptr };

    item.m_first_sequence = s_sequences.size( );

    if ( !model )
    {
        return;
    }

    char        include[ 260 ]{ };
    const usize groups{ static_cast< usize >( studio_int( model, STUDIO_NUMINCLUDEMODELS + 4 ) ) };

    if ( studio_header( model, size ) && studio_int( model, STUDIO_NUMLOCALSEQ ) == 0 &&
         studio_int( model, STUDIO_NUMINCLUDEMODELS ) == 1 && groups + 8 <= size )
    {
        const char *path{ studio_string( model, size, groups + static_cast< usize >( studio_int( model, groups + 4 ) ) ) };

        if ( path )
        {
            str::copy( include, sizeof( include ), path );
        }
    }

    atl::free( model );

    char *animations{ include[ 0 ] ? read_game_file( include, size ) : nullptr };

    if ( !animations )
    {
        return;
    }

    const i32   count{ studio_header( animations, size ) ? studio_int( animations, STUDIO_NUMLOCALSEQ ) : 0 };
    const usize first{ static_cast< usize >( studio_int( animations, STUDIO_NUMLOCALSEQ + 4 ) ) };

    for ( i32 index{ }; index < count && index < static_cast< i32 >( SEQUENCES_MAX ); ++index )
    {
        const usize base{ first + static_cast< usize >( index ) * SEQDESC_SIZE };

        if ( base + SEQDESC_SIZE > size )
        {
            break;
        }

        const char *label{ studio_string( animations, size, base + static_cast< usize >( studio_int( animations, base + 4 ) ) ) };
        const char *activity{ studio_string( animations, size, base + static_cast< usize >( studio_int( animations, base + 8 ) ) ) };

        if ( !label || !activity )
        {
            break;
        }

        s_sequences.push_back( sequence_t{ HASH_RT( label ), activity[ 0 ] ? HASH_RT( activity ) : 0u,
                                           studio_int( animations, base + SEQDESC_ACTWEIGHT ) } );
    }

    item.m_sequence_count = s_sequences.size( ) - item.m_first_sequence;
    atl::free( animations );
}

static bool label_less( const char *left, const char *right )
{
    return str::compare_ignore_case( left, right ) < 0;
}

static void name_duplicates( atl::vector< kit_t > &kits )
{
    for ( kit_t &kit : kits )
    {
        if ( kit.m_label == str::npos )
        {
            kit.m_label = kit.m_name;
        }
    }

    for ( usize index{ }; index < kits.size( ); ++index )
    {
        bool duplicate{ };

        for ( usize other{ }; other < kits.size( ) && !duplicate; ++other )
        {
            duplicate = other != index && str::equals( text( kits[ index ].m_label ), text( kits[ other ].m_label ) );
        }

        if ( !duplicate )
        {
            continue;
        }

        fixed_string< LABEL_MAX > label{ };
        label.assign( text( kits[ index ].m_label ) );
        label.append( ATLAS_XORSTR( " (" ).get( ) );
        label.append( text( kits[ index ].m_name ) );
        label.append( ATLAS_XORSTR( ")" ).get( ) );

        kits[ index ].m_label = store( label.data( ), label.size( ) );
    }
}

static bool pair_shipped( i32 definition, i32 kit )
{
    const u32 key{ static_cast< u32 >( definition ) << 16 | static_cast< u32 >( kit ) << 2 };
    usize     low{ };
    usize     high{ s_pairs.size( ) };

    while ( low < high )
    {
        const usize middle{ ( low + high ) / 2 };

        if ( s_pairs[ middle ] < key )
        {
            low = middle + 1;
        } else
        {
            high = middle;
        }
    }

    return low < s_pairs.size( ) && s_pairs[ low ] == key;
}

static void assign_kits( atl::vector< item_t > &items, const atl::vector< kit_t > &kits )
{
    for ( item_t &item : items )
    {
        item.m_first_kit = s_item_kits.size( );

        for ( usize index{ }; index < kits.size( ); ++index )
        {
            if ( pair_shipped( item.m_definition, kits[ index ].m_id ) )
            {
                s_item_kits.push_back( index );
            }
        }

        item.m_kit_count = s_item_kits.size( ) - item.m_first_kit;
    }
}

static void build_game_tables( )
{
    usize size{ };

    if ( char *items{ read_game_file( ATLAS_XORSTR( "scripts/items/items_game.txt" ).get( ), size ) } )
    {
        parse_items_game( items, size );

        s_prefabs.reset( );
        atl::free( items );
    }

    atl::vector< item_t > *const item_lists[]{ &s_weapons, &s_knives, &s_gloves };
    atl::vector< kit_t > *const  kit_lists[]{ &s_weapon_kits, &s_glove_kits };

    atl::vector< pending_t > pending{ };

    for ( atl::vector< item_t > *list : item_lists )
    {
        for ( item_t &item : *list )
        {
            add_pending( pending, item.m_token, &item.m_label );
        }
    }

    for ( atl::vector< kit_t > *list : kit_lists )
    {
        for ( kit_t &kit : *list )
        {
            add_pending( pending, kit.m_token, &kit.m_label );
        }
    }

    if ( char *tokens{ read_game_file( ATLAS_XORSTR( "resource/csgo_english.txt" ).get( ), size ) } )
    {
        if ( size > sizeof( wchar_t ) )
        {
            localize( reinterpret_cast< const wchar_t * >( tokens ) + 1, size / sizeof( wchar_t ) - 1, pending );
        }

        atl::free( tokens );
    }

    for ( atl::vector< item_t > *list : item_lists )
    {
        for ( item_t &item : *list )
        {
            if ( item.m_label == str::npos )
            {
                item.m_label = item.m_token != str::npos ? item.m_token : store( "?", 1 );
            }
        }

        atl::sort( *list, []( const item_t &left, const item_t &right )
        { return label_less( text( left.m_label ), text( right.m_label ) ); } );
    }

    name_duplicates( s_weapon_kits );
    name_duplicates( s_glove_kits );

    for ( atl::vector< kit_t > *list : kit_lists )
    {
        atl::sort( *list, []( const kit_t &left, const kit_t &right )
        { return label_less( text( left.m_label ), text( right.m_label ) ); } );
    }

    for ( item_t &knife : s_knives )
    {
        read_sequences( knife );
    }

    for ( item_t &knife : s_stock_knives )
    {
        read_sequences( knife );
    }

    atl::sort( s_pairs, []( u32 left, u32 right )
    { return left < right; } );

    assign_kits( s_weapons, s_weapon_kits );
    assign_kits( s_knives, s_weapon_kits );
    assign_kits( s_gloves, s_glove_kits );
}

static const item_t *find_item( const atl::vector< item_t > &items, i32 definition )
{
    for ( const item_t &item : items )
    {
        if ( item.m_definition == definition )
        {
            return &item;
        }
    }

    return nullptr;
}

static const item_t *find_knife( i32 definition )
{
    const item_t *knife{ find_item( s_knives, definition ) };
    return knife ? knife : find_item( s_stock_knives, definition );
}

static i32 item_kit( const item_t &item, const atl::vector< kit_t > &kits, usize row )
{
    return row > 0 && row <= item.m_kit_count ? kits[ s_item_kits[ item.m_first_kit + row - 1 ] ].m_id : 0;
}

static i32 item_index( const atl::vector< item_t > &items, i32 definition )
{
    for ( usize index{ }; index < items.size( ); ++index )
    {
        if ( items[ index ].m_definition == definition )
        {
            return static_cast< i32 >( index );
        }
    }

    return -1;
}

static u32 raw_handle( const CBaseHandle &handle )
{
    return __builtin_bit_cast( u32, handle );
}

static C_BaseEntity *entity_from_handle( u32 handle )
{
    IClientEntity *entity{ g_csgo.m_entity_list->GetClientEntityFromHandle( __builtin_bit_cast( CBaseHandle, handle ) ) };
    return entity ? entity->As< C_BaseEntity * >( ) : nullptr;
}

static C_CSPlayer *local_player( )
{
    IClientEntity *entity{ g_csgo.m_entity_list->GetClientEntity( g_csgo.m_engine->GetLocalPlayer( ) ) };
    return entity ? entity->As< C_CSPlayer * >( ) : nullptr;
}

static void call_networkable( IClientNetworkable *networkable, u16 index, i32 update_type )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, i32 ) >( networkable, index )( networkable, update_type );
}

static void release_networkable( IClientNetworkable *networkable )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void * ) >( networkable, g_csgo.m_client_networkable_release )( networkable );
}

static void remove_dressing_attributes( C_EconEntity *entity )
{
    u8  *list{ entity->m_Item( ) + ITEM_ATTRIBUTE_LIST };
    u8  *memory{ *reinterpret_cast< u8 ** >( list + LIST_MEMORY ) };
    i32 &size{ *reinterpret_cast< i32 * >( list + LIST_SIZE ) };

    for ( i32 index{ size - 1 }; index >= 0; --index )
    {
        u8       *attribute{ memory + static_cast< usize >( index ) * ATTRIBUTE_SIZE };
        const u16 definition{ *reinterpret_cast< const u16 * >( attribute + ATTRIBUTE_DEFINITION ) };

        bool ours{ };
        for ( const u16 wanted : s_attributes )
        {
            ours |= wanted == definition;
        }

        if ( !ours )
        {
            continue;
        }

        for ( i32 next{ index + 1 }; next < size; ++next )
        {
            atl::memcpy( memory + static_cast< usize >( next - 1 ) * ATTRIBUTE_SIZE,
                         memory + static_cast< usize >( next ) * ATTRIBUTE_SIZE, ATTRIBUTE_SIZE );
        }

        --size;
    }
}

static void clear_custom_materials( u8 *owner )
{
    g_csgo.m_CustomMaterialOwner_ClearMaterials.get_as< void( ATLAS_THISCALL * )( u8 *, bool ) >( )( owner, false );
}

static void copy_custom_materials( u8 *owner, u8 *other )
{
    util::call_virtual< void( ATLAS_THISCALL * )( void *, void * ) >( owner, g_csgo.m_custom_material_owner_copy_to )( owner, other );
}

static void release_custom_material( C_EconEntity *entity )
{
    void *&material{ *reinterpret_cast< void ** >( entity->m_Item( ) + ITEM_CUSTOM_MATERIAL ) };

    if ( material )
    {
        util::call_virtual< void( ATLAS_THISCALL * )( void *, i32 ) >( material, 0 )( material, 1 );
        material = nullptr;
    }
}

static i32 local_account( )
{
    player_info_t info{ };
    return g_csgo.m_engine->GetPlayerInfo( g_csgo.m_engine->GetLocalPlayer( ), &info ) ? static_cast< i32 >( info.xuid ) : 0;
}

static void write_fallback( C_EconEntity *entity, const auto &dressing )
{
    entity->m_nFallbackPaintKit( ) = dressing.m_paintkit;
    entity->m_nFallbackSeed( )     = dressing.m_seed;
    entity->m_flFallbackWear( )    = dressing.m_wear;
    entity->m_nFallbackStatTrak( ) = dressing.m_stattrak;

    entity->m_iItemIDHigh( )           = -1;
    entity->m_OriginalOwnerXuidLow( )  = 0;
    entity->m_OriginalOwnerXuidHigh( ) = 0;
    entity->m_bInitialized( )          = true;

    if ( dressing.m_stattrak >= 0 )
    {
        entity->m_iAccountID( ) = local_account( );
    }

    str::copy( entity->m_szCustomName( ), CUSTOM_NAME_SIZE, dressing.m_name );
}

static void sequence_proxy( const CRecvProxyData *data, void *entity, void *out )
{
    if ( g_cl.m_is_running )
    {
        CRecvProxyData *changed{ const_cast< CRecvProxyData * >( data ) };
        changed->m_Int = g_skins.remap_sequence( static_cast< C_BaseViewModel * >( entity ), data->m_Int );
    }

    s_sequence_proxy( data, entity, out );
}

bool skins::install_proxies( )
{
    for ( ClientClass *iter{ g_csgo.m_client->GetAllClasses( ) }; iter; iter = iter->m_pNext )
    {
        if ( iter->m_pNetworkName && str::equals( iter->m_pNetworkName, ATLAS_XORSTR( "CEconWearable" ).get( ) ) )
        {
            s_wearable_class = iter;
        }
    }

    RecvProp *sequence{ g_netvars.get_prop( HASH_CT( "DT_BaseViewModel" ), HASH_CT( "m_nSequence" ) ) };

    if ( !s_wearable_class || !sequence )
    {
        return false;
    }

    s_sequence_proxy    = sequence->m_ProxyFn;
    sequence->m_ProxyFn = &sequence_proxy;
    return true;
}

void skins::publish_lists( ) const
{
    const auto fill_items{ []( gui::dynamic_list_box *list, const atl::vector< item_t > &items )
    {
        if ( !list || !list->insert_element( ATLAS_XORSTR_STATIC( "Default" ) ) )
        {
            return;
        }

        for ( const item_t &item : items )
        {
            if ( !list->insert_element( text( item.m_label ) ) )
            {
                return;
            }
        }
    } };

    const auto fill_paints{ []( gui::dynamic_list_box *const *lists, const atl::vector< item_t > &items, const atl::vector< kit_t > &kits,
                                bool none )
    {
        for ( usize index{ }; index < items.size( ) && index < PAINT_LISTS_MAX; ++index )
        {
            gui::dynamic_list_box *list{ lists[ index ] };

            if ( !list || ( none && !list->insert_element( ATLAS_XORSTR_STATIC( "None" ) ) ) )
            {
                continue;
            }

            for ( usize kit{ }; kit < items[ index ].m_kit_count; ++kit )
            {
                if ( !list->insert_element( text( kits[ s_item_kits[ items[ index ].m_first_kit + kit ] ].m_label ) ) )
                {
                    break;
                }
            }
        }
    } };

    fill_items( this->m_knife_list, s_knives );
    fill_items( this->m_glove_list, s_gloves );
    fill_paints( this->m_paint_lists[ static_cast< usize >( paint_list_t::weapon ) ], s_weapons, s_weapon_kits, true );
    fill_paints( this->m_paint_lists[ static_cast< usize >( paint_list_t::knife ) ], s_knives, s_weapon_kits, true );

    fill_paints( this->m_paint_lists[ static_cast< usize >( paint_list_t::glove ) ], s_gloves, s_glove_kits, false );
}

void skins::service_game_tables( )
{
    if ( !this->m_tables_requested.load( ) || this->m_tables_ready.load( ) )
    {
        return;
    }

    build_game_tables( );
    this->publish_lists( );
    this->m_tables_ready.store( true );
}

const skins::seen_t *skins::find_seen( u32 handle ) const
{
    for ( usize index{ }; index < this->m_seen_count; ++index )
    {
        if ( this->m_seen[ index ].m_handle == handle )
        {
            return &this->m_seen[ index ];
        }
    }

    return nullptr;
}

skins::dressing_t skins::wanted_weapon( i32 server_definition ) const
{
    const config::skins_t &cfg{ g_cfg.m_skins };

    if ( !cfg.m_enabled )
    {
        return dressing_t{ };
    }

    dressing_t    dressing{ };
    const item_t *item{ find_item( s_weapons, server_definition ) };

    if ( find_knife( server_definition ) )
    {
        item = cfg.m_knife > 0 && cfg.m_knife <= s_knives.size( ) ? &s_knives[ cfg.m_knife - 1 ] : nullptr;

        if ( !item )
        {
            return dressing_t{ };
        }

        dressing.m_knife = cfg.m_knife;
    }

    if ( !item )
    {
        return dressing_t{ };
    }

    const config::skin_t *skin{ };

    for ( const config::skin_entry_t &entry : cfg.m_weapons )
    {
        if ( entry.m_definition == item->m_definition )
        {
            skin = &entry.m_skin;
            break;
        }
    }

    if ( !dressing.m_knife && ( !skin || skin->off( ) ) )
    {
        return dressing_t{ };
    }

    dressing.m_definition = item->m_definition;

    if ( skin )
    {
        dressing.m_paintkit = item_kit( *item, s_weapon_kits, skin->m_paintkit );
        dressing.m_wear     = skin->m_wear;
        dressing.m_seed     = skin->m_seed;
        dressing.m_stattrak = skin->m_stattrak ? skin->m_stattrak_count : -1;
        str::copy( dressing.m_name, sizeof( dressing.m_name ), skin->m_name );
    }

    return dressing;
}

skins::dressing_t skins::wanted_gloves( ) const
{
    const config::skins_t &cfg{ g_cfg.m_skins };

    if ( !cfg.m_enabled || cfg.m_gloves.m_model == 0 || cfg.m_gloves.m_model > s_gloves.size( ) )
    {
        return dressing_t{ };
    }

    const item_t &glove{ s_gloves[ cfg.m_gloves.m_model - 1 ] };

    dressing_t dressing{ };
    dressing.m_definition = glove.m_definition;
    dressing.m_paintkit   = item_kit( glove, s_glove_kits, cfg.m_gloves.m_paintkit + 1 );
    dressing.m_wear       = cfg.m_gloves.m_wear;
    dressing.m_seed       = cfg.m_gloves.m_seed;
    return dressing;
}

void skins::dress_weapon( C_BaseCombatWeapon *weapon, seen_t &item, const dressing_t &dressing )
{
    C_EconEntity     *econ{ weapon->As< C_EconEntity * >( ) };
    const original_t &original{ item.m_original };
    const bool        fresh{ !econ->m_bFallbackApplied( ) };

    item.m_dressing    = dressing;
    item.m_view_model  = -1;
    item.m_world_model = -1;

    i32 definition{ original.m_definition };
    i32 quality{ original.m_quality };

    if ( dressing.m_knife )
    {
        const item_t &knife{ s_knives[ dressing.m_knife - 1 ] };
        const i32     view{ g_csgo.m_model_info->GetModelIndex( text( knife.m_model_player ) ) };
        const i32     world{ g_csgo.m_model_info->GetModelIndex( text( knife.m_model_world ) ) };

        if ( view != -1 && world != -1 )
        {
            definition         = knife.m_definition;
            item.m_view_model  = view;
            item.m_world_model = world;
        }
    }

    if ( dressing.dressed( ) )
    {
        if ( find_item( s_knives, definition ) )
        {
            quality = s_quality_unusual;
        } else if ( dressing.m_stattrak >= 0 )
        {
            quality = s_quality_strange;
        }

        write_fallback( econ, dressing );
    } else
    {
        econ->m_iItemIDHigh( )       = original.m_item_id_high;
        econ->m_iAccountID( )        = original.m_account_id;
        econ->m_bInitialized( )      = original.m_initialized;
        econ->m_nFallbackPaintKit( ) = 0;
        econ->m_nFallbackSeed( )     = 0;
        econ->m_flFallbackWear( )    = 0.0f;
        econ->m_nFallbackStatTrak( ) = -1;
        econ->m_szCustomName( )[ 0 ] = '\0';
    }

    econ->m_iItemDefinitionIndex( ) = static_cast< i16 >( definition );
    econ->m_iEntityQuality( )       = quality;

    const i32 model{ item.m_view_model != -1 ? item.m_view_model : original.m_model };
    const i32 view_model{ item.m_view_model != -1 ? item.m_view_model : original.m_view_model };
    const i32 world_model{ item.m_world_model != -1 ? item.m_world_model : original.m_world_model };

    if ( weapon->m_nModelIndex( ) != model )
    {
        weapon->SetModelIndex( model );
    }

    weapon->m_iViewModelIndex( )  = view_model;
    weapon->m_iWorldModelIndex( ) = world_model;

    if ( C_BaseEntity *world{ weapon->m_hWeaponWorldModel( ).Get( ) }; world && world->m_nModelIndex( ) != world_model )
    {
        world->SetModelIndex( world_model );
    }

    if ( fresh )
    {
        return;
    }

    remove_dressing_attributes( econ );
    release_custom_material( econ );
    clear_custom_materials( econ->m_Item( ) + ITEM_MATERIAL_OWNER );
    clear_custom_materials( econ->m_Item( ) + ITEM_COMPOSITE_TEXTURES );
    clear_custom_materials( reinterpret_cast< u8 * >( weapon ) + ENTITY_MATERIAL_OWNER );
    *( reinterpret_cast< u8 * >( weapon ) + WEAPON_MATERIALS_CHECKED ) = 0;

    econ->UpdateFallbackAttributes( );

    g_csgo.m_C_WeaponCSBase_CheckCustomMaterial.get_as< void( ATLAS_THISCALL * )( C_BaseCombatWeapon * ) >( )( weapon );
    g_csgo.m_SFWeaponSelection_RefreshWeapon.get_as< void( ATLAS_STDCALL * )( C_BaseCombatWeapon * ) >( )( weapon );

    C_CSPlayer *local{ local_player( ) };

    if ( C_BaseEntity *viewmodel{ local ? local->m_hViewModel( ).Get( ) : nullptr };
         viewmodel && raw_handle( viewmodel->As< C_BaseViewModel * >( )->m_hWeapon( ) ) == raw_handle( weapon->GetRefEHandle( ) ) )
    {
        copy_custom_materials( reinterpret_cast< u8 * >( weapon ) + ENTITY_MATERIAL_OWNER,
                               reinterpret_cast< u8 * >( viewmodel ) + ENTITY_MATERIAL_OWNER );
    }

    if ( C_BaseEntity * world{ weapon->m_hWeaponWorldModel( ).Get( ) } )
    {
        copy_custom_materials( reinterpret_cast< u8 * >( weapon ) + ENTITY_MATERIAL_OWNER,
                               reinterpret_cast< u8 * >( world ) + ENTITY_MATERIAL_OWNER );
    }
}

void skins::release_gloves( C_CSPlayer *local )
{
    if ( C_BaseEntity * glove{ entity_from_handle( this->m_glove_handle ) } )
    {
        release_networkable( glove->GetClientNetworkable( ) );
    }

    if ( local && raw_handle( local->m_hMyWearables( )[ 0 ] ) == this->m_glove_handle )
    {
        local->m_hMyWearables( )[ 0 ] = __builtin_bit_cast( EHANDLE, this->m_server_wearable );
        local->m_nBody( )             = this->m_server_body;
    }

    this->m_glove_handle   = 0;
    this->m_glove_dressing = dressing_t{ };
    this->m_arms_dirty     = true;
}

void skins::update_gloves( C_CSPlayer *local )
{
    const dressing_t wanted{ wanted_gloves( ) };
    C_BaseEntity    *glove{ entity_from_handle( this->m_glove_handle ) };

    if ( glove && wanted != this->m_glove_dressing )
    {
        this->release_gloves( local );
        glove = nullptr;
    }

    if ( !wanted.dressed( ) )
    {
        if ( this->m_glove_handle )
        {
            this->release_gloves( local );
        }

        return;
    }

    if ( !glove )
    {
        i32 index{ MAX_EDICTS - 1 };

        while ( index > 0 && g_csgo.m_entity_list->GetClientEntity( index ) )
        {
            --index;
        }

        const item_t *item{ find_item( s_gloves, wanted.m_definition ) };
        const u32     serial{ ( ++this->m_glove_serial & 0x7FFFu ) | 0x8000u };

        IClientNetworkable *networkable{ index > 0 && item ? s_wearable_class->m_pCreateFn( index, static_cast< i32 >( serial ) ) : nullptr };

        if ( !networkable )
        {
            return;
        }

        call_networkable( networkable, g_csgo.m_client_networkable_pre_data_update, DATA_UPDATE_CREATED );

        C_EconEntity *wearable{ g_csgo.m_entity_list->GetClientEntity( index )->As< C_EconEntity * >( ) };

        wearable->m_iItemDefinitionIndex( ) = static_cast< i16 >( wanted.m_definition );
        wearable->m_iEntityQuality( )       = s_quality_unusual;
        wearable->m_iAccountID( )           = local_account( );
        wearable->m_hOwnerEntity( )         = __builtin_bit_cast( EHANDLE, raw_handle( local->GetRefEHandle( ) ) );
        write_fallback( wearable, wanted );

        if ( const i32 world{ g_csgo.m_model_info->GetModelIndex( text( item->m_model_world ) ) }; world != -1 )
        {
            wearable->SetModelIndex( world );
        }

        call_networkable( networkable, g_csgo.m_client_networkable_post_data_update, DATA_UPDATE_CREATED );
        call_networkable( networkable, g_csgo.m_client_networkable_on_data_changed, DATA_UPDATE_CREATED );

        wearable->m_index( ) = -1;

        util::call_virtual< void( ATLAS_THISCALL * )( void *, C_CSPlayer * ) >( wearable, g_csgo.m_econ_wearable_equip )( wearable, local );

        this->m_glove_handle   = static_cast< u32 >( index ) | serial << 16;
        this->m_glove_dressing = wanted;
        this->m_arms_dirty     = true;
    }

    EHANDLE &slot{ local->m_hMyWearables( )[ 0 ] };

    if ( raw_handle( slot ) != this->m_glove_handle )
    {
        this->m_server_wearable = raw_handle( slot );
        slot                    = __builtin_bit_cast( EHANDLE, this->m_glove_handle );
        this->m_arms_dirty      = true;
    }

    if ( ( local->m_nBody( ) & GLOVES_BODYGROUP_EMPTY ) == 0 )
    {
        this->m_server_body = local->m_nBody( );
        local->m_nBody( ) |= GLOVES_BODYGROUP_EMPTY;
    }
}

void skins::on_post_data_update_start( )
{
    C_CSPlayer *local{ g_csgo.m_engine->IsInGame( ) ? local_player( ) : nullptr };

    if ( !this->m_tables_ready.load( ) || !local )
    {
        this->m_seen_count    = 0;
        this->m_swapped_knife = 0;
        this->m_held_definition.store( -1 );
        return;
    }

    usize    next_count{ };
    EHANDLE *weapons{ local->m_hMyWeapons( ) };

    for ( usize slot{ }; slot < MY_WEAPONS && next_count < SEEN_MAX; ++slot )
    {
        const u32     handle{ raw_handle( weapons[ slot ] ) };
        C_BaseEntity *entity{ entity_from_handle( handle ) };

        if ( !entity )
        {
            continue;
        }

        C_BaseCombatWeapon *weapon{ entity->As< C_BaseCombatWeapon * >( ) };
        C_EconEntity       *econ{ entity->As< C_EconEntity * >( ) };
        const seen_t       *before{ this->find_seen( handle ) };
        seen_t             &now{ this->m_seen_next[ next_count++ ] };

        if ( before && econ->m_bFallbackApplied( ) )
        {
            now = *before;
        } else
        {
            now                           = seen_t{ };
            now.m_handle                  = handle;
            now.m_original.m_definition   = econ->m_iItemDefinitionIndex( );
            now.m_original.m_quality      = econ->m_iEntityQuality( );
            now.m_original.m_item_id_high = econ->m_iItemIDHigh( );
            now.m_original.m_account_id   = econ->m_iAccountID( );
            now.m_original.m_initialized  = econ->m_bInitialized( );
            now.m_original.m_model        = weapon->m_nModelIndex( );
            now.m_original.m_view_model   = weapon->m_iViewModelIndex( );
            now.m_original.m_world_model  = weapon->m_iWorldModelIndex( );
        }

        const dressing_t wanted{ this->wanted_weapon( now.m_original.m_definition ) };

        if ( wanted != now.m_dressing )
        {
            this->dress_weapon( weapon, now, wanted );
        }
    }

    this->m_swapped_knife = 0;

    for ( usize index{ }; index < next_count; ++index )
    {
        this->m_seen[ index ] = this->m_seen_next[ index ];

        if ( this->m_seen[ index ].m_view_model != -1 )
        {
            this->m_swapped_knife = this->m_seen[ index ].m_dressing.m_knife;
        }
    }

    this->m_seen_count = next_count;

    const seen_t *held{ this->find_seen( raw_handle( local->m_hActiveWeapon( ) ) ) };
    this->m_held_definition.store( held ? held->m_original.m_definition : -1 );

    this->update_gloves( local );
    this->match_knife_models( local );
}

void skins::match_knife_models( C_CSPlayer *local )
{
    const u32     handle{ raw_handle( local->m_hActiveWeapon( ) ) };
    const seen_t *item{ this->find_seen( handle ) };
    C_BaseEntity *entity{ entity_from_handle( handle ) };

    if ( !item || !entity || !find_knife( item->m_original.m_definition ) )
    {
        return;
    }

    const i32 view_model{ item->m_view_model != -1 ? item->m_view_model : item->m_original.m_view_model };
    const i32 world_model{ item->m_world_model != -1 ? item->m_world_model : item->m_original.m_world_model };

    if ( C_BaseEntity *viewmodel{ local->m_hViewModel( ).Get( ) };
         viewmodel && raw_handle( viewmodel->As< C_BaseViewModel * >( )->m_hWeapon( ) ) == handle &&
         viewmodel->m_nModelIndex( ) != view_model )
    {
        viewmodel->SetModelIndex( view_model );
    }

    if ( C_BaseEntity *world{ entity->As< C_BaseCombatWeapon * >( )->m_hWeaponWorldModel( ).Get( ) };
         world && world->m_nModelIndex( ) != world_model )
    {
        world->SetModelIndex( world_model );
    }
}

void skins::on_render_start( )
{
    C_CSPlayer *local{ g_csgo.m_engine->IsInGame( ) && this->m_tables_ready.load( ) ? local_player( ) : nullptr };

    if ( !local )
    {
        return;
    }

    this->match_knife_models( local );

    if ( !this->m_arms_dirty )
    {
        return;
    }

    if ( C_BaseEntity * entity{ local->m_hViewModel( ).Get( ) } )
    {
        C_BaseViewModel *viewmodel{ entity->As< C_BaseViewModel * >( ) };

        viewmodel->RemoveViewmodelArmModels( );
        viewmodel->UpdateAllViewmodelAddons( );
    }

    this->m_arms_dirty = false;
}

void skins::on_player_death( IGameEvent *event )
{
    if ( !this->m_swapped_knife || !this->m_tables_ready.load( ) )
    {
        return;
    }

    player_info_t info{ };

    if ( !g_csgo.m_engine->GetPlayerInfo( g_csgo.m_engine->GetLocalPlayer( ), &info ) ||
         event->GetInt( ATLAS_XORSTR( "attacker" ).get( ) ) != info.userID )
    {
        return;
    }

    const char *weapon{ event->GetString( ATLAS_XORSTR( "weapon" ).get( ) ) };

    for ( const item_t &stock : s_stock_knives )
    {
        if ( stock.m_death_name != str::npos &&
             str::starts_with( weapon, str::length( weapon ), text( stock.m_death_name ), str::length( text( stock.m_death_name ) ) ) )
        {
            event->SetString( ATLAS_XORSTR( "weapon" ).get( ), text( s_knives[ this->m_swapped_knife - 1 ].m_death_name ) );
            return;
        }
    }
}

i32 skins::remap_sequence( C_BaseViewModel *viewmodel, i32 sequence )
{
    const seen_t *item{ this->find_seen( raw_handle( viewmodel->m_hWeapon( ) ) ) };

    if ( !item || item->m_view_model == -1 || !item->m_dressing.m_knife )
    {
        return sequence;
    }

    const item_t *from{ find_knife( item->m_original.m_definition ) };
    const item_t &to{ s_knives[ item->m_dressing.m_knife - 1 ] };

    if ( !from || sequence < 0 || static_cast< usize >( sequence ) >= from->m_sequence_count || !to.m_sequence_count )
    {
        return sequence;
    }

    const sequence_t &played{ s_sequences[ from->m_first_sequence + static_cast< usize >( sequence ) ] };
    const sequence_t *targets{ &s_sequences[ to.m_first_sequence ] };
    hash_t            activity{ played.m_activity };

    for ( usize index{ }; !activity && index < to.m_sequence_count; ++index )
    {
        if ( targets[ index ].m_label == played.m_label )
        {
            if ( !targets[ index ].m_activity )
            {
                return static_cast< i32 >( index );
            }

            activity = targets[ index ].m_activity;
        }
    }

    i32 total{ };

    for ( usize index{ }; index < to.m_sequence_count; ++index )
    {
        if ( activity && targets[ index ].m_activity == activity )
        {
            total += math::bigger( targets[ index ].m_weight, 1 );
        }
    }

    if ( !total )
    {
        return sequence;
    }

    i32 pick{ static_cast< i32 >( next_random( ) % static_cast< u32 >( total ) ) };

    for ( usize index{ }; index < to.m_sequence_count; ++index )
    {
        if ( targets[ index ].m_activity != activity )
        {
            continue;
        }

        pick -= math::bigger( targets[ index ].m_weight, 1 );

        if ( pick < 0 )
        {
            return static_cast< i32 >( index );
        }
    }

    return sequence;
}

bool skins::paint_list_shown( paint_list_t kind, usize index ) const
{
    if ( !this->m_tables_ready.load( ) || !g_cfg.m_skins.m_enabled )
    {
        return false;
    }

    switch ( kind )
    {
        case paint_list_t::weapon: return item_index( s_weapons, this->m_held_definition.load( ) ) == static_cast< i32 >( index );
        case paint_list_t::knife: return g_cfg.m_skins.m_knife == index + 1;
        case paint_list_t::glove: return g_cfg.m_skins.m_gloves.m_model == index + 1;
        default: return false;
    }
}

bool skins::editing_weapon( ) const
{
    return this->m_tables_ready.load( ) && g_cfg.m_skins.m_enabled && find_item( s_weapons, this->m_held_definition.load( ) );
}

bool skins::editing_knife( ) const
{
    return this->m_tables_ready.load( ) && g_cfg.m_skins.m_enabled && g_cfg.m_skins.m_knife > 0;
}

const char *skins::held_text( ) const
{
    if ( !this->m_tables_ready.load( ) )
    {
        return ATLAS_XORSTR_STATIC( "Reading the game's item list" );
    }

    const i32 held{ this->m_held_definition.load( ) };

    if ( const item_t *item{ find_item( s_weapons, held ) } )
    {
        return text( item->m_label );
    }

    if ( find_knife( held ) )
    {
        return ATLAS_XORSTR_STATIC( "Knife, its looks are under Knife" );
    }

    return held < 0 ? ATLAS_XORSTR_STATIC( "Hold a weapon to edit its looks" )
                    : ATLAS_XORSTR_STATIC( "This weapon has no paints" );
}

void skins::sync_entry( paint_list_t kind, i32 key, config::skin_t &edit )
{
    config::skins_t &cfg{ g_cfg.m_skins };
    i32             &edit_key{ this->m_edit_keys[ static_cast< usize >( kind ) ] };
    gui::text_input *input{ this->m_name_inputs[ static_cast< usize >( kind ) ] };

    config::skin_entry_t *entry{ };
    config::skin_entry_t *free_slot{ };

    for ( config::skin_entry_t &candidate : cfg.m_weapons )
    {
        if ( candidate.m_definition == key )
        {
            entry = &candidate;
            break;
        }

        if ( candidate.m_definition < 0 && !free_slot )
        {
            free_slot = &candidate;
        }
    }

    if ( key != edit_key )
    {
        edit_key = key;
        edit     = entry ? entry->m_skin : config::skin_t{ };

        if ( input )
        {
            input->set_value( edit.m_name );
        }

        return;
    }

    if ( key < 0 )
    {
        return;
    }

    if ( input )
    {
        const string_view name{ input->value( ) };
        str::copy( edit.m_name, sizeof( edit.m_name ), name.data( ), math::smaller( name.size( ), config::SKIN_NAME_LENGTH ) );
    }

    if ( entry )
    {
        entry->m_skin = edit;

        if ( entry->m_skin.off( ) )
        {
            entry->m_definition = -1;
        }
    } else if ( free_slot && !edit.off( ) )
    {
        free_slot->m_skin       = edit;
        free_slot->m_definition = key;
    }
}

void skins::sync_editor( )
{
    if ( !this->m_tables_ready.load( ) )
    {
        return;
    }

    config::skins_t &cfg{ g_cfg.m_skins };
    const item_t    *held{ find_item( s_weapons, this->m_held_definition.load( ) ) };
    const i32        knife{ cfg.m_knife > 0 && cfg.m_knife <= s_knives.size( ) ? s_knives[ cfg.m_knife - 1 ].m_definition : -1 };

    this->sync_entry( paint_list_t::weapon, held ? held->m_definition : -1, cfg.m_weapon_edit );
    this->sync_entry( paint_list_t::knife, knife, cfg.m_knife_edit );

    config::glove_skin_t &gloves{ cfg.m_gloves };

    if ( gloves.m_model != this->m_edit_glove_model )
    {
        this->m_edit_glove_model = gloves.m_model;

        if ( gloves.m_model > 0 && gloves.m_model <= s_gloves.size( ) && s_gloves[ gloves.m_model - 1 ].m_kit_count )
        {
            gloves.m_paintkit = next_random( ) % s_gloves[ gloves.m_model - 1 ].m_kit_count;
        }
    }
}
