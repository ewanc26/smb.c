/* host/video.h — presentation.
 *
 * Owns: the SDL window, renderer and texture that present the core's
 * 256×240 frame of NES colour indices. Native NES geometry only: the
 * texture is 256×240, scaling is integer and nearest-neighbour, and
 * the aspect ratio is the frame's own (no widescreen, no smoothing,
 * no effects — #31 fidelity).
 *
 * Not owned: frame pacing (frontend.c), palette choice (palette.c) or
 * producing the pixels (hw/render). */
#ifndef SMBC_HOST_VIDEO_H
#define SMBC_HOST_VIDEO_H

#include <stdbool.h>
#include <stdint.h>

#include <SDL.h>

#include "../hw/render.h"

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  uint32_t rgb[SMBC_RENDER_PIXELS]; /* ARGB8888 staging */
} SmbcVideo;

/* Open the window at scale× the native frame (256×scale × 240×scale).
 * Returns false on SDL failure, with the message in SDL_GetError. */
bool video_open(SmbcVideo *video, unsigned scale);

/* Present one frame of 2C02 palette indices. */
void video_present(SmbcVideo *video, const uint8_t *fb);

void video_close(SmbcVideo *video);

#endif /* SMBC_HOST_VIDEO_H */
