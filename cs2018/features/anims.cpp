#include "anims.hpp"
#include "bone_setup.hpp"
#include "lagcomp.hpp"
#include "ragebot.hpp"
#include "resolver.hpp"

#include "math/math.hpp"

ATLAS_NO_DESTROY animations g_anims{ };

// sv_maxusrcmdprocessticks' default (server.dll 0x10017D80): a longer gap is not one batch the server ran, the player
// was out of the snapshots.
constexpr i32 MAX_BATCH_TICKS{ 16 };

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L243
constexpr float CSGO_ANIM_DEPLOY_RATELIMIT{ 0.15f };

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/cs_shareddefs.cpp#L18
constexpr float CS_PLAYER_SPEED_STOPPED{ 1.0f };

// DT_Animationlayer's m_flCycle: 15 bits over 0..1, SPROP_ROUNDDOWN (server 0x101A55A0), so a step of 2^-15
// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/server/BaseAnimatingOverlay.cpp#L73
constexpr float LAYER_CYCLE_STEP{ 1.0f / 32768.0f };

ATLAS_FORCE_INLINE void save_layer( const C_AnimationLayer &layer, anim_layer_t &saved )
{
    saved.m_sequence          = layer.m_nSequence;
    saved.m_order             = layer.m_nOrder;
    saved.m_prev_cycle        = layer.m_flPrevCycle;
    saved.m_weight            = layer.m_flWeight;
    saved.m_weight_delta_rate = layer.m_flWeightDeltaRate;
    saved.m_playback_rate     = layer.m_flPlaybackRate;
    saved.m_cycle             = layer.m_flCycle;
}

ATLAS_FORCE_INLINE void load_layer( C_AnimationLayer &layer, const anim_layer_t &saved )
{
    layer.m_nSequence         = saved.m_sequence;
    layer.m_nOrder            = saved.m_order;
    layer.m_flPrevCycle       = saved.m_prev_cycle;
    layer.m_flWeight          = saved.m_weight;
    layer.m_flWeightDeltaRate = saved.m_weight_delta_rate;
    layer.m_flPlaybackRate    = saved.m_playback_rate;
    layer.m_flCycle           = saved.m_cycle;
}

// The networked layers drive the animstate's movement cycle and weight through the layer proxies
// (NotifyOnLayerChangeCycle/Weight); loading layers by hand has to do the same.
// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L553-L586
ATLAS_FORCE_INLINE void load_layers( C_CSPlayer *player, const anim_layer_t *layers )
{
    anim_overlay_t &overlays{ player->m_AnimOverlay( ) };
    for ( i32 index{ }; index < ANIMATION_LAYER_COUNT; ++index )
    {
        load_layer( overlays[ index ], layers[ index ] );
    }

    player->m_PlayerAnimState( )->m_flPrimaryCycle( ) = layers[ ANIMATION_LAYER_MOVEMENT_MOVE ].m_cycle;
    player->m_PlayerAnimState( )->m_flMoveWeight( )   = layers[ ANIMATION_LAYER_MOVEMENT_MOVE ].m_weight;
}

ATLAS_FORCE_INLINE bool is_client_layer( i32 layer )
{
    switch ( layer )
    {
        case ANIMATION_LAYER_AIMMATRIX:
        case ANIMATION_LAYER_MOVEMENT_MOVE:
        case ANIMATION_LAYER_MOVEMENT_STRAFECHANGE:
        case ANIMATION_LAYER_LEAN: return true;
        default: break;
    }

    return false;
}

ATLAS_FORCE_INLINE bool is_picked_layer( i32 layer )
{
    switch ( layer )
    {
        case ANIMATION_LAYER_ADJUST:
        case ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL:
        case ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB: return true;
        default: break;
    }

    return false;
}

// The layers the server picks and advances in its #ifndef CLIENT_DLL code and only networks: the local keeps an emulated
// copy of them, stepped with the server's formulas at its own commands.
ATLAS_FORCE_INLINE bool is_server_layer( i32 layer )
{
    switch ( layer )
    {
        case ANIMATION_LAYER_WEAPON_ACTION:
        case ANIMATION_LAYER_WEAPON_ACTION_RECROUCH:
        case ANIMATION_LAYER_ALIVELOOP: return true;
        default: break;
    }

    return false;
}

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L24-L38
ATLAS_FORCE_INLINE float clamp_cycle( float cycle )
{
    cycle -= static_cast< float >( static_cast< i32 >( cycle ) );
    if ( cycle < 0.0f )
    {
        cycle += 1.0f;
    } else if ( cycle > 1.0f )
    {
        cycle -= 1.0f;
    }

    return cycle;
}

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L1830-L1849
ATLAS_FORCE_INLINE void increment_layer_cycle( C_AnimationLayer &layer, float increment, bool allow_loop )
{
    if ( layer.m_flPlaybackRate == 0.0f )
    {
        return;
    }

    float cycle{ layer.m_flCycle + increment * layer.m_flPlaybackRate };
    if ( !allow_loop && cycle >= 1.0f )
    {
        cycle = 0.999f;
    }

    layer.m_flCycle = clamp_cycle( cycle );
}

// GetLayerIdealWeightFromSeqCycle; the sequence's fade in and fade out over its cycle
// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L1892-L1922
ATLAS_FORCE_INLINE float ideal_weight( CStudioHdr *studio, const C_AnimationLayer &layer )
{
    const mstudioseqdesc_t &seqdesc{ studio->pSeqdesc( layer.m_nSequence ) };
    const float             cycle{ layer.m_flCycle >= 0.999f ? 1.0f : layer.m_flCycle };

    float weight{ 1.0f };
    if ( seqdesc.fadeintime > 0.0f && cycle < seqdesc.fadeintime )
    {
        weight = math::smooth_step( cycle / seqdesc.fadeintime );
    } else if ( seqdesc.fadeouttime < 1.0f && cycle > seqdesc.fadeouttime )
    {
        weight = math::smooth_step( ( cycle - 1.0f ) / ( seqdesc.fadeouttime - 1.0f ) );
    }

    return weight < 0.0015f ? 0.0f : math::clamp( weight, 0.0f, 1.0f );
}

