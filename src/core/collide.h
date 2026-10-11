/* src/core/collide.h — smb.asm's bounding box and collision layer:
 * the enemy bounding box computation and the collision detectors
 * built on it. */
#ifndef SMBC_CORE_COLLIDE_H
#define SMBC_CORE_COLLIDE_H

#include <stdint.h>

#include "state.h"

/* smb.asm: GetEnemyBoundBox — masks the offscreen bits against the
 * screen-relative position, parks the bounding box offscreen when
 * anything is masked, and otherwise computes the corners from the
 * relative position and the bound box control data, fixing up
 * coordinates that crossed a screen edge */
void get_enemy_bound_box(CoreState *state, uint8_t x);

/* smb.asm: GetEnemyBoundBoxOfs / GetEnemyBoundBoxOfsArg — the enemy
 * bounding box offset (slot * 4 + 4, past the player's) and the
 * low offscreen nybble, with a flag for all four bits set */
uint8_t get_enemy_bound_box_ofs(CoreState *state, uint8_t slot,
                                uint8_t *all_set);

/* smb.asm: ChkToStunEnemies — the shared stun setup for defeated
 * enemies: ids $09, $0e, $0f and $10 demote to plain koopa
 * (the LSB), the state takes d1 under the preserved high nybble,
 * the enemy lifts two pixels and gets the water-dependent
 * vertical speed, and the horizontal speed and direction face the
 * player — bullet bills keep their direction. A arrives as the
 * caller's accumulator: the asm's piranha-plant caller passes the
 * adjusted Y coordinate, which always fails the demote tests */
void chk_to_stun_enemies(CoreState *state, uint8_t x, uint8_t a);

/* smb.asm: SetStun — the stun body without the demote compares:
 * d1 under the preserved high nybble, two pixels up, the
 * water-dependent vertical speed, and the horizontal speed and
 * direction facing the player */
void set_stun(CoreState *state, uint8_t x);

/* smb.asm: SetupFloateyNumber — the score value from A with the
 * $30 timer and the enemy's position */
void setup_floatey_number(CoreState *state, uint8_t x, uint8_t score);

/* smb.asm: ShellOrBlockDefeat — the shell/bump kill: a piranha
 * plant sinks $18 pixels first, the enemy stuns (with the adjusted
 * Y coordinate in the accumulator, per the asm), takes the d5
 * defeat bit under its low five state bits, and scores 200 by
 * default, 1000 for a hammer bro or 100 for a goomba, with the
 * smack sound */
void shell_or_block_defeat(CoreState *state, uint8_t x);

/* smb.asm: SprObjectCollisionCore / PlayerCollisionCore — the
 * bounding box overlap test, horizontal then vertical, returning
 * nonzero on collision. x and y index the bounding box arrays by
 * coordinate (slot * 4); the player entry uses x = 0. The asm's
 * vertical wrap cases — a box whose bottom is above its top —
 * count as collisions */
uint8_t spr_object_collision_core(CoreState *state, uint8_t x, uint8_t y);
uint8_t player_collision_core(CoreState *state, uint8_t y);

/* smb.asm: EnemyTurnAround — reverse the horizontal speed and
 * invert the moving direction. Piranha plants, lakitus, hammer
 * bros and any other id >= $07 leave as-is; spinys and jumping
 * green paratroopas always turn */
void enemy_turn_around(CoreState *state, uint8_t x);

/* smb.asm: ProcEnemyCollisions — the reaction to an enemy-enemy
 * collision. With d5 set in either state nothing happens. A shell
 * (either state >= $06) kills the other enemy and chains the
 * shell chain counter into the score, unless the victim is a
 * hammer bro in an alt state; otherwise both enemies turn
 * around */
void proc_enemy_collisions(CoreState *state, uint8_t x, uint8_t y);

/* smb.asm: EnemiesCollision — the enemy-enemy collision sweep for
 * slot x. Runs on odd frames only, never in water areas, and
 * skips lakitus, piranha plants and anything id >= $15 on either
 * side. Every lower slot with its flag set and masked offscreen
 * bits clear gets the box comparison; the collision bits in the
 * slot enemy track which pairs already reacted, so only fresh
 * contacts (or anything with d7 set) run ProcEnemyCollisions */
void enemies_collision(CoreState *state, uint8_t x);

/* smb.asm: CheckPlayerVertical — nonzero when the player is
 * vertically onscreen above the $d0 pixel row */
uint8_t check_player_vertical(CoreState *state);

/* smb.asm: InitVStf — zero the enemy's vertical speed and move
 * force, returning zero */
uint8_t init_vstf(CoreState *state, uint8_t x);

/* smb.asm: EnemyFacePlayer — face the enemy towards the player,
 * returning the data offset (0 right, 1 left) */
uint8_t enemy_face_player(CoreState *state, uint8_t x);

