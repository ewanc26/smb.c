/* core/runners.h — the per-frame enemy object runners.
 *
 * smb.asm's RunEnemyObjectsCore jump table and its runner routines.
 * This atom starts with OffscreenBoundsCheck, the shared erasure
 * guard every runner tails into; the runners themselves and the
 * dispatch follow. */
#ifndef SMBC_CORE_RUNNERS_H
#define SMBC_CORE_RUNNERS_H

#include <stdint.h>

#include "state.h"

/* smb.asm: OffscreenBoundsCheck — erase the object when it strays
 * seventy-two pixels (fifty-six less for hammer bros and piranha
 * plants) beyond the left screen edge, or seventy-two beyond the
 * right edge, with exemptions for the spiny's egg state and several
 * objects, and never for the flying cheep-cheep */
void offscreen_bounds_check(CoreState *state, uint8_t x);

/* smb.asm: MoveNormalEnemy — the mover for most walker enemies:
 * gravity and horizontal movement by state, slowed movement for
 * d6-state objects, defeated enemies falling away, and stunned
 * enemies reviving on their interval timer (goombas are erased
 * instead when the timer reaches $0e) */
void move_normal_enemy(CoreState *state, uint8_t x);

/* smb.asm: MoveJumpingEnemy — gravity with the jumping amount, then
 * horizontal movement (green paratroopas and the like) */
void move_jumping_enemy(CoreState *state, uint8_t x);

/* smb.asm: ProcMoveRedPTroopa — the flying red paratroopa: nudged up
 * one pixel every eighth frame while at rest above its origin,
 * otherwise moved down or up towards its central vertical
 * coordinate */
void proc_move_red_p_troopa(CoreState *state, uint8_t x);

/* smb.asm: MoveFlyGreenPTroopa — the flying green paratroopa: the
 * primary/secondary movement counters ramp and reflect the X speed
 * for its sine-like horizontal flight (XMoveCntr_GreenPTroopa,
 * MoveWithXMCntrs), and every fourth frame it sways one pixel down
 * or up by the frame counter's d6 */
void move_fly_green_p_troopa(CoreState *state, uint8_t x);

/* smb.asm: MoveBloober — the bloober: defeated ones sink slowly;
 * otherwise the LSFR (masked by secondary hard mode) occasionally
 * picks a moving direction, ProcSwimmingB runs the swim cycle of
 * speeding up, slowing down and floating down towards the player,
 * and the movement force bobs the bloober up while its speed swims
 * it sideways (BlooperMoveSpeed aliases the X speed at $58,
 * BlooperMoveCounter the primary movement counter at $a0) */
void move_bloober(CoreState *state, uint8_t x);

/* smb.asm: MoveBulletBill — the frenzy bullet bill: defeated ones
 * drop with jumping gravity; otherwise a fixed $e8 leftward speed
 * (the cannon-fired bills move elsewhere) */
void move_bullet_bill(CoreState *state, uint8_t x);

/* smb.asm: MoveSwimmingCheepCheep — the underwater cheep-cheeps:
 * defeated ones sink slowly; otherwise a per-identifier horizontal
 * force drain drifts them left, and slots beyond the first two
 * oscillate vertically around their origin, the movement flag
 * flipping direction fifteen pixels out (CheepCheepMoveMFlag
 * aliases the X speed at $58) */
void move_swimming_cheep_cheep(CoreState *state, uint8_t x);

/* smb.asm: MovePodoboo — when the interval timer expires the
 * podoboo is reinitialised below the screen with a randomised
 * movement force and timer and a $f9 upward speed; gravity runs
 * every frame */
void move_podoboo(CoreState *state, uint8_t x);

/* smb.asm: ProcHammerBro — defeated bros fall; otherwise the jump
 * timer counts down, an on-screen bro with an expired throwing
 * timer spawns a hammer (d3 of the state marks the throw) and the
 * jump code picks a speed by screen position and LSFR, the length
 * timer by hard mode and more random bits. Every frame the bro
 * shimmies by the frame counter's d6, faces the player, and walks
 * left at them when the walking timer expires */
void proc_hammer_bro(CoreState *state, uint8_t x);

/* smb.asm: MovePiranhaPlant — any state or an unexpired frame
 * timer keeps the plant in its pipe; a resting plant reverses its
 * speed to emerge, but only when the player is at least $21 pixels
 * away horizontally (the low byte of PlayerEnemyDiff, negated when
 * the plant is left of the player). The plant then rises or falls
 * one pixel every other frame towards its up or down coordinate,
 * resting for $40 frames at each end. The background priority bit
 * is always set, for the inside-the-pipe illusion */
void move_piranha_plant(CoreState *state, uint8_t x);