ATLAS_FORCE_INLINE bool has_activity_modifier( CStudioHdr *studio, i32 sequence, const char *name )
{
    const mstudioseqdesc_t &seqdesc{ studio->pSeqdesc( sequence ) };
    for ( i32 index{ }; index < seqdesc.numactivitymodifiers; ++index )
    {
        if ( str::equals( seqdesc.pActivityModifier( index )->pszName( ), name ) )
        {
            return true;
        }
    }

    return false;
}

// UpdateActivityModifiers, which every DoAnimationEvent runs before its own pick
// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L2699-L2714
ATLAS_FORCE_INLINE activity_modifiers_t current_modifiers( CCSGOPlayerAnimState *anim_state )
{
    return activity_modifiers_t{ anim_state->GetWeaponPrefix( ), anim_state->m_flSpeedAsPortionOfWalkTopSpeed( ) > 0.25f,
                                 anim_state->m_flAnimDuckAmount( ) > 0.55f };
}

ATLAS_FORCE_INLINE bool in_modifiers( const activity_modifiers_t &modifiers, const char *name )
{
    return str::equals( name, modifiers.m_weapon_prefix ) || ( modifiers.m_moving && str::equals( name, ATLAS_XORSTR( "moving" ).get( ) ) ) ||
           ( modifiers.m_crouch && str::equals( name, ATLAS_XORSTR( "crouch" ).get( ) ) );
}

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/animation.cpp#L433-L527
ATLAS_FORCE_INLINE i32 select_sequence( CStudioHdr *studio, i32 activity, const activity_modifiers_t &modifiers )
{
    i32 selected{ -1 };
    i32 best_score{ -1 };
    for ( i32 sequence{ }; sequence < studio->GetNumSeq( ); ++sequence )
    {
        if ( studio->GetSequenceActivity( sequence ) != activity )
        {
            continue;
        }

        const mstudioseqdesc_t &seqdesc{ studio->pSeqdesc( sequence ) };
        i32                     score{ };
        for ( i32 index{ }; index < seqdesc.numactivitymodifiers; ++index )
        {
            score += in_modifiers( modifiers, seqdesc.pActivityModifier( index )->pszName( ) ) ? 2 : -1;
        }

        if ( score > best_score || ( score == best_score && selected < 0 ) )
        {
            selected   = sequence;
            best_score = score;
        }
    }

    return selected;
}

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L1868-L1890
ATLAS_FORCE_INLINE void set_layer_sequence( C_CSPlayer *player, i32 index, i32 sequence )
{
    if ( sequence <= 1 )
    {
        return;
    }

    C_AnimationLayer &layer{ player->m_AnimOverlay( )[ index ] };
    layer.m_nSequence      = sequence;
    layer.m_flPlaybackRate = player->GetLayerSequenceCycleRate( &layer, sequence );
    layer.m_flCycle        = 0.0f;
    layer.m_flWeight       = 0.0f;
    player->m_PlayerAnimState( )->UpdateLayerOrderPreset( index, sequence );
}

bool animations::replay_velocity( const C_BaseEntity *entity, vec3 &velocity ) const
{
    if ( entity != this->m_replay_player )
    {
        return false;
    }

    velocity = vec3{ this->m_replay_velocity.x, this->m_replay_velocity.y, 0.0f };
    return true;
}

const remote_anim_record_t *animations::record( i32 index, i32 tick ) const
{
    const remote_anim_record_t &record{ this->m_records[ index ] };
    return record.m_tick == tick ? &record : nullptr;
}

const desync_edges_t *animations::edges( i32 index, i32 tick ) const
{
    const desync_edges_t &edges{ this->m_edges[ index ] };
    return edges.m_tick == tick ? &edges : nullptr;
}

const local_anim_state_t &animations::local_state( ) const
{
    return this->m_local;
}

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L266
void animations::step( C_CSPlayer *player, const vec3 &angles ) const
{
    CCSGOPlayerAnimState *anim_state{ player->m_PlayerAnimState( ) };
    anim_state->m_nLastUpdateFrame( ) = g_csgo.m_globals->framecount - 1;
    anim_state->Update( angles.y, angles.x );
}

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L2353-L2377
void animations::update_lower_body( CCSGOPlayerAnimState *anim_state, bool was_on_ground )
{
    if ( !was_on_ground )
    {
        return;
    }

    const float curtime{ g_csgo.m_globals->curtime };
    if ( anim_state->m_flVelocityLengthXY( ) > 0.1f )
    {
        this->m_local.m_lower_body_realign_time = curtime + CSGO_ANIM_LOWER_REALIGN_DELAY * 0.2f;
        this->m_local.m_lower_body_yaw          = anim_state->m_flEyeYaw( );
        return;
    }

    if ( curtime > this->m_local.m_lower_body_realign_time &&
         math::abs( math::normalize_yaw( anim_state->m_flFootYaw( ) - anim_state->m_flEyeYaw( ) ) ) > 35.0f )
    {
        this->m_local.m_lower_body_realign_time = curtime + CSGO_ANIM_LOWER_REALIGN_DELAY;
        this->m_local.m_lower_body_yaw          = anim_state->m_flEyeYaw( );
    }
}

void animations::on_create_move( i32 command )
{
    this->m_batch_starts[ command % MULTIPLAYER_BACKUP ] = g_csgo.m_client_states[ 0 ]->chokedcommands == 0 ? command : 0;
}

