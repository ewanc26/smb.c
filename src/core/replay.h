/* core/replay.h — deterministic input recording and replay (#37).
 *
 * Owns: the versioned recording format (frame-indexed controller
 * bitmaps plus a post-frame state hash), load/save with validation,
 * the state hash itself, and replay with first-divergence reporting.
 * Recordings are frame-indexed only — no wall clock anywhere — and
 * are distinct from save states (#25): a recording is inputs plus
 * expected hashes, replayed from a fresh reset.
 *
 * Not owned: the frontend flags that drive recording (--record /
 * --play live in host/frontend.c).
 *
 * Format v1 (all integers little-endian):
 *   char[8]  magic "SMBCRPLY"
 *   u16      version (= 1)
 *   u16      reserved (0)
 *   u32      frame count
 *   per frame: u8 player-1 bitmap, u8 player-2 bitmap, u64 state hash
 */
#ifndef SMBC_CORE_REPLAY_H
#define SMBC_CORE_REPLAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "frame.h"

#define SMBC_REPLAY_VERSION 1u
#define SMBC_REPLAY_NO_DIVERGENCE UINT32_MAX

typedef struct {
  uint8_t p1;
  uint8_t p2;
  uint64_t hash; /* state hash after this frame ran */
} SmbcReplayFrame;

typedef struct {
  SmbcReplayFrame *frames;
  uint32_t count;
  uint32_t cap;
} SmbcReplay;

/* FNV-1a over everything the simulation can mutate: RAM, the PPU's
 * registers, VRAM, OAM, scroll state and log, the APU register image
 * and both joypads. Host-supplied pointers (CHR, area/enemy streams)
 * are not simulation state and are not hashed. Deterministic for a
 * given frame sequence. */
uint64_t smbc_state_hash(const Smbc *smbc);

void replay_init(SmbcReplay *replay);
void replay_free(SmbcReplay *replay);

/* Append one frame; grows the buffer. False on allocation failure. */
bool replay_append(SmbcReplay *replay, uint8_t p1, uint8_t p2,
                   uint64_t hash);

/* Save/load. Load validates the magic, the version and the exact
 * length; on failure it returns false, leaves the replay empty and
 * writes a message to err (when non-NULL). */
bool replay_save(const SmbcReplay *replay, const char *path, char *err,
                 size_t err_len);
bool replay_load(SmbcReplay *replay, const char *path, char *err,
                 size_t err_len);

/* Replay from the current state (call on a fresh smbc_init), applying
 * the recorded inputs and checking the post-frame hash each frame.
 * stop_frame caps the run (pass the recording's count for a full
 * replay; a smaller value stops early). Returns the frames run. On
 * divergence the run stops: diverge_frame receives the frame index
 * and got/want the two hashes. On a clean run diverge_frame is
 * SMBC_REPLAY_NO_DIVERGENCE. */
uint32_t replay_run(Smbc *smbc, const SmbcReplay *replay,
                    uint32_t stop_frame, uint32_t *diverge_frame,
                    uint64_t *got, uint64_t *want);

#endif /* SMBC_CORE_REPLAY_H */
