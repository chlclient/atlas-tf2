#include "netvars.hpp"
#include "global.hpp"

void netvars::insert( hash_t key, i32 offset, const RecvProp *prop )
{
    u32 slot{ key & VARS_MASK };
    for ( usize step{ }; step < VARS_CAPACITY; ++step )
    {
        entry &current{ this->m_entries[ slot ] };

        if ( current.m_prop == nullptr )
        {
            current.m_key    = key;
            current.m_offset = offset;
            current.m_prop   = prop;
            ++this->m_count;
            return;
        }

        if ( current.m_key == key )
        {
            return;
        }

        slot = ( slot + 1 ) & VARS_MASK;
    }

    this->m_overflow = true;
}

const netvars::entry *netvars::find( hash_t key ) const
{
    u32 slot{ key & VARS_MASK };

    for ( usize step{ }; step < VARS_CAPACITY; ++step )
    {
        const entry &current{ this->m_entries[ slot ] };

        if ( current.m_prop == nullptr )
        {
            return nullptr;
        }

        if ( current.m_key == key )
        {
            return &current;
        }

        slot = ( slot + 1 ) & VARS_MASK;
    }

    return nullptr;
}

i32 netvars::get_offset( hash_t table, hash_t prop ) const
{
    const entry *hit{ this->find( table ^ prop ) };
    Assert( hit != nullptr );

    return hit ? hit->m_offset : 0;
}

// the props live in client.dll's writable table data, which is what lets a proxy be swapped
RecvProp *netvars::get_prop( hash_t table, hash_t prop ) const
{
    const entry *hit{ this->find( table ^ prop ) };

    return hit ? const_cast< RecvProp * >( hit->m_prop ) : nullptr;
}

// -----------------------------------------------------------------------------
// Here we basically mirror the recursive descent Valve itself performs when building
// CRecvDecoder (check BuildFlatSendTable in dt.cpp and CRecvDecoder::Init in
// dt_recv.cpp) Every DPT_DataTable child is inlined into its parent by adding
// its m_Offset to the accumulated base;
// -----------------------------------------------------------------------------
void netvars::dump_recv_table( RecvTable *table, hash_t table_hash, i32 base_offset )
{
    if ( !table || !table->m_pProps )
    {
        return;
    }

    for ( i32 index{ }; index < table->m_nProps; ++index )
    {
        RecvProp &prop{ table->m_pProps[ index ] };

        const char *name{ prop.m_pVarName };
        if ( !name )
        {
            continue;
        }

        // Array element properties are named "000", "001", etc..
        // These names are only relevant through the parent array property,
        // so they can be skipped during named lookup;
        if ( str::is_digit( name[ 0 ] ) )
        {
            continue;
        }

        const i32 total_offset{ base_offset + prop.m_Offset };

        if ( prop.m_RecvType == DPT_DataTable && prop.m_pDataTable )
        {
            RecvTable *child{ prop.m_pDataTable };

            // "baseclass" chains a derived table to its parent's, and CS:GO has parents with no
            // client class of their own (DT_BaseCombatCharacter holds m_hActiveWeapon), so the
            // parent is dumped under its own name instead of being skipped
            const bool is_base{ str::equals( name, ATLAS_XORSTR( "baseclass" ).get( ) ) };
            if ( !is_base )
            {
                this->insert( table_hash ^ HASH_RT( name ), total_offset, &prop );
            }

            // @ SendPropDataTable can either splice a child table's props into the
            // parent's namespace (Valve uses this for DT_LocalPlayerExclusive etc) or expose it as a nested
            // namespace; When the prop's name equals the child table's
            // m_pNetTableName Valve is treating it as its own namespace, so we
            // switch the hash to the child; Otherwise we keep flattening under
            // the current table so grand-children remain findable at the top-level name
            hash_t child_hash{ table_hash };
            if ( child->m_pNetTableName && ( is_base || str::equals( name, child->m_pNetTableName ) ) )
            {
                child_hash = HASH_RT( child->m_pNetTableName );
            }

            this->dump_recv_table( child, child_hash, total_offset );
            continue;
        }

        this->insert( table_hash ^ HASH_RT( name ), total_offset, &prop );
    }
}

// -----------------------------------------------------------------------------
// CHLClient::GetAllClasses returns the head of a singly linked
// list built by every DECLARE_CLIENTCLASS / IMPLEMENT_CLIENTCLASS invocation
// in client.dll (client_class.h - ClientClass constructor pushes
// itself onto g_pClientClassHead) We walk it once at startup.
// -----------------------------------------------------------------------------
bool netvars::load( )
{
    for ( ClientClass *iter{ g_csgo.m_client->GetAllClasses( ) }; iter; iter = iter->m_pNext )
    {
        RecvTable *table{ iter->m_pRecvTable };
        if ( !table || !table->m_pNetTableName )
        {
            continue;
        }

        this->dump_recv_table( table, HASH_RT( table->m_pNetTableName ), 0 );
    }

    return this->m_count > 0 && !this->m_overflow;
}