// CCSGameMovement::CheckJumpButton jumped in a command predicted for the first time; its update_local takes it
void animations::on_jump( )
{
    this->m_jump_pending = true;
}

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/server/cstrike15/cs_player.cpp#L4192-L4201
void animations::update_local( C_CSPlayer *local )
{
    const bool jumped{ this->m_jump_pending };
    this->m_jump_pending = false;

    if ( !local->IsAlive( ) )
    {
        this->m_local = local_anim_state_t{ };
        return;
    }

    const CUserCmd *cmd{ local->m_pCurrentCommand( ) };
    if ( !g_csgo.m_prediction->m_bFirstTimePredicted )
    {
        return;
    }

    local_record_t &record{ this->m_local_records[ cmd->command_number % MULTIPLAYER_BACKUP ] };
    record           = local_record_t{ };
    record.m_command = cmd->command_number;

    C_BaseCombatWeapon *weapon{ local->m_hActiveWeapon( ).Get( ) };
    if ( this->m_local.m_player == local && weapon != this->m_local.m_weapon )
    {
        record.m_deploy = true;

        this->swap_server_layers( local );
        this->deploy( local, current_modifiers( local->m_PlayerAnimState( ) ) );
        this->swap_server_layers( local );
    }
    this->m_local.m_weapon = weapon;

    // https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L2664-L2669
    if ( jumped && this->m_local.m_player == local )
    {
        C_AnimationLayer      &jump{ local->m_AnimOverlay( )[ ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL ] };
        const C_AnimationLayer networked{ jump };
        this->m_local.m_modifiers = current_modifiers( local->m_PlayerAnimState( ) );
        this->m_local.m_jumping   = true;
        load_layer( jump, this->m_local.m_layers[ ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL ] );
        set_layer_sequence( local, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL,
                            select_sequence( local->m_pStudioHdr( ), ACT_CSGO_JUMP, this->m_local.m_modifiers ) );
        save_layer( jump, this->m_local.m_layers[ ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL ] );
        jump = networked;
    }

    if ( this->m_batch_starts[ cmd->command_number % MULTIPLAYER_BACKUP ] == cmd->command_number )
    {
        record.m_batch_start = true;
        this->step_local( local, cmd, record );
    }

    record.m_lower_body_yaw      = this->m_local.m_lower_body_yaw;
    record.m_modifiers           = this->m_local.m_modifiers;
    record.m_deploy_rate_limited = this->m_local.m_deploy_rate_limited;
    for ( i32 index{ }; index < ANIMATION_LAYER_COUNT; ++index )
    {
        if ( is_picked_layer( index ) )
        {
            record.m_sequences[ index ] = this->m_local.m_layers[ index ].m_sequence;
            record.m_cycles[ index ]    = this->m_local.m_layers[ index ].m_cycle;
        }
    }
}

void animations::animate_local( C_CSPlayer *local, const CUserCmd *cmd )
{
    CCSGOPlayerAnimState *anim_state{ local->m_PlayerAnimState( ) };
    const float           networked_lower_body_yaw{ local->m_flLowerBodyYawTarget( ) };
    local->m_flLowerBodyYawTarget( ) = this->m_local.m_lower_body_yaw;

    const movement_before_t before{ this->pick_movement_layers( local ) };

    this->m_replay_player      = local;
    this->m_replay_velocity    = local->m_vecVelocity( );
    this->m_local_step         = anim_state;
    this->m_local_step_buttons = cmd->buttons;
    this->step( local, cmd->viewangles );
    this->m_local_step    = nullptr;
    this->m_replay_player = nullptr;

    this->settle_movement_layers( local, before );
    this->idle_adjust( local, before );

    this->update_lower_body( anim_state, before.m_on_ground );
    local->m_flLowerBodyYawTarget( ) = networked_lower_body_yaw;
}

void animations::step_local( C_CSPlayer *local, const CUserCmd *cmd, local_record_t &record )
{
    anim_overlay_t &overlays{ local->m_AnimOverlay( ) };

    CCSGOPlayerAnimState *anim_state{ local->m_PlayerAnimState( ) };
    const bool            spawned{ anim_state->m_bFirstRunSinceInit( ) };
    const bool            reseeded{ this->m_local.m_player != local || spawned };
    if ( reseeded )
    {
        this->m_local                  = local_anim_state_t{ };
        this->m_local.m_player         = local;
        this->m_local.m_weapon         = local->m_hActiveWeapon( ).Get( );
        this->m_local.m_lower_body_yaw = local->m_flLowerBodyYawTarget( );
        this->m_local.m_modifiers      = current_modifiers( anim_state );
        for ( i32 index{ }; index < ANIMATION_LAYER_COUNT; ++index )
        {
            this->m_local.m_server_layers[ index ] = overlays[ index ];
            save_layer( overlays[ index ], this->m_local.m_layers[ index ] );
        }
    }

    C_AnimationLayer networked_layers[ ANIMATION_LAYER_COUNT ]{ };
    for ( i32 index{ }; index < ANIMATION_LAYER_COUNT; ++index )
    {
        networked_layers[ index ] = overlays[ index ];
        if ( is_picked_layer( index ) )
        {
            load_layer( overlays[ index ], this->m_local.m_layers[ index ] );
        }
    }

    void *const previous_weapon{ anim_state->m_pWeaponLast( ) };
    local->m_flThirdpersonRecoil( ) = local->m_aimPunchAngle( ).x;

    if ( spawned )
    {
        const float curtime{ g_csgo.m_globals->curtime };
        g_csgo.m_globals->curtime      = curtime - TICK_INTERVAL;
        this->m_local.m_lower_body_yaw = 0.0f;
        this->animate_local( local, cmd );
        g_csgo.m_globals->curtime = curtime;
    } else if ( reseeded )
    {
        anim_state->m_flLastUpdateTime( ) = g_csgo.m_globals->curtime - TICK_INTERVAL;
    }

    this->animate_local( local, cmd );

    for ( i32 index{ }; index < ANIMATION_LAYER_COUNT; ++index )
    {
        if ( is_client_layer( index ) || is_picked_layer( index ) )
        {
            save_layer( overlays[ index ], this->m_local.m_layers[ index ] );
        } else if ( is_server_layer( index ) )
        {
            overlays[ index ] = networked_layers[ index ];
        }
    }

    this->m_local.m_primary_cycle = anim_state->m_flPrimaryCycle( );
    this->m_local.m_move_weight   = anim_state->m_flMoveWeight( );

    record.m_increment      = anim_state->m_flLastUpdateIncrement( );
    record.m_duck_amount    = anim_state->m_flAnimDuckAmount( );
    record.m_run_portion    = anim_state->m_flSpeedAsPortionOfRunTopSpeed( );
    record.m_weapon_changed = anim_state->m_pWeapon( ) && anim_state->m_pWeapon( ) != previous_weapon;

    this->swap_server_layers( local );
    this->weapon_action( local, record.m_increment, record.m_duck_amount );
    this->alive_loop( local, record.m_increment, record.m_run_portion, record.m_weapon_changed );
    g_bone_mgr.store_pose( local, g_csgo.m_globals->curtime );
    this->swap_server_layers( local );

    for ( i32 index{ }; index < ANIMATION_LAYER_COUNT; ++index )
    {
        if ( is_picked_layer( index ) )
        {
            overlays[ index ] = networked_layers[ index ];
        }
    }
}

