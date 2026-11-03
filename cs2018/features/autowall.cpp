#include "autowall.hpp"

ATLAS_NO_DESTROY autowall g_autowall{ };

constexpr float PENETRATION_DISTANCE{ 3000.0f };

ATLAS_FORCE_INLINE bool trace_to_exit( const vec3 &start, const vec3 &direction, vec3 &end, const trace_t &enter_trace,
                                       trace_t &exit_trace )
{
    constexpr float STEP{ 4.0f };

    float distance{ };
    i32   start_contents{ };

    while ( distance <= MAX_PENETRATION_DISTANCE )
    {
        distance += STEP;

        end = start + direction * distance;

        const vec3 trace_end{ end - direction * STEP };

        if ( !start_contents )
        {
            start_contents = g_csgo.m_trace->GetPointContents( end, CS_MASK_SHOOT | CONTENTS_HITBOX, nullptr );
        }

        const i32 contents{ g_csgo.m_trace->GetPointContents( end, CS_MASK_SHOOT | CONTENTS_HITBOX, nullptr ) };
        if ( ( contents & CS_MASK_SHOOT ) && ( !( contents & CONTENTS_HITBOX ) || contents == start_contents ) )
        {
            continue;
        }

        g_cl.trace_line( end, trace_end, CS_MASK_SHOOT | CONTENTS_HITBOX, nullptr, exit_trace );

        if ( g_csgo.sv_clip_penetration_traces_to_players->GetInt( ) )
        {
            g_cl.clip_trace_to_players( end, trace_end, CS_MASK_SHOOT | CONTENTS_HITBOX, nullptr, exit_trace, -60.0f );
        }

        if ( exit_trace.startsolid && ( exit_trace.surface.flags & SURF_HITBOX ) )
        {
            CTraceFilterSkipTwoEntities filter{ exit_trace.m_pEnt, nullptr };
            g_cl.trace_line( end, start, CS_MASK_SHOOT, filter.filter( ), exit_trace );

            if ( exit_trace.DidHit( ) && !exit_trace.startsolid )
            {
                end = exit_trace.endpos;
                return true;
            }

            continue;
        }

        if ( !exit_trace.DidHit( ) || exit_trace.startsolid )
        {
            if ( enter_trace.DidHitNonWorldEntity( ) && IsBreakableEntity( enter_trace.m_pEnt ) )
            {
                exit_trace        = enter_trace;
                exit_trace.endpos = start + direction;
                return true;
            }

            continue;
        }

        if ( exit_trace.surface.flags & SURF_NODRAW )
        {
            if ( IsBreakableEntity( exit_trace.m_pEnt ) && IsBreakableEntity( enter_trace.m_pEnt ) )
            {
                end = exit_trace.endpos;
                return true;
            }

            if ( !( enter_trace.surface.flags & SURF_NODRAW ) )
            {
                continue;
            }
        }

        if ( exit_trace.plane.normal.dot( direction ) <= 1.0f )
        {
            end -= direction * ( exit_trace.fraction * STEP );
            return true;
        }
    }

    return false;
}

