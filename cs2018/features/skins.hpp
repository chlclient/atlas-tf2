#pragma once

#include "link.hpp"
#include "runtime/atomic.hpp"

namespace gui
{
    class dynamic_list_box;
    class text_input;
}

enum class paint_list_t : u8
{
    weapon,
    knife,
    glove,
    count,
};

class skins
{
public:
    static constexpr usize PAINT_LISTS_MAX{ 40 };
    static constexpr usize PAINT_LISTS[ static_cast< usize >( paint_list_t::count ) ]{ 40, 12, 8 };

    void bind_paint_list( paint_list_t kind, usize index, gui::dynamic_list_box *list )
    {
        this->m_paint_lists[ static_cast< usize >( kind ) ][ index ] = list;
    }

    void bind_knife_list( gui::dynamic_list_box *list )
    {
        this->m_knife_list = list;
    }

    void bind_glove_list( gui::dynamic_list_box *list )
    {
        this->m_glove_list = list;
    }

    void bind_name_input( paint_list_t kind, gui::text_input *input )
    {
        this->m_name_inputs[ static_cast< usize >( kind ) ] = input;
    }

    void request_game_tables( )
    {
        this->m_tables_requested.store( true );
    }

    void service_game_tables( );
    bool install_proxies( );

    void on_post_data_update_start( );
    void on_render_start( );
    void on_player_death( IGameEvent *event );
    void sync_editor( );

    i32 remap_sequence( C_BaseViewModel *viewmodel, i32 sequence );

    bool        paint_list_shown( paint_list_t kind, usize index ) const;
    bool        editing_weapon( ) const;
    bool        editing_knife( ) const;
    const char *held_text( ) const;

private:
    struct dressing_t
    {
        i32   m_definition{ -1 };
        usize m_knife{ };
        i32   m_paintkit{ };
        float m_wear{ };
        i32   m_seed{ };
        i32   m_stattrak{ -1 };
        char  m_name[ config::SKIN_NAME_LENGTH + 1 ]{ };

        bool operator==( const dressing_t &other ) const = default;

        bool dressed( ) const
        {
            return this->m_definition >= 0;
        }
    };

    struct original_t
    {
        i32  m_definition{ };
        i32  m_quality{ };
        i32  m_item_id_high{ };
        i32  m_account_id{ };
        bool m_initialized{ };
        i32  m_model{ };
        i32  m_view_model{ };
        i32  m_world_model{ };
    };

    struct seen_t
    {
        u32        m_handle{ };
        original_t m_original{ };
        dressing_t m_dressing{ };

        i32 m_view_model{ -1 };
        i32 m_world_model{ -1 };
    };

    static constexpr usize SEEN_MAX{ 64 };

    dressing_t wanted_weapon( i32 server_definition ) const;
    dressing_t wanted_gloves( ) const;
    void       dress_weapon( C_BaseCombatWeapon *weapon, seen_t &item, const dressing_t &dressing );
    void       update_gloves( C_CSPlayer *local );
    void       release_gloves( C_CSPlayer *local );
    void       match_knife_models( C_CSPlayer *local );
    void       publish_lists( ) const;
    void       sync_entry( paint_list_t kind, i32 key, config::skin_t &edit );

    const seen_t *find_seen( u32 handle ) const;

    seen_t m_seen[ SEEN_MAX ]{ };
    usize  m_seen_count{ };
    seen_t m_seen_next[ SEEN_MAX ]{ };

    u32        m_glove_handle{ };
    u32        m_server_wearable{ };
    i32        m_server_body{ };
    dressing_t m_glove_dressing{ };
    u32        m_glove_serial{ };
    bool       m_arms_dirty{ };
    usize      m_swapped_knife{ };

    atl::atomic< i32 > m_held_definition{ -1 };

    i32   m_edit_keys[ static_cast< usize >( paint_list_t::count ) ]{ -2, -2, -2 };
    usize m_edit_glove_model{ };

    gui::dynamic_list_box *m_paint_lists[ static_cast< usize >( paint_list_t::count ) ][ PAINT_LISTS_MAX ]{ };
    gui::dynamic_list_box *m_knife_list{ };
    gui::dynamic_list_box *m_glove_list{ };
    gui::text_input       *m_name_inputs[ static_cast< usize >( paint_list_t::count ) ]{ };

    atl::atomic< bool > m_tables_requested{ false };
    atl::atomic< bool > m_tables_ready{ false };
};

extern skins g_skins;