void animations::swap_server_layers( C_CSPlayer *local )
{
    anim_overlay_t &overlays{ local->m_AnimOverlay( ) };
    for ( i32 index{ }; index < ANIMATION_LAYER_COUNT; ++index )
    {
        if ( is_server_layer( index ) )
        {
            const C_AnimationLayer held{ overlays[ index ] };
            overlays[ index ]                      = this->m_local.m_server_layers[ index ];
            this->m_local.m_server_layers[ index ] = held;
        }
    }
}

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L2650-L2662
void animations::deploy( C_CSPlayer *local, const activity_modifiers_t &modifiers )
{
    this->m_local.m_modifiers = modifiers;

    CStudioHdr             *studio{ local->m_pStudioHdr( ) };
    const C_AnimationLayer &action{ local->m_AnimOverlay( )[ ANIMATION_LAYER_WEAPON_ACTION ] };
    if ( studio->GetSequenceActivity( action.m_nSequence ) == ACT_CSGO_DEPLOY && action.m_flCycle < CSGO_ANIM_DEPLOY_RATELIMIT )
    {
        this->m_local.m_deploy_rate_limited = true;
        return;
    }

    set_layer_sequence( local, ANIMATION_LAYER_WEAPON_ACTION, select_sequence( studio, ACT_CSGO_DEPLOY, modifiers ) );
}

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L1219-L1301
void animations::weapon_action( C_CSPlayer *local, float increment, float duck_amount )
{
    CStudioHdr       *studio{ local->m_pStudioHdr( ) };
    anim_overlay_t   &overlays{ local->m_AnimOverlay( ) };
    C_AnimationLayer &action{ overlays[ ANIMATION_LAYER_WEAPON_ACTION ] };
    C_AnimationLayer &recrouch{ overlays[ ANIMATION_LAYER_WEAPON_ACTION_RECROUCH ] };

    bool advance{ true };
    if ( this->m_local.m_deploy_rate_limited && studio->GetSequenceActivity( action.m_nSequence ) == ACT_CSGO_DEPLOY &&
         action.m_flCycle >= CSGO_ANIM_DEPLOY_RATELIMIT )
    {
        this->m_local.m_deploy_rate_limited = false;
        set_layer_sequence( local, ANIMATION_LAYER_WEAPON_ACTION, select_sequence( studio, ACT_CSGO_DEPLOY, this->m_local.m_modifiers ) );
        action.m_flWeight = 0.0f;
        advance           = false;
    }

    const bool crouch_action{ action.m_flWeight > 0.0f && has_activity_modifier( studio, action.m_nSequence, ATLAS_XORSTR( "crouch" ).get( ) ) };
    if ( action.m_flWeight > 0.0f && recrouch.m_nSequence <= 0 &&
         ( local->m_PlayerAnimState( )->m_nAnimstateModelVersion( ) >= 2 || ( duck_amount > 0.0f && !crouch_action ) ) )
    {
        set_layer_sequence( local, ANIMATION_LAYER_WEAPON_ACTION_RECROUCH, local->LookupSequence( ATLAS_XORSTR( "recrouch_generic" ).get( ) ) );
    }

    float recrouch_weight{ };
    if ( local->m_PlayerAnimState( )->m_nAnimstateModelVersion( ) < 2 )
    {
        recrouch_weight = duck_amount > 0.0f && action.m_flWeight > 0.0f && !crouch_action ? action.m_flWeight * duck_amount : 0.0f;
    } else if ( action.m_flWeight > 0.0f )
    {
        recrouch_weight = crouch_action ? action.m_flWeight * ( 1.0f - duck_amount ) : action.m_flWeight * duck_amount;
    } else if ( recrouch.m_flWeight > 0.0f )
    {
        recrouch_weight = math::approach( recrouch.m_flWeight, 0.0f, increment * 4.0f );
    }
    recrouch.m_flWeight = math::clamp( recrouch_weight, 0.0f, 1.0f );

    if ( !advance )
    {
        return;
    }

    increment_layer_cycle( action, increment, false );
    const float previous_weight{ action.m_flWeight };
    action.m_flWeight = ideal_weight( studio, action );
    if ( increment != 0.0f )
    {
        action.m_flWeightDeltaRate = ( action.m_flWeight - previous_weight ) / increment;
    }
}

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L1163-L1213
void animations::alive_loop( C_CSPlayer *local, float increment, float run_portion, bool weapon_changed )
{
    CStudioHdr       *studio{ local->m_pStudioHdr( ) };
    C_AnimationLayer &alive{ local->m_AnimOverlay( )[ ANIMATION_LAYER_ALIVELOOP ] };

    if ( studio->GetSequenceActivity( alive.m_nSequence ) == ACT_CSGO_ALIVE_LOOP )
    {
        if ( weapon_changed )
        {
            const float cycle{ alive.m_flCycle };
            set_layer_sequence( local, ANIMATION_LAYER_ALIVELOOP, select_sequence( studio, ACT_CSGO_ALIVE_LOOP, this->m_local.m_modifiers ) );
            alive.m_flCycle = cycle;
        } else if ( alive.m_flCycle + increment * alive.m_flPlaybackRate < 1.0f )
        {
            alive.m_flWeight = math::remap_clamped( run_portion, 0.55f, 0.9f, 1.0f, 0.0f );
        }
    }

    increment_layer_cycle( alive, increment, true );
}