/* smb.asm: MoveLakitu — a defeated lakitu falls; a spawning one
 * (any nonzero state) resets its direction and the frenzy buffer
 * and flies at $10. Otherwise the frenzy buffer holds the spiny
 * identifier and PlayerLakituDiff derives the speed: the capped
 * player distance, quartered, subtracted from a movement-class
 * adjustment, with the direction flipping beyond $3c pixels away.
 * The direction's d0 then negates the speed for leftward flight */
void move_lakitu(CoreState *state, uint8_t x);

/* smb.asm: MoveFlyingCheepCheep — the jumping cheep-cheep: a
 * defeated one clears its sprite attributes and falls with jumping
 * gravity; otherwise it moves horizontally, takes $0d gravity
 * capped at five, and wobbles its movement force up by $10 whenever
 * a pseudorandom subtraction against the vertical position lands
 * within eight pixels. The background priority lookup is residual
 * in the asm */
void move_flying_cheep_cheep(CoreState *state, uint8_t x);

/* smb.asm: EnemyMovementSubs — the jump engine over Enemy_ID for
 * objects $00-$14, dispatching to the movers above; ids $09 and
 * $13 are the asm's NoMoveCode dummies */
void enemy_movement_subs(CoreState *state, uint8_t x);

/* smb.asm: RunRetainerObj — offscreen bits, relative position,
 * and graphics for the mushroom retainer */
void run_retainer_obj(CoreState *state, uint8_t x);

/* smb.asm: RunNormalEnemies — clear the sprite attributes, then
 * the full per-frame pipeline: offscreen bits, relative position,
 * graphics, bounding box, background collision, enemy and player
 * collisions, movement (unless the master timer control is set),
 * and the offscreen bounds check */
void run_normal_enemies(CoreState *state, uint8_t x);

/* smb.asm: ProcBowserFlame — the flame's movement and its
 * three-sprite draw */
void proc_bowser_flame(CoreState *state, uint8_t x);

/* smb.asm: RunBowserFlame — process the flame, then offscreen
 * bits, relative position, bounding box, player collision, and
 * the offscreen bounds check */
void run_bowser_flame(CoreState *state, uint8_t x);

/* smb.asm: RunFireworks — tick the explosion, draw the frame, and
 * at the end award 500 points with the blast sound */
void run_fireworks(CoreState *state, uint8_t x);

/* smb.asm: RunStarFlagObj — clear the frenzy buffer and run the
 * current star flag task: fireworks count from the timer's last
 * digit, the 50-points-per-interval countdown, raising the star
 * with fireworks, then the delay to the area end */
void run_star_flag_obj(CoreState *state, uint8_t x);

/* smb.asm: JumpspringHandler — the spring's compression frames
 * carry the player, a fresh A press on the third frame loads the
 * high-bounce force, the fifth frame launches, and the frame
 * control steps every four frames */
void jumpspring_handler(CoreState *state, uint8_t x);

/* smb.asm: VineObjectHandler — grow, draw, and reap the vine in
 * the special enemy slot, and write the climbing metatile at its
 * tip once it tops out */
void vine_object_handler(CoreState *state, uint8_t x);

/* smb.asm: RunBowser — the full bowser frame: feet, facing, the
 * drift around the origin, jumps, hammers from world 6, the flame
 * spit with the mouth toggle, and the two-half draw */
void run_bowser(CoreState *state, uint8_t x);

/* smb.asm: BowserGfxHandler — draw the front, mirror the state into the
 * rear object offset by the facing, and draw that too; also the tail of
 * BridgeCollapse's RemoveBridge and MoveD_Bowser */
void bowser_gfx_handler(CoreState *state, uint8_t x);

/* smb.asm: PowerUpObjHandler — rise the power-up out of its
 * block, then move it by type with the draw, collide, and bounds
 * suite; the power-up always lives in the last enemy slot */
void power_up_obj_handler(CoreState *state);

/* smb.asm: RunFirebarObj — spin the bar, draw and collide each
 * fireball from the lookup rows, then the offscreen bounds */
void run_firebar_obj(CoreState *state, uint8_t x);

/* smb.asm: LargePlatformSubroutines — the per-object movement
 * dispatch for platforms $24-$2a */
void large_platform_subroutines(CoreState *state, uint8_t x);

/* smb.asm: RunEnemyObjectsCore — the enemy object dispatcher */
void run_enemy_objects_core(CoreState *state, uint8_t x);

/* smb.asm: WarpZoneObject — release the scroll lock and raise
 * the warp control once the player reaches the top */
void warp_zone_object(CoreState *state, uint8_t x);

/* smb.asm: RunSmallPlatform / RunLargePlatform */
void run_small_platform(CoreState *state, uint8_t x);
void run_large_platform(CoreState *state, uint8_t x);

#endif /* SMBC_CORE_RUNNERS_H */