ATLAS_FORCE_INLINE bool handle_bullet_penetration( C_CSPlayer *shooter, const bullet_t &bullet, const vec3 &direction,
                                                   const trace_t &enter_trace, const surfacedata_t *enter_surface,
                                                   i32 &penetrations, vec3 &start, float &damage, bullet_hit_t &hit )
{
    if ( bullet.m_penetration <= 0.0f || penetrations <= 0 )
    {
        return true;
    }

    vec3    end{ };
    trace_t exit_trace{ };
    if ( !trace_to_exit( enter_trace.endpos, direction, end, enter_trace, exit_trace ) &&
         !( g_csgo.m_trace->GetPointContents( enter_trace.endpos, CS_MASK_SHOOT, nullptr ) & CS_MASK_SHOOT ) )
    {
        return true;
    }

    const surfacedata_t *exit_surface{ g_csgo.m_physics_surface_props->GetSurfaceData( exit_trace.surface.surfaceProps ) };

    const u16  enter_material{ enter_surface->game.material };
    const u16  exit_material{ exit_surface->game.material };
    const bool hit_grate{ ( enter_trace.contents & CONTENTS_GRATE ) != 0 };
    const bool no_draw{ ( enter_trace.surface.flags & SURF_NODRAW ) != 0 };

    float penetration_modifier{ };
    float damage_lost_percent{ 0.16f };

    if ( hit_grate || no_draw || enter_material == CHAR_TEX_GLASS || enter_material == CHAR_TEX_GRATE )
    {
        if ( enter_material == CHAR_TEX_GLASS || enter_material == CHAR_TEX_GRATE )
        {
            penetration_modifier = 3.0f;
            damage_lost_percent  = 0.05f;
        } else
        {
            penetration_modifier = 1.0f;
        }
    } else if ( enter_material == CHAR_TEX_FLESH &&
                g_csgo.ff_damage_reduction_bullets->GetFloat( ) == 0.0f &&
                enter_trace.m_pEnt && enter_trace.m_pEnt->IsPlayer( ) &&
                !shooter->IsOtherEnemy( enter_trace.m_pEnt->As< C_CSPlayer * >( ) ) )
    {
        penetration_modifier = g_csgo.ff_damage_bullet_penetration->GetFloat( );
        if ( penetration_modifier == 0.0f )
        {
            return true;
        }
    } else
    {
        penetration_modifier = ( enter_surface->game.penetrationModifier + exit_surface->game.penetrationModifier ) * 0.5f;
    }

    if ( enter_material == exit_material )
    {
        if ( exit_material == CHAR_TEX_WOOD || exit_material == CHAR_TEX_CARDBOARD )
        {
            penetration_modifier = 3.0f;
        } else if ( exit_material == CHAR_TEX_PLASTIC )
        {
            penetration_modifier = 2.0f;
        }
    }

    const float thickness{ ( exit_trace.endpos - enter_trace.endpos ).length( ) };
    const float modifier{ math::bigger( 1.0f / penetration_modifier, 0.0f ) };

    const float lost{ math::bigger( 3.0f / bullet.m_penetration * 1.25f, 0.0f ) * ( modifier * 3.0f ) +
                      damage * damage_lost_percent + thickness * thickness * modifier * 0.041666668f };

    const float impact_damage{ damage };

    damage -= math::bigger( lost, 0.0f );
    if ( damage < 1.0f )
    {
        return true;
    }

    hit.m_walls[ hit.m_wall_count++ ] = wall_t{ enter_trace.endpos, exit_trace.endpos, impact_damage };

    start = exit_trace.endpos;
    --penetrations;
    return false;
}

bool autowall::setup( C_CSPlayer *shooter, bullet_t &bullet ) const
{
    C_BaseCombatWeapon *weapon{ shooter->m_hActiveWeapon( ).Get( ) };
    if ( !weapon )
    {
        return false;
    }

    const CCSWeaponInfo *info{ weapon->GetCSWpnData( ) };
    if ( !info )
    {
        return false;
    }

    const bool shock{ weapon->m_iItemDefinitionIndex( ) == WEAPON_TASER };
    if ( !shock && ( info->m_WeaponType < WEAPONTYPE_PISTOL || info->m_WeaponType > WEAPONTYPE_MACHINEGUN ) )
    {
        return false;
    }

    bullet.m_start          = shooter->Weapon_ShootPosition( );
    bullet.m_damage         = static_cast< float >( info->m_iDamage );
    bullet.m_range          = info->m_flRange;
    bullet.m_range_modifier = info->m_flRangeModifier;
    bullet.m_penetration    = info->m_flPenetration;
    bullet.m_armor_ratio    = info->m_flArmorRatio;
    bullet.m_shock          = shock;
    return true;
}