void animations::adopt_movement_layers( C_CSPlayer *local, const local_record_t &anchor, i32 predicted )
{
    float since{ };
    for ( i32 command{ anchor.m_command + 1 };; ++command )
    {
        const local_record_t &record{ this->m_local_records[ command % MULTIPLAYER_BACKUP ] };
        if ( record.m_command != command )
        {
            break;
        }

        if ( record.m_batch_start )
        {
            since += record.m_increment;
        }
    }

    anim_overlay_t &overlays{ local->m_AnimOverlay( ) };
    for ( i32 index{ }; index < ANIMATION_LAYER_COUNT; ++index )
    {
        const C_AnimationLayer &networked{ overlays[ index ] };
        anim_layer_t           &ours{ this->m_local.m_layers[ index ] };
        const bool              same_pick{ ours.m_sequence == anchor.m_sequences[ index ] };
        if ( !is_picked_layer( index ) || anchor.m_command <= this->m_local.m_adopted_through[ index ] ||
             ( networked.m_nSequence == anchor.m_sequences[ index ] &&
               math::abs( networked.m_flCycle - anchor.m_cycles[ index ] ) <= LAYER_CYCLE_STEP ) || // in step
             ( same_pick && ours.m_cycle < anchor.m_cycles[ index ] ) ||                            // restarted
             ( !same_pick && ours.m_sequence != networked.m_nSequence ) )                           // picked again
        {
            continue;
        }

        C_AnimationLayer adopted{ networked };
        if ( !same_pick )
        {
            increment_layer_cycle( adopted, since, false );
        } else if ( ours.m_cycle > anchor.m_cycles[ index ] )
        {
            increment_layer_cycle( adopted, ( ours.m_cycle - anchor.m_cycles[ index ] ) / ours.m_playback_rate, false );
        }

        save_layer( adopted, ours );
        this->m_local.m_adopted_through[ index ] = predicted;
    }
}

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L1614-L1619
// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L1681-L1690
// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L1748-L1756
movement_before_t animations::pick_movement_layers( C_CSPlayer *local ) const
{
    CCSGOPlayerAnimState *anim_state{ local->m_PlayerAnimState( ) };
    CStudioHdr           *studio{ local->m_pStudioHdr( ) };

    movement_before_t before{ };
    before.m_adjust         = local->m_AnimOverlay( )[ ANIMATION_LAYER_ADJUST ];
    before.m_duration_still = anim_state->m_flDurationStill( );
    before.m_stutter_step   = anim_state->m_flStutterStep( );
    before.m_on_ground      = anim_state->m_bOnGround( );
    before.m_on_ladder      = anim_state->m_bOnLadder( );
    before.m_landing        = anim_state->m_bLanding( );

    const bool on_ground{ ( local->m_fFlags( ) & FL_ONGROUND ) != 0 };
    const bool on_ladder{ !on_ground && local->m_MoveType( ) == MOVETYPE_LADDER };
    const bool left_ladder{ before.m_on_ladder && !on_ladder };

    if ( on_ladder && !before.m_on_ladder )
    {
        set_layer_sequence( local, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB,
                            select_sequence( studio, ACT_CSGO_CLIMB_LADDER, this->m_local.m_modifiers ) );
    }

    if ( on_ground )
    {
        // the air time SetUpMovement zeroes right after
        if ( !before.m_landing && ( ( !anim_state->m_bFirstRunSinceInit( ) && !before.m_on_ground ) || left_ladder ) )
        {
            const i32 activity{ anim_state->m_flDurationInAir( ) > 1.0f ? ACT_CSGO_LAND_HEAVY : ACT_CSGO_LAND_LIGHT };
            set_layer_sequence( local, ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB, select_sequence( studio, activity, this->m_local.m_modifiers ) );
            before.m_land_started = true;
        }
    } else if ( !on_ladder && ( before.m_on_ground || left_ladder ) && !this->m_local.m_jumping )
    {
        set_layer_sequence( local, ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL, select_sequence( studio, ACT_CSGO_FALL, this->m_local.m_modifiers ) );
    }

    return before;
}

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L1694-L1698
// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L1734-L1739
void animations::settle_movement_layers( C_CSPlayer *local, const movement_before_t &before )
{
    if ( !( local->m_fFlags( ) & FL_ONGROUND ) )
    {
        return;
    }

    CCSGOPlayerAnimState *anim_state{ local->m_PlayerAnimState( ) };
    C_AnimationLayer     &land{ local->m_AnimOverlay( )[ ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB ] };
    if ( ( before.m_landing || before.m_land_started ) && local->m_pStudioHdr( )->GetSequenceActivity( land.m_nSequence ) != ACT_CSGO_CLIMB_LADDER )
    {
        this->m_local.m_jumping = false;
    }

    if ( !anim_state->m_bLanding( ) && !this->m_local.m_jumping && anim_state->m_flLadderWeight( ) <= 0.0f )
    {
        land.m_flWeight = 0.0f;
    }
}

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L2301-L2322
// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L2379-L2384
void animations::idle_adjust( C_CSPlayer *local, const movement_before_t &before )
{
    CCSGOPlayerAnimState *anim_state{ local->m_PlayerAnimState( ) };
    CStudioHdr           *studio{ local->m_pStudioHdr( ) };
    C_AnimationLayer     &adjust{ local->m_AnimOverlay( )[ ANIMATION_LAYER_ADJUST ] };
    const float           increment{ anim_state->m_flLastUpdateIncrement( ) };
    const bool            settled{ before.m_on_ground && !before.m_on_ladder && !before.m_landing };

    adjust = before.m_adjust;

    const bool stopped{ anim_state->m_flVelocityLengthXY( ) <= 0.0f && before.m_duration_still <= 0.0f };
    if ( !this->m_local.m_adjust_started && stopped && settled && before.m_stutter_step < 50.0f )
    {
        set_layer_sequence( local, ANIMATION_LAYER_ADJUST, select_sequence( studio, ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING, this->m_local.m_modifiers ) );
        this->m_local.m_adjust_started = true;
    }

    const i32 activity{ studio->GetSequenceActivity( adjust.m_nSequence ) };
    if ( activity == ACT_CSGO_IDLE_ADJUST_STOPPEDMOVING || activity == ACT_CSGO_IDLE_TURN_BALANCEADJUST )
    {
        const float previous_weight{ adjust.m_flWeight };
        if ( this->m_local.m_adjust_started && anim_state->m_flSpeedAsPortionOfCrouchTopSpeed( ) <= 0.25f )
        {
            increment_layer_cycle( adjust, increment, false );
            adjust.m_flWeight              = ideal_weight( studio, adjust );
            this->m_local.m_adjust_started = adjust.m_flCycle + increment * adjust.m_flPlaybackRate < 1.0f;
        } else
        {
            this->m_local.m_adjust_started = false;
            adjust.m_flWeight              = math::approach( previous_weight, 0.0f, increment * 5.0f );
        }

        if ( increment != 0.0f )
        {
            adjust.m_flWeightDeltaRate = ( adjust.m_flWeight - previous_weight ) / increment;
        }
    }

    if ( anim_state->m_flVelocityLengthXY( ) <= CS_PLAYER_SPEED_STOPPED && settled && increment > 0.0f &&
         math::normalize_yaw( anim_state->m_flFootYawLast( ) - anim_state->m_flFootYaw( ) ) / increment > CSGO_ANIM_READJUST_THRESHOLD )
    {
        set_layer_sequence( local, ANIMATION_LAYER_ADJUST, select_sequence( studio, ACT_CSGO_IDLE_TURN_BALANCEADJUST, this->m_local.m_modifiers ) );
        this->m_local.m_adjust_started = true;
    }
}

