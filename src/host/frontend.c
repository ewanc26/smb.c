/* host/frontend.c — host entry point.
 *
 * The playable frontend (#5): window, presentation, input and the
 * fixed-step loop that paces the deterministic core. One core frame
 * runs per NES frame period (60.0988 Hz); the renderer composes the
 * PPU's state and the video module presents it at integer scale.
 * Simulation never depends on render rate — the core advances in
 * fixed steps and the host catches up (capped) when it falls behind.
 *
 * `--headless N` runs N frames without opening any device, for CI
 * and debugging; the default launch opens the game directly.
 *
 * Recording (#37): `--record PATH` captures frame-indexed inputs plus
 * post-frame state hashes while playing; `--play PATH [N]` replays a
 * recording headlessly and reports the first divergent frame, if any.
 *
 * Usage: smbc [--scale N] [--headless N] [--record PATH] [--play PATH [N]] */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "../core/frame.h"
#include "../core/replay.h"
#include "../core/runtime.h"
#include "../core/snapshot.h"
#include "../hw/render.h"
#include "audio.h"
#include "capture.h"
#include "input.h"
#include "keymap.h"
#include "padset.h"
#include "video.h"

/* NTSC 2C02 frame rate: the core runs one frame per period. */
#define SMBC_FRAME_NS 16639266ull /* 1e9 / 60.0988 */
#define SMBC_MAX_CATCHUP 4u       /* steps per present, then drop time */
#define SMBC_DEFAULT_SCALE 3u

/* this SDL2 predates SDL_GetTicksNS/SDL_DelayNS */
static uint64_t now_ns(void) {
  static uint64_t freq = 0;
  if (freq == 0) {
    freq = SDL_GetPerformanceFrequency();
  }
  return SDL_GetPerformanceCounter() * 1000000000ull / freq;
}

static int run_headless(unsigned frames) {
  Smbc smbc;
  smbc_init(&smbc);
  for (unsigned frame = 0; frame < frames; frame++) {
    smbc_frame(&smbc);
  }
  printf("smbc: ran %u frames (counter=%u)\n", frames,
         (unsigned)smbc.state.ram[SMBC_FRAME_COUNTER]);
  return 0;
}

static int run_play(const char *path, unsigned stop) {
  SmbcReplay replay;
  char err[128];
  if (!replay_load(&replay, path, err, sizeof(err))) {
    fprintf(stderr, "smbc: %s: %s\n", path, err);
    return 1;
  }
  Smbc smbc;
  smbc_init(&smbc);
  uint32_t diverge = SMBC_REPLAY_NO_DIVERGENCE;
  uint64_t got = 0, want = 0;
  uint32_t ran = replay_run(&smbc, &replay,
                            stop != 0 ? stop : replay.count, &diverge,
                            &got, &want);
  if (diverge == SMBC_REPLAY_NO_DIVERGENCE) {
    printf("smbc: %s: %u frames, no divergence\n", path, ran);
    replay_free(&replay);
    return 0;
  }
  fprintf(stderr,
          "smbc: %s: divergence at frame %u (hash 0x%016" PRIx64
          ", expected 0x%016" PRIx64 ")\n",
          path, diverge, got, want);
  replay_free(&replay);
  return 1;
}

/* Host-side controls. Keys are read here only; the simulation sees the
 * resulting runtime commands and the joypad bitmap. */
#define SMBC_REWIND_FRAMES 600u /* ten seconds at 60 Hz */
#define SMBC_STATE_PATH "smbc.sav"
#define SMBC_KEYS_PATH "smbc.keys"

/* Load the optional mapping file. A missing file keeps the defaults; a bad
 * one is reported and also keeps the defaults rather than half-applying. */