/* smb.asm: EnemyStomped — the stomp kill for non-shell enemies:
 * the stomp sound, the per-id score (cheep-cheep, either bullet
 * bill and the unreachable podoboo case take 200; hammer bro 600;
 * lakitu 500; bloober 600; anything else goes through the koopa
 * demote), the stun with the direction preserved, d5 set, both
 * speeds zeroed and the player's $fd bounce */
void enemy_stomped(CoreState *state, uint8_t x);

/* smb.asm: ChkForDemoteKoopa — stomping a paratroopa (id >= $09)
 * demotes it to a plain troopa by the LSB, restores the normal
 * state, awards 400 points, faces it at the player and sends it
 * off at the demoted speed; anything lower is a stomped shell */
void chk_for_demote_koopa(CoreState *state, uint8_t x);

/* smb.asm: HandleStompedShellE — a stomped shell takes state $04,
 * the stomp chain counter increments into the score with the
 * stomp timer, the timer increments, the revival rate comes from
 * the hard mode flag, and the player bounces at $fc */
void handle_stomped_shell_e(CoreState *state, uint8_t x);

/* smb.asm: InjurePlayer / ForceInjury / KillPlayer — the injury
 * path. With the injury timer running nothing happens; otherwise
 * a big player shrinks with an $08 injury timer, the pipedown
 * sound and a palette refresh, while a small player dies: speed
 * halted, death music, $fc vertical speed. Both queue the $0a
 * ($0b for death) game engine subroutine with player state 1,
 * halt the timers and zero the scroll */
void injure_player(CoreState *state);
void force_injury(CoreState *state);

/* smb.asm: ChkForPlayerInjury — the stomp-versus-injury
 * decision. A player moving up (or an enemy below the player's
 * $0c-adjusted position for ids >= $07) can be injured; falling
 * or standing means the enemy is stomped. Returns nonzero when
 * the enemy was stomped */
uint8_t chk_for_player_injury(CoreState *state, uint8_t x);

/* smb.asm: HandlePowerUpCollision — collecting a power-up: the
 * object erases, 1000 points and the grab sound by default. A
 * star sets the $23 invincibility timer and queues its music, a
 * 1-up turns the floatey into an extra life, a mushroom or flower
 * grows a small player to super (subroutine $09) or a super
 * player to fiery (subroutine $0c) with a palette refresh */
void handle_power_up_collision(CoreState *state, uint8_t x);

/* smb.asm: HandlePECollisions — the player-enemy contact
 * reaction, dispatched on the enemy id and state: d0 of the
 * collision bits or any masked offscreen bits suppress repeats;
 * spinys, piranha plants, podoboos and bullet bills injure,
 * ids >= $15 injure, water areas injure, d7 shells injure,
 * stunned enemies (state 2-5) kick a non-goomba into a moving
 * shell with the chain score, and anything else goes through
 * the stomp-or-injury decision */
void handle_pe_collisions(CoreState *state, uint8_t x);

/* smb.asm: PlayerEnemyCollision — the player-enemy collision
 * detector for slot x, gated to even frames with the player
 * vertically onscreen, the enemy unmasked and the control
 * routine queued. A miss clears d0 of the collision bits; a hit
 * goes to the power-up grab, the star kill, or
 * HandlePECollisions */
void player_enemy_collision(CoreState *state, uint8_t x);

/* smb.asm: GetBlockBufferAddr — the block buffer address for a
 * column (0-31 across both buffers) into the $06/$07 scratch */
void get_block_buffer_addr(CoreState *state, uint8_t column);

/* smb.asm: BlockBufferCollision, entered as BlockBufferChk_Enemy
 * — the metatile at the enemy's position plus the adder pair at
 * y_ofs, with the row offset in $02, the low nybble of the
 * vertical (return_x = 0) or horizontal coordinate in $04, and
 * the buffer pointer in $06/$07. Returns the metatile (also in
 * $03) */
uint8_t block_buffer_chk_enemy(CoreState *state, uint8_t x, uint8_t y_ofs,
                               uint8_t return_x);

/* smb.asm: ChkUnderEnemy — the metatile under the enemy's bottom
 * middle */
uint8_t chk_under_enemy(CoreState *state, uint8_t x);

/* smb.asm: ChkForNonSolids — nonzero unless the metatile is a
 * vine blank ($26), either coin ($c2/$c3), or a hidden block
 * ($5f/$60) */
uint8_t chk_for_non_solids(uint8_t metatile);

/* smb.asm: SubtEnemyYPos — nonzero when the enemy's vertical
 * coordinate + 62 reaches 68 (wrapping), the asm's carry */
uint8_t subt_enemy_y_pos(CoreState *state, uint8_t x);

/* smb.asm: EnemyLanding — zero the vertical speed and force and
 * snap the vertical coordinate to the block, d3 set */