// https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L1452-L1490
void animations::set_up_movement( CCSGOPlayerAnimState *anim_state )
{
    if ( !this->m_replay_player || anim_state != this->m_replay_player->m_PlayerAnimState( ) )
    {
        return;
    }

    anim_state->m_flVelocityLengthZ( ) = this->m_replay_velocity.z;
    if ( anim_state != this->m_local_step )
    {
        return;
    }

    const i32   buttons{ this->m_local_step_buttons };
    const bool  forward{ ( buttons & IN_FORWARD ) != 0 };
    const bool  back{ ( buttons & IN_BACK ) != 0 };
    const bool  left{ ( buttons & IN_MOVELEFT ) != 0 };
    const bool  right{ ( buttons & IN_MOVERIGHT ) != 0 };
    const float yaw{ DEG2RAD( anim_state->m_flFootYaw( ) ) };
    const vec3 &direction{ anim_state->m_vecVelocityNormalizedNonZero( ) };
    const float walk{ anim_state->m_flSpeedAsPortionOfWalkTopSpeed( ) };

    // AngleVectors( 0, yaw, 0 ): forward ( cos, sin ), right ( sin, -cos )
    const float to_forward{ direction.x * math::cos( yaw ) + direction.y * math::sin( yaw ) };
    const float to_right{ direction.x * math::sin( yaw ) - direction.y * math::cos( yaw ) };

    this->m_local.m_player->m_bStrafing( ) = ( walk >= 0.73f && right && !left && to_right < -0.63f ) ||
                                             ( walk >= 0.73f && left && !right && to_right > 0.63f ) ||
                                             ( walk >= 0.65f && forward && !back && to_forward < -0.55f ) ||
                                             ( walk >= 0.65f && back && !forward && to_forward > 0.55f );
}

void animations::restore_local( )
{
    C_CSPlayer *local{ g_cl.m_local };
    if ( !this->m_local.m_player || this->m_local.m_player != local || !local->IsAlive( ) )
    {
        return;
    }

    const CClientState   *cl{ g_csgo.m_client_states[ 0 ] };
    const i32             acknowledged{ cl->command_ack };
    const local_record_t &anchor{ this->m_local_records[ acknowledged % MULTIPLAYER_BACKUP ] };
    if ( acknowledged > this->m_local.m_corrected_through && anchor.m_command == acknowledged &&
         anchor.m_lower_body_yaw != local->m_flLowerBodyYawTarget( ) )
    {
        this->m_local.m_lower_body_yaw    = local->m_flLowerBodyYawTarget( );
        this->m_local.m_corrected_through = cl->lastoutgoingcommand + cl->chokedcommands;
    }

    if ( anchor.m_command == acknowledged )
    {
        this->adopt_movement_layers( local, anchor, cl->lastoutgoingcommand + cl->chokedcommands );
    }

    anim_overlay_t &overlays{ local->m_AnimOverlay( ) };
    for ( i32 index{ }; index < ANIMATION_LAYER_COUNT; ++index )
    {
        if ( is_client_layer( index ) )
        {
            load_layer( overlays[ index ], this->m_local.m_layers[ index ] );
        }
    }

    local->m_PlayerAnimState( )->m_flPrimaryCycle( ) = this->m_local.m_primary_cycle;
    local->m_PlayerAnimState( )->m_flMoveWeight( )   = this->m_local.m_move_weight;

    if ( anchor.m_command != acknowledged )
    {
        return;
    }

    for ( i32 index{ }; index < ANIMATION_LAYER_COUNT; ++index )
    {
        if ( is_server_layer( index ) )
        {
            this->m_local.m_server_layers[ index ] = overlays[ index ];
        }
    }

    this->m_local.m_modifiers           = anchor.m_modifiers;
    this->m_local.m_deploy_rate_limited = anchor.m_deploy_rate_limited;

    this->swap_server_layers( local );
    for ( i32 command{ acknowledged + 1 };; ++command )
    {
        const local_record_t &record{ this->m_local_records[ command % MULTIPLAYER_BACKUP ] };
        if ( record.m_command != command )
        {
            break;
        }

        // the list the command's events left, a jump's among them
        this->m_local.m_modifiers = record.m_modifiers;
        if ( record.m_deploy )
        {
            this->deploy( local, record.m_modifiers );
        }

        if ( record.m_batch_start )
        {
            this->weapon_action( local, record.m_increment, record.m_duck_amount );
            this->alive_loop( local, record.m_increment, record.m_run_portion, record.m_weapon_changed );
        }
    }
    this->swap_server_layers( local );
}