bool autowall::fire( C_CSPlayer *shooter, const bullet_t &bullet, const C_CSPlayer *target, bullet_hit_t &hit ) const
{
    hit = bullet_hit_t{ };

    vec3 direction{ bullet.m_direction };
    math::vector_normalize( direction );

    vec3  start{ bullet.m_start };
    float damage{ bullet.m_damage };
    float distance{ };
    i32   penetrations{ MAX_PENETRATIONS };

    C_BaseEntity *last_hit{ };

    while ( damage > 0.0f )
    {
        const float remaining{ bullet.m_range - distance };
        const vec3  end{ start + direction * remaining };

        CTraceFilterSkipTwoEntities filter{ shooter, last_hit };

        trace_t trace{ };
        g_cl.trace_line( start, end, CS_MASK_SHOOT | CONTENTS_HITBOX, filter.filter( ), trace );
        g_cl.clip_trace_to_players( start, end + direction * PLAYER_RAY_EXTENSION, CS_MASK_SHOOT | CONTENTS_HITBOX,
                                    filter.filter( ), trace, 0.0f );

        last_hit  = trace.m_pEnt ? trace.m_pEnt->MyCombatCharacterPointer( ) : nullptr;
        hit.m_end = trace.endpos;

        if ( trace.fraction == 1.0f )
        {
            return false;
        }

        const surfacedata_t *surface{ g_csgo.m_physics_surface_props->GetSurfaceData( trace.surface.surfaceProps ) };

        distance += trace.fraction * remaining;
        damage *= math::pow( bullet.m_range_modifier, distance * 0.002f );

        if ( ( distance > PENETRATION_DISTANCE && bullet.m_penetration > 0.0f ) ||
             static_cast< double >( surface->game.penetrationModifier ) < 0.1 )
        {
            penetrations = 0;
        }

        hit.m_impact_damage = damage;

        if ( trace.m_pEnt && trace.m_pEnt->IsPlayer( ) )
        {
            auto *player{ trace.m_pEnt->As< C_CSPlayer * >( ) };
            if ( !target || player == target )
            {
                hit.m_player   = player;
                hit.m_hitgroup = trace.hitgroup;
                hit.m_hitbox   = trace.hitbox;
                hit.m_damage   = this->scale_damage( this->armor_of( player ), damage, bullet.m_armor_ratio, trace.hitgroup, bullet.m_shock );
                return true;
            }
        }

        if ( handle_bullet_penetration( shooter, bullet, direction, trace, surface, penetrations, start, damage, hit ) )
        {
            return false;
        }
    }

    return false;
}

ATLAS_FORCE_INLINE bool is_armored( const armor_t &armor, i32 hitgroup )
{
    if ( armor.m_value <= 0 )
    {
        return false;
    }

    switch ( hitgroup )
    {
        case HITGROUP_GENERIC:
        case HITGROUP_CHEST:
        case HITGROUP_STOMACH:
        case HITGROUP_LEFTARM:
        case HITGROUP_RIGHTARM: return true;
        case HITGROUP_HEAD: return armor.m_helmet || armor.m_heavy;
        default: return armor.m_heavy;
    }
}

armor_t autowall::armor_of( C_CSPlayer *player ) const
{
    const bool ct{ player->m_iTeamNum( ) == TEAM_CT };
    return armor_t{ player->m_ArmorValue( ),
                    player->m_bHasHelmet( ),
                    player->m_bHasHeavyArmor( ),
                    player->m_bGunGameImmunity( ),
                    ( ct ? g_csgo.mp_damage_scale_ct_head : g_csgo.mp_damage_scale_t_head )->GetFloat( ),
                    ( ct ? g_csgo.mp_damage_scale_ct_body : g_csgo.mp_damage_scale_t_body )->GetFloat( ) };
}

i32 autowall::scale_damage( const armor_t &armor, float damage, float armor_ratio, i32 hitgroup, bool shock ) const
{
    if ( armor.m_immune )
    {
        return 0;
    }

    if ( shock )
    {
        return static_cast< i32 >( damage );
    }

    switch ( hitgroup )
    {
        case HITGROUP_HEAD: damage = damage * 4.0f * ( armor.m_head_scale * ( armor.m_heavy ? 0.5f : 1.0f ) ); break;
        case HITGROUP_CHEST:
        case HITGROUP_LEFTARM:
        case HITGROUP_RIGHTARM: damage *= armor.m_body_scale; break;
        case HITGROUP_STOMACH: damage = damage * 1.25f * armor.m_body_scale; break;
        case HITGROUP_LEFTLEG:
        case HITGROUP_RIGHTLEG: damage = damage * 0.75f * armor.m_body_scale; break;
        default: break;
    }

    if ( !is_armored( armor, hitgroup ) )
    {
        return static_cast< i32 >( damage );
    }

    float ratio{ armor_ratio * 0.5f };
    float bonus{ 0.5f };
    float heavy_bonus{ 1.0f };

    if ( armor.m_heavy )
    {
        ratio *= 0.2f;

        if ( g_csgo.game_type->GetInt( ) == GAMETYPE_COOPERATIVE )
        {
            bonus       = 0.33f;
            heavy_bonus = 0.25f;
        }
    }

    const float value{ static_cast< float >( armor.m_value ) };

    float health{ damage * ratio };
    if ( ( damage - health ) * ( heavy_bonus * bonus ) > value )
    {
        health = damage - value / bonus;
    }

    return static_cast< i32 >( health );
}