void enemy_landing(CoreState *state, uint8_t x);

/* smb.asm: RXSpd — negate the horizontal speed and invert the
 * moving direction, without EnemyTurnAround's id checks */
void rx_spd(CoreState *state, uint8_t x);

/* smb.asm: HammerBroJumpLData + SetHJ — a hammer bro jump: the
 * vertical speed, d0 of the state, a hard-mode-gated jump
 * length from the LSFR, and the jump timer */
void set_hj(CoreState *state, uint8_t x, uint8_t y_speed, uint8_t bitmask);

/* smb.asm: ChkForBump_HammerBroJ — the turn-or-jump reaction to
 * a wall or a red koopa edge: the bump sound for a d7 shell on
 * any slot but the special-use one, a hammer bro jumps ($fa,
 * zero bitmask), anything else turns around */
void chk_for_bump_hammer_bro_j(CoreState *state, uint8_t x);

/* smb.asm: LandEnemyInitState — land, then either clear d6 for
 * a d7 state or reinitialise to the walking state */
void land_enemy_init_state(CoreState *state, uint8_t x);

/* smb.asm: ProcEnemyDirection — pick the walking direction on
 * landing: goombas just land, spinys walk right unless an
 * eighth-frame lands them first, others face the player and
 * turn if the current direction already matches */
void proc_enemy_direction(CoreState *state, uint8_t x);

/* smb.asm: ChkLandedEnemyState — a landing state's reaction:
 * walking goes to the side check, the spiny's egg and stunned
 * states go to direction processing, state 2 becomes state 3
 * with the interval timer, states 3+ leave */
void chk_landed_enemy_state(CoreState *state, uint8_t x);

/* smb.asm: LandEnemyProperly — the landing branch: outside the
 * $08-$0c landing window it defers to ChkForRedKoopa, d6 lands,
 * d7 side-checks, and everything else goes through the landed
 * state dispatch */
void land_enemy_properly(CoreState *state, uint8_t x);

/* smb.asm: ChkForRedKoopa — a walking red koopa turns at the
 * edge; otherwise the state becomes d6 for a d7 shell or the
 * EnemyBGCStateData mapping otherwise */
void chk_for_red_koopa(CoreState *state, uint8_t x);

/* smb.asm: DoEnemySideCheck — probe the block at the side the
 * enemy moves towards ($00,$14) or ($10,$14); a solid one
 * triggers the bump reaction */
void do_enemy_side_check(CoreState *state, uint8_t x);

/* smb.asm: EnemyJump — the green paratroopa's bounce: past the
 * screen range check, falling at speed 1+, on solid ground,
 * land and spring back up at $fd, then the side check */
void enemy_jump(CoreState *state, uint8_t x);

/* smb.asm: KillEnemyAboveBlock — defeat the enemy like a shell
 * hit and send it upward at $fc */
void kill_enemy_above_block(CoreState *state, uint8_t x);

/* smb.asm: HammerBroBGColl — standing on solid ground with the
 * frame timer expired saves only d7/d3 of the state, lands and
 * side-checks; over the $23 blank the block defeat kills; with
 * nothing underneath d0 sets for jumping or falling */
void hammer_bro_bg_coll(CoreState *state, uint8_t x);

/* smb.asm: HandleEToBGCollision — solid ground under the enemy:
 * non-solids fall through to the red koopa check, other solids
 * land through LandEnemyProperly, and the $23 blank gets
 * erased from the buffer, defeats a goomba above the block,
 * awards 100 points to anything below $15, and runs the
 * stun-enemies comparisons */
void handle_e_to_bg_collision(CoreState *state, uint8_t x, uint8_t metatile);

/* smb.asm: EnemyToBGCollisionDet — the enemy background
 * collision entry: d6 states leave, the +62/68 range check
 * leaves, a spiny above $24 leaves, the jumping paratroopa and
 * the hammer bro take their own routines, ids >= $07 leave, and
 * the rest check underneath and land or edge-check */
void enemy_to_bg_collision_det(CoreState *state, uint8_t x);

/* smb.asm: EnemyGfxHandler — render the enemy's six sprites:
 * resolve the graphics-table offset through the per-object
 * exceptions (piranha movement, retainer/princess, cannon
 * bullet bill, jumpspring frames, podoboo flip, bowser halves,
 * goomba waddle, shells and eggs, hammer bro pose, the
 * bloober bob, animation timing, defeated flip), draw three
 * rows, mirror or flip per the symmetry rules, and park
 * offscreen columns and rows at $f8 */
void enemy_gfx_handler(CoreState *state, uint8_t x);

/* smb.asm: DrawPowerUp — two tile rows from the power-up type,
 * palette cycling and right-side flips for the flower and star,
 * then the shared offscreen check */
void draw_power_up(CoreState *state, uint8_t x);

#endif