void animations::replay( C_CSPlayer *player, const remote_anim_record_t &previous, remote_anim_record_t &current )
{
    const bool                  reset{ player->m_PlayerAnimState( )->m_bFirstRunSinceInit( ) };
    const i32                   gap{ current.m_tick - previous.m_tick };
    const bool                  continuous{ !reset && previous.m_player == player && previous.m_model == current.m_model && gap > 0 &&
                                            gap <= MAX_BATCH_TICKS };
    const remote_anim_record_t &start{ continuous ? previous : current };
    const i32                   first_tick{ continuous ? previous.m_tick + 1 : current.m_tick };

    globals_backup_t backup{ };
    backup.store( );

    g_csgo.m_globals->curtime   = TICKS_TO_TIME( first_tick );
    g_csgo.m_globals->tickcount = first_tick;
    g_csgo.m_globals->frametime = TICK_INTERVAL;

    if ( !continuous && !reset )
    {
        player->m_PlayerAnimState( )->m_flLastUpdateTime( ) = TICKS_TO_TIME( first_tick - 1 );
    }

    const i32   flags{ gap == 1 || !continuous ? current.m_flags : ( current.m_flags & ~FL_ONGROUND ) | ( previous.m_flags & FL_ONGROUND ) };
    const float move_rate{ current.m_layers[ ANIMATION_LAYER_MOVEMENT_MOVE ].m_playback_rate };

    this->m_replay_velocity = vec3{ };
    if ( continuous )
    {
        this->m_replay_velocity = ( current.m_origin - previous.m_origin ) / TICKS_TO_TIME( gap );

        // https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/gamemovement.cpp#L1501
        // https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/gamemovement.cpp#L1950
        if ( flags & FL_ONGROUND )
        {
            this->m_replay_velocity.z = 0.0f;
        } else if ( player->m_MoveType( ) == MOVETYPE_WALK )
        {
            this->m_replay_velocity.z += 0.5f * g_csgo.sv_gravity->GetFloat( ) * TICKS_TO_TIME( gap - 2 );
        }

        // https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L1435-L1449
        if ( move_rate == 0.0f )
        {
            this->m_replay_velocity.x = 0.0f;
            this->m_replay_velocity.y = 0.0f;
        }
    }

    // https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L2353-L2377
    const bool animated_ground{ player->m_PlayerAnimState( )->m_bOnGround( ) };
    const bool was_on_ground{ animated_ground && ( previous.m_flags & FL_ONGROUND ) };
    const bool moved{ move_rate > 0.0f && was_on_ground };
    const bool lower_body_changed{ current.m_lower_body_yaw != previous.m_lower_body_yaw };
    const bool snapshots_seen{ previous.m_snapshot >= this->m_snapshot_break };

    const bool lower_body_written{ lower_body_changed || ( moved && current.m_layers[ ANIMATION_LAYER_MOVEMENT_MOVE ].m_weight == 1.0f ) };

    const bool advanced{ current.m_layers[ ANIMATION_LAYER_ALIVELOOP ].m_cycle != previous.m_layers[ ANIMATION_LAYER_ALIVELOOP ].m_cycle };
    const bool clean{ continuous && advanced };

    current.m_yaw_class = YAW_UNKNOWN;
    if ( clean && gap == 1 )
    {
        current.m_yaw_class = YAW_NETWORKED;
    } else if ( clean && snapshots_seen && lower_body_written )
    {
        current.m_yaw_class = YAW_LOWER_BODY;
    } else if ( clean && snapshots_seen && moved )
    {
        current.m_yaw_class = YAW_MOVING;
    }

    const anim_layer_t &adjust{ current.m_layers[ ANIMATION_LAYER_ADJUST ] };
    const anim_layer_t &adjust_before{ previous.m_layers[ ANIMATION_LAYER_ADJUST ] };
    const bool          adjust_started{ adjust.m_cycle == 0.0f && adjust.m_weight == 0.0f &&
                                        ( adjust_before.m_sequence != adjust.m_sequence || adjust_before.m_cycle != 0.0f || adjust_before.m_weight != 0.0f ) };
    const bool          balance_adjust{ continuous && adjust_started &&
                                        player->m_pStudioHdr( )->GetSequenceActivity( adjust.m_sequence ) == ACT_CSGO_IDLE_TURN_BALANCEADJUST };

    const bool  batched{ continuous && ( gap > 1 || !advanced ) };
    const float networked{ batched ? previous.m_angles.y + math::normalize_yaw( current.m_angles.y - previous.m_angles.y ) / gap : current.m_angles.y };
    const float resolved{ g_resolver.on_batch( player, previous, current, networked, clean, animated_ground, snapshots_seen, balance_adjust ) };

    vec3 angles{ current.m_angles };
    if ( batched )
    {
        angles.x = previous.m_angles.x + ( current.m_angles.x - previous.m_angles.x ) / gap;
        angles.y = lower_body_changed || ( snapshots_seen && moved ) ? current.m_lower_body_yaw : resolved;
    }

    player->m_fFlags( )               = flags;
    player->m_flLowerBodyYawTarget( ) = reset ? 0.0f : start.m_lower_body_yaw;
    load_layers( player, start.m_layers );

    // https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L2328-L2350
    // https://github.com/perilouswithadollarsign/cstrike15_src/blob/master/game/shared/cstrike15/csgo_playeranimstate.cpp#L2378-L2384
    CCSGOPlayerAnimState *anim_state{ player->m_PlayerAnimState( ) };
    const float           increment{ TICKS_TO_TIME( first_tick ) - anim_state->m_flLastUpdateTime( ) };
    const float           turned{ math::normalize_yaw( anim_state->m_flEyeYaw( ) - angles.y ) };
    if ( balance_adjust )
    {
        if ( current.m_yaw_class == YAW_NETWORKED || current.m_yaw_class == YAW_LOWER_BODY ||
             turned > CSGO_ANIM_READJUST_THRESHOLD * increment - CSGO_ANIM_LOWER_CATCHUP_IDLE * math::bigger( increment, anim_state->m_flLastUpdateIncrement( ) ) )
        {
            anim_state->m_flFootYaw( ) = math::normalize_yaw( angles.y + 90.0f );
        } else if ( current.m_yaw_class == YAW_UNKNOWN )
        {
            anim_state->m_flFootYaw( ) = math::normalize_yaw( angles.y - 90.0f );
        }
    }

    const bool edged{ g_cl.m_local && g_cl.m_local->IsOtherEnemy( player ) && g_ragebot.uses_safe_point( ) };
    if ( edged )
    {
        atl::memcpy( this->m_state_before, anim_state, ANIMSTATE_SIZE );
    }

    this->m_replay_player = player;
    this->step( player, angles );
    this->m_replay_player = nullptr;

#if defined( ATLAS_DEV )
    if ( g_cfg.m_debug.m_lag_records )
    {
        DBGOUT( "RSLV idx=%d t=%d gap=%d class=%d yaw=%.4f eye=%.4f lby=%.4f rate=%.6f weight=%.4f ground=%d seen=%d foot=%.4f",
                player->entindex( ), current.m_tick, gap, static_cast< i32 >( current.m_yaw_class ), angles.y, current.m_angles.y,
                current.m_lower_body_yaw, move_rate, current.m_layers[ ANIMATION_LAYER_MOVEMENT_MOVE ].m_weight, was_on_ground, snapshots_seen,
                player->m_PlayerAnimState( )->m_flFootYaw( ) );
    }
#endif // ATLAS_DEV

    player->m_fFlags( )               = current.m_flags;
    player->m_flLowerBodyYawTarget( ) = current.m_lower_body_yaw;
    load_layers( player, current.m_layers );

    g_bone_mgr.store_pose( player, TICKS_TO_TIME( current.m_tick ) );

    if ( edged )
    {
        this->build_edges( player, start, current, angles, flags, reset ? 0.0f : start.m_lower_body_yaw );
    }

    backup.restore( );
}

