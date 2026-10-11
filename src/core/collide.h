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

#endif
