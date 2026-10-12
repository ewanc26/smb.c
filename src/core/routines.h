/* core/routines.h — the GameRoutines tree.
 *
 * Owns: the thirteen GameEngineSubroutine entries — the area entrance
 * setup, the pipe and vine entries, the flagpole slide and end of
 * level walk, the lose-life, death, injury and size-change sequences,
 * and PlayerCtrlRoutine's controller split, bounding box, scroll and
 * hole handling — plus ScrollHandler and the small shared helpers
 * (AutoControlPlayer, ChgAreaMode, NextArea). PlayerCtrlRoutine's two
 * physics calls (PlayerMovementSubs, PlayerBGCollision) belong to the
 * player physics atom and are still pending translation.
 */
#ifndef SMBC_CORE_ROUTINES_H
#define SMBC_CORE_ROUTINES_H

#include <stdint.h>

struct Smbc;

/* smb.asm: Entrance_GameTimerSetup (routine 0) — position and flags
 * for the start of an area */
void entrance_game_timer_setup(struct Smbc *smbc);

/* smb.asm: Vine_AutoClimb (routine 1) */
void vine_auto_climb(struct Smbc *smbc);

/* smb.asm: SideExitPipeEntry (routine 2) */
void side_exit_pipe_entry(struct Smbc *smbc);

/* smb.asm: VerticalPipeEntry (routine 3) */
void vertical_pipe_entry(struct Smbc *smbc);

/* smb.asm: FlagpoleSlide (routine 4) */
void flagpole_slide(struct Smbc *smbc);

/* smb.asm: PlayerEndLevel (routine 5) */
void player_end_level(struct Smbc *smbc);

/* smb.asm: PlayerLoseLife (routine 6) */
void player_lose_life(struct Smbc *smbc);

/* smb.asm: PlayerEntrance (routine 7) */
void player_entrance(struct Smbc *smbc);

/* smb.asm: PlayerCtrlRoutine (routine 8) — the controller split,
 * bounding box, scroll handler and hole logic; the physics calls are
 * still pending translation */
void player_ctrl_routine(struct Smbc *smbc);

/* smb.asm: PlayerChangeSize (routine 9) */
void player_change_size(struct Smbc *smbc);

/* smb.asm: PlayerInjuryBlink (routine 10) */
void player_injury_blink(struct Smbc *smbc);

/* smb.asm: PlayerDeath (routine 11) */
void player_death(struct Smbc *smbc);

/* smb.asm: PlayerFireFlower (routine 12) */
void player_fire_flower(struct Smbc *smbc);

/* smb.asm: ScrollHandler — move the screen with the player's scroll
 * force, clamp the player at the screen edges */
void scroll_handler(struct Smbc *smbc);

/* smb.asm: ScrollScreen — scroll the screen y pixels (also called from
 * PlayerVictoryWalk) */
void scroll_screen(struct Smbc *smbc, uint8_t y);

/* smb.asm: AutoControlPlayer — override the controller bits and run
 * the player control routine with them (victory walk, flagpole) */
void auto_control_player(struct Smbc *smbc, uint8_t a);

#endif /* SMBC_CORE_ROUTINES_H */