void animations::build_edges( C_CSPlayer *player, const remote_anim_record_t &start, const remote_anim_record_t &current, const vec3 &angles,
                              i32 flags, float lower_body )
{
    CCSGOPlayerAnimState     *anim_state{ player->m_PlayerAnimState( ) };
    float                    *poses{ player->m_flPoseParameter( ) };
    const studiohdr_t        *header{ player->m_pStudioHdr( )->m_pStudioHdr };
    const mstudiohitboxset_t *set{ header->pHitboxSet( player->m_nHitboxSet( ) ) };
    desync_edges_t           &edges{ this->m_edges[ player->entindex( ) ] };

    edges.m_tick = 0;
    if ( !set )
    {
        return;
    }

    atl::memcpy( this->m_state_after, anim_state, ANIMSTATE_SIZE );
    atl::memcpy( this->m_poses_after, poses, sizeof( this->m_poses_after ) );

    bool built{ true };
    for ( i32 edge{ }; edge < DESYNC_EDGE_COUNT && built; ++edge )
    {
        load_layers( player, start.m_layers );
        atl::memcpy( anim_state, this->m_state_before, ANIMSTATE_SIZE );
        player->m_fFlags( )               = flags;
        player->m_flLowerBodyYawTarget( ) = lower_body;

        anim_state->m_flFootYaw( ) = math::normalize_yaw( angles.y + ( edge == 0 ? 90.0f : -90.0f ) );

        this->m_replay_player = player;
        this->step( player, angles );
        this->m_replay_player = nullptr;

        load_layers( player, current.m_layers );
        player->m_AnimOverlay( )[ ANIMATION_LAYER_LEAN ].m_flWeight = 0.0f;

        bone_setup::prepare_build( player );
        built = player->BaseSetupBones( nullptr, MAXSTUDIOBONES, BONE_USED_BY_HITBOX, TICKS_TO_TIME( current.m_tick ) );
        if ( built )
        {
            edges.m_poses[ edge ].build( set, player->m_CachedBoneData( ) );
        }

#if defined( ATLAS_DEV )
        if ( g_cfg.m_debug.m_lag_records )
        {
            DBGOUT( "EDGE idx=%d t=%d edge=%d yaw=%.4f foot=%.4f built=%d", player->entindex( ), current.m_tick, edge, angles.y,
                    anim_state->m_flFootYaw( ), built );
        }
#endif // ATLAS_DEV
    }

    player->m_fFlags( )               = current.m_flags;
    player->m_flLowerBodyYawTarget( ) = current.m_lower_body_yaw;
    load_layers( player, current.m_layers );
    atl::memcpy( anim_state, this->m_state_after, ANIMSTATE_SIZE );
    atl::memcpy( poses, this->m_poses_after, sizeof( this->m_poses_after ) );
    bone_setup::prepare_build( player );

    if ( built )
    {
        edges.m_tick = current.m_tick;
    }
}

void animations::update_others( )
{
    if ( !g_csgo.m_engine->IsInGame( ) )
    {
        return;
    }

    const i32 snapshot{ g_csgo.m_client_states[ 0 ]->m_nServerTick };
    if ( snapshot != this->m_snapshot && snapshot != this->m_snapshot + 1 )
    {
        this->m_snapshot_break = snapshot;
    }
    this->m_snapshot = snapshot;

    const i32 local_index{ g_csgo.m_engine->GetLocalPlayer( ) };
    const i32 highest{ math::smaller( g_csgo.m_globals->maxClients, MAX_TRACKED_PLAYERS - 1 ) };

    for ( i32 index{ 1 }; index <= highest; ++index )
    {
        remote_anim_record_t &previous{ this->m_records[ index ] };
        IClientEntity        *client_entity{ g_csgo.m_entity_list->GetClientEntity( index ) };
        C_CSPlayer           *player{ client_entity ? client_entity->As< C_CSPlayer * >( ) : nullptr };

        if ( index == local_index ||
             !player ||
             player->IsDormant( ) ||
             !player->IsAlive( ) ||
             player->m_AnimOverlay( ).count( ) < ANIMATION_LAYER_COUNT )
        {
            previous = remote_anim_record_t{ };
            continue;
        }

        remote_anim_record_t current{ };
        current.m_origin         = player->GetAbsOrigin( );
        current.m_angles         = player->m_angEyeAngles( );
        current.m_player         = player;
        current.m_lower_body_yaw = player->m_flLowerBodyYawTarget( );
        current.m_tick           = TIME_TO_TICKS( player->m_flSimulationTime( ) );
        current.m_flags          = player->m_fFlags( );
        current.m_model          = player->m_nModelIndex( );
        current.m_snapshot       = snapshot;

        if ( previous.m_player == player && current.m_tick == previous.m_tick )
        {
            continue;
        }

        anim_overlay_t &overlays{ player->m_AnimOverlay( ) };
        for ( i32 layer{ }; layer < ANIMATION_LAYER_COUNT; ++layer )
        {
            save_layer( overlays[ layer ], current.m_layers[ layer ] );
        }

        if ( previous.m_player == player && current.m_tick % TIMESTAMP_BASE == 0 &&
             current.m_layers[ ANIMATION_LAYER_ALIVELOOP ].m_cycle == previous.m_layers[ ANIMATION_LAYER_ALIVELOOP ].m_cycle )
        {
            continue;
        }

        this->replay( player, previous, current );
        previous = current;
    }
}