#if defined( ATLAS_DEV )
constexpr color_t DEBUG_WALL_COLOR{ 245, 190, 40 };
constexpr color_t DEBUG_HIT_COLOR{ 126, 196, 107 };
constexpr color_t DEBUG_STOP_COLOR{ 239, 75, 72 };

ATLAS_FORCE_INLINE const char *hitgroup_name( i32 hitgroup )
{
    switch ( hitgroup )
    {
        case HITGROUP_HEAD: return ATLAS_XORSTR_STATIC( "head" );
        case HITGROUP_CHEST: return ATLAS_XORSTR_STATIC( "chest" );
        case HITGROUP_STOMACH: return ATLAS_XORSTR_STATIC( "stomach" );
        case HITGROUP_LEFTARM:
        case HITGROUP_RIGHTARM: return ATLAS_XORSTR_STATIC( "arm" );
        case HITGROUP_LEFTLEG:
        case HITGROUP_RIGHTLEG: return ATLAS_XORSTR_STATIC( "leg" );
        default: return ATLAS_XORSTR_STATIC( "generic" );
    }
}

void autowall::think( )
{
    this->m_debug_fired = false;

    bullet_t bullet{ };
    if ( !g_cfg.m_debug.m_autowall || !this->setup( g_cl.m_local, bullet ) )
    {
        return;
    }

    math::angle_vectors( g_cl.m_cmd->viewangles + g_cl.m_local->GetAimPunchAngleScaled( ), &bullet.m_direction );

    this->fire( g_cl.m_local, bullet, nullptr, this->m_debug_hit );
    this->m_debug_fired = true;
}

void autowall::paint( ) const
{
    if ( !g_cfg.m_debug.m_autowall || !this->m_debug_fired || !g_csgo.m_engine->IsInGame( ) || !g_cl.m_local ||
         !g_cl.m_local->IsAlive( ) )
    {
        return;
    }

    const bullet_hit_t &hit{ this->m_debug_hit };

    char text[ 32 ]{ };

    for ( i32 index{ }; index < hit.m_wall_count; ++index )
    {
        const wall_t &wall{ hit.m_walls[ index ] };

        vec2 enter{ };
        vec2 exit{ };
        if ( !g_cl.world_to_screen( wall.m_enter, enter ) || !g_cl.world_to_screen( wall.m_exit, exit ) )
        {
            continue;
        }

        g_draw.line( enter, exit, DEBUG_WALL_COLOR, 2.0f );
        g_draw.circle( enter, DEBUG_WALL_COLOR, 2.0f );

        str::format_to( text, sizeof( text ), ATLAS_XORSTR( "{}" ).get( ), static_cast< i32 >( math::ceil( wall.m_impact_damage ) ) );
        g_draw.text( ATLAS_XORSTR( "ESP Small" ).get( ), point_t{ static_cast< i32 >( enter.x ), static_cast< i32 >( enter.y ) + 4 },
                     DEBUG_WALL_COLOR, text, true, render::text_align::center );
    }

    vec2 end{ };
    if ( !g_cl.world_to_screen( hit.m_end, end ) )
    {
        return;
    }

    const point_t label{ static_cast< i32 >( end.x ), static_cast< i32 >( end.y ) + 4 };

    if ( !hit.m_player )
    {
        g_draw.circle( end, DEBUG_STOP_COLOR, 3.0f );

        str::format_to( text, sizeof( text ), ATLAS_XORSTR( "{}" ).get( ), static_cast< i32 >( math::ceil( hit.m_impact_damage ) ) );
        g_draw.text( ATLAS_XORSTR( "ESP Small" ).get( ), label, DEBUG_STOP_COLOR, text, true, render::text_align::center );
        return;
    }

    g_draw.circle( end, DEBUG_HIT_COLOR, 3.0f );

    str::format_to( text, sizeof( text ), ATLAS_XORSTR( "{} {} -{}" ).get( ), static_cast< i32 >( math::ceil( hit.m_impact_damage ) ),
                    hitgroup_name( hit.m_hitgroup ), hit.m_damage );
    g_draw.text( ATLAS_XORSTR( "ESP Small" ).get( ), label, DEBUG_HIT_COLOR, text, true, render::text_align::center );
}
#endif // ATLAS_DEV
