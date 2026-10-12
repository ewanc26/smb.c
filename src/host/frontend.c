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
 * Usage: smbc [--scale N] [--headless N] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

#include "../core/frame.h"
#include "../hw/render.h"
#include "audio.h"
#include "input.h"
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

static int run_game(unsigned scale) {
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

  SmbcAudio audio = {0};
  if (!audio_open(&audio, &smbc)) {
    /* non-fatal: play silent */
    fprintf(stderr, "smbc: audio: %s (continuing silent)\n", SDL_GetError());
  }
  SDL_GameController *controller = input_open_controller();

  uint8_t fb[SMBC_RENDER_PIXELS];
  bool quit = false;
  uint64_t next = now_ns();
  while (!quit) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT ||
          (event.type == SDL_KEYDOWN &&
           event.key.keysym.sym == SDLK_ESCAPE)) {
        quit = true;
      }
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
      joypad_set_buttons(&smbc.joypad1, input_poll(controller));
      smbc_frame(&smbc);
      next += SMBC_FRAME_NS;
      steps++;
    }
    if (steps == SMBC_MAX_CATCHUP && now >= next) {
      next = now; /* sustained overload: drop the backlog */
    }

    ppu_render_frame(&smbc.ppu, fb, NULL);
    video_present(&video, fb);
  }

  input_close_controller(controller);
  audio_close(&audio);
  video_close(&video);
  SDL_Quit();
  return 0;
}

int main(int argc, char **argv) {
  unsigned scale = SMBC_DEFAULT_SCALE;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--scale") == 0 && i + 1 < argc) {
      scale = (unsigned)atoi(argv[++i]);
      if (scale == 0 || scale > 8) {
        fprintf(stderr, "smbc: --scale must be 1-8\n");
        return 1;
      }
    } else if (strcmp(argv[i], "--headless") == 0 && i + 1 < argc) {
      return run_headless((unsigned)atoi(argv[++i]));
    } else {
      fprintf(stderr, "usage: smbc [--scale N] [--headless N]\n");
      return 1;
    }
  }
  return run_game(scale);
}