static void load_keymap(SmbcKeymap *map) {
  keymap_defaults(map);
  FILE *f = fopen(SMBC_KEYS_PATH, "rb");
  if (f == NULL) {
    return;
  }
  char text[4096];
  size_t n = fread(text, 1, sizeof(text) - 1, f);
  fclose(f);
  text[n] = '\0';
  char err[128];
  if (!keymap_parse(map, text, err, sizeof(err))) {
    keymap_defaults(map);
    fprintf(stderr, "smbc: %s: %s (using default controls)\n", SMBC_KEYS_PATH,
            err);
  } else {
    printf("smbc: controls loaded from %s\n", SMBC_KEYS_PATH);
  }
}

/* Open every controller present at start. */
static void scan_pads(SmbcPadSet *pads) {
  for (int i = 0; i < SDL_NumJoysticks() && pads->count < SMBC_MAX_PADS; i++) {
    SDL_GameController *c = input_open_pad(i);
    if (c != NULL) {
      padset_add(pads, input_pad_instance_id(c));
    }
  }
}

static void report_active_pad(const SmbcPadSet *pads) {
  if (padset_active_id(pads) < 0) {
    printf("smbc: no controller selected (keyboard only)\n");
  } else {
    printf("smbc: controller %u of %u active\n",
           (unsigned)padset_index_of(pads, padset_active_id(pads)) + 1u,
           (unsigned)pads->count);
  }
}

static void handle_key(SDL_Keycode key, SmbcRuntime *rt, bool *reset_now,
                       bool *save_now, bool *load_now, bool *shot_now,
                       bool *cycle_pad) {
  switch (key) {
  case SDLK_p:
    runtime_command(rt, SMBC_CMD_PAUSE_TOGGLE);
    break;
  case SDLK_n:
    runtime_command(rt, SMBC_CMD_STEP);
    break;
  case SDLK_EQUALS:
  case SDLK_KP_PLUS:
    runtime_command(rt, SMBC_CMD_SPEED_UP);
    break;
  case SDLK_MINUS:
  case SDLK_KP_MINUS:
    runtime_command(rt, SMBC_CMD_SPEED_DOWN);
    break;
  case SDLK_0:
    runtime_command(rt, SMBC_CMD_SPEED_NORMAL);
    break;
  case SDLK_F1:
    runtime_command(rt, SMBC_CMD_OVERLAY_TOGGLE);
    break;
  case SDLK_F2:
    *save_now = true;
    break;
  case SDLK_F3:
    *load_now = true;
    break;
  case SDLK_F5:
    *reset_now = true;
    break;
  case SDLK_F6:
    *cycle_pad = true;
    break;
  case SDLK_F12:
    *shot_now = true;
    break;
  default:
    break;
  }
}

static void update_overlay(SmbcVideo *video, const SmbcRuntime *rt,
                           const SmbcHistory *history) {
  static char shown[160];
  char text[160];
  snprintf(text, sizeof(text),
           "smbc | frame %u | %u%% | rewind %u/%u%s%s",
           (unsigned)rt->frames_run, (unsigned)runtime_speed_percent(rt),
           (unsigned)history->count, (unsigned)history->capacity,
           rt->paused ? " | paused" : "", rt->overlay ? "" : " | (F1)");
  if (rt->overlay && strcmp(text, shown) != 0) {
    SDL_SetWindowTitle(video->window, text);
    snprintf(shown, sizeof(shown), "%s", text);
  } else if (!rt->overlay && shown[0] != '\0') {
    SDL_SetWindowTitle(video->window, "smbc");
    shown[0] = '\0';
  }
}

static int run_game(unsigned scale, const char *record_path) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) !=
      0) {
    fprintf(stderr, "smbc: SDL_Init: %s\n", SDL_GetError());
    return 1;
  }

  Smbc smbc;
  smbc_init(&smbc);

  SmbcVideo video = {0};
  if (!video_open(&video, scale)) {
    fprintf(stderr, "smbc: video: %s\n", SDL_GetError());
    video_close(&video);
    SDL_Quit();
    return 1;
  }

  SmbcReplay recording_store, *recording = NULL;
  if (record_path != NULL) {
    replay_init(&recording_store);
    recording = &recording_store;
  }

  uint8_t *rewind_store = malloc((size_t)SMBC_REWIND_FRAMES * smbc_snapshot_size());
  if (rewind_store == NULL) {
    fprintf(stderr, "smbc: out of memory for rewind history\n");
    video_close(&video);
    SDL_Quit();
    return 1;
  }
  SmbcHistory history;
  history_init(&history, rewind_store, SMBC_REWIND_FRAMES);
  SmbcRuntime rt;
  runtime_init(&rt, &history);

  SmbcAudio audio = {0};
  if (!audio_open(&audio)) {
    /* non-fatal: play silent */
    fprintf(stderr, "smbc: audio: %s (continuing silent)\n", SDL_GetError());
  }
  SmbcKeymap keymap;
  load_keymap(&keymap);
  SmbcPadSet pads;
  padset_init(&pads);
  scan_pads(&pads);
  report_active_pad(&pads);

  uint8_t fb[SMBC_RENDER_PIXELS];
  memset(fb, 0, sizeof(fb));
  char err[128];
  bool quit = false;
  uint64_t next = now_ns();
  while (!quit) {
    bool reset_now = false, save_now = false, load_now = false,
         shot_now = false, cycle_pad = false;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT ||
          (event.type == SDL_KEYDOWN &&
           event.key.keysym.sym == SDLK_ESCAPE)) {
        quit = true;
      } else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
        handle_key(event.key.keysym.sym, &rt, &reset_now, &save_now,
                   &load_now, &shot_now, &cycle_pad);
      } else if (event.type == SDL_CONTROLLERDEVICEADDED) {
        /* `which` is a device index here */
        SDL_GameController *c = input_open_pad(event.cdevice.which);
        if (c != NULL && padset_add(&pads, input_pad_instance_id(c))) {
          printf("smbc: controller connected\n");
          report_active_pad(&pads);
        } else if (c != NULL) {
          SDL_GameControllerClose(c); /* already known, or set is full */
        }
      } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
        /* `which` is an instance id here */
        SDL_GameController *c = input_pad_from_instance(event.cdevice.which);
        bool was_active = false;
        if (padset_remove(&pads, event.cdevice.which, &was_active)) {
          if (c != NULL) {
            SDL_GameControllerClose(c);
          }
          printf("smbc: controller disconnected%s\n",
                 was_active ? " (was active)" : "");
          report_active_pad(&pads);
        }
      }
    }
    if (cycle_pad && pads.count > 0) {
      unsigned next = (unsigned)padset_index_of(&pads, padset_active_id(&pads));
      padset_select(&pads, (next + 1u) % pads.count);
      report_active_pad(&pads);
    }

    if (save_now) {
      if (capture_write_state(&smbc, SMBC_STATE_PATH, err, sizeof(err))) {
        printf("smbc: state saved to %s\n", SMBC_STATE_PATH);
      } else {
        fprintf(stderr, "smbc: save state: %s\n", err);
      }
    }
    if (load_now) {
      if (capture_read_state(&smbc, SMBC_STATE_PATH, err, sizeof(err))) {
        printf("smbc: state loaded from %s\n", SMBC_STATE_PATH);
      } else {
        fprintf(stderr, "smbc: load state: %s\n", err);
      }
    }
    if (shot_now) {
      char path[64];
      snprintf(path, sizeof(path), "smbc-%u.ppm", (unsigned)rt.frames_run);
      if (capture_write_ppm(path, fb, err, sizeof(err))) {
        printf("smbc: screenshot %s\n", path);
      } else {
        fprintf(stderr, "smbc: screenshot: %s\n", err);
      }
    }
    if (reset_now) {
      runtime_command(&rt, SMBC_CMD_RESET);
    }

    /* fixed-step pacing: advance the core one frame per NES period,
     * catching up (capped) after a stall; the simulation sees the
     * same frame sequence regardless of host render rate */
    uint64_t now = now_ns();
    if (now < next) {
      SDL_Delay((uint32_t)((next - now) / 1000000ull));
      now = next;
    }
    unsigned steps = 0;
    while (now >= next && steps < SMBC_MAX_CATCHUP) {
      const Uint8 *keys = SDL_GetKeyboardState(NULL);
      if (keys[SDL_SCANCODE_BACKSPACE]) {
        runtime_command(&rt, SMBC_CMD_REWIND);
      }
      SDL_GameController *active =
          input_pad_from_instance(padset_active_id(&pads));
      uint8_t buttons = input_poll(&keymap, active);
      /* a plain step is one frame at 100% with nothing rewound or reset:
       * only those steps can be reproduced by a replay file */
      bool plain = !rt.paused && runtime_speed_percent(&rt) == 100u &&
                   !rt.rewind_requested && !rt.reset_requested && !reset_now;
      uint32_t frames = runtime_tick(&rt, &smbc, buttons, 0x00u);
      if (recording != NULL && (!plain || frames != 1u)) {
        printf("smbc: recording stopped: play left the plain 100%% path\n");
        replay_free(recording);
        recording = NULL;
      }
      if (frames == 1u) {
        audio_push_frame(&audio, &smbc);
      }
      if (recording != NULL && frames == 1u &&
          !replay_append(recording, buttons, 0x00u,
                         smbc_state_hash(&smbc))) {
        fprintf(stderr, "smbc: recording aborted (out of memory)\n");
        quit = true;
        break;
      }
      next += SMBC_FRAME_NS;
      steps++;
    }
    if (steps == SMBC_MAX_CATCHUP && now >= next) {
      next = now; /* sustained overload: drop the backlog */
    }

    ppu_render_frame(&smbc.ppu, fb, NULL);
    video_present(&video, fb);
    update_overlay(&video, &rt, &history);
  }

  if (recording != NULL) {
    char rec_err[128];
    if (!replay_save(recording, record_path, rec_err, sizeof(rec_err))) {
      fprintf(stderr, "smbc: %s: %s\n", record_path, rec_err);
    } else {
      printf("smbc: recorded %u frames to %s\n", recording->count,
             record_path);
    }
    replay_free(recording);
  }

  free(rewind_store);
  for (unsigned i = 0; i < pads.count; i++) {
    SDL_GameController *c = input_pad_from_instance(pads.ids[i]);
    if (c != NULL) {
      SDL_GameControllerClose(c);
    }
  }
  audio_close(&audio);
  video_close(&video);
  SDL_Quit();
  return 0;
}

int main(int argc, char **argv) {
  unsigned scale = SMBC_DEFAULT_SCALE;
  const char *record_path = NULL;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--scale") == 0 && i + 1 < argc) {
      scale = (unsigned)atoi(argv[++i]);
      if (scale == 0 || scale > 8) {
        fprintf(stderr, "smbc: --scale must be 1-8\n");
        return 1;
      }
    } else if (strcmp(argv[i], "--headless") == 0 && i + 1 < argc) {
      return run_headless((unsigned)atoi(argv[++i]));
    } else if (strcmp(argv[i], "--play") == 0 && i + 1 < argc) {
      const char *path = argv[++i];
      unsigned stop = 0;
      if (i + 1 < argc && argv[i + 1][0] != '-') {
        stop = (unsigned)atoi(argv[++i]);
      }
      return run_play(path, stop);
    } else if (strcmp(argv[i], "--record") == 0 && i + 1 < argc) {
      record_path = argv[++i];
    } else {
      fprintf(stderr,
              "usage: smbc [--scale N] [--headless N] [--record PATH] "
              "[--play PATH [N]]\n");
      return 1;
    }
  }
  return run_game(scale, record_path);
}
