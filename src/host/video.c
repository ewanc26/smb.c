/* host/video.c — presentation. */
#include "video.h"

#include "palette.h"

bool video_open(SmbcVideo *video, unsigned scale) {
  video->window = SDL_CreateWindow(
      "smbc", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      (int)(SMBC_RENDER_WIDTH * scale), (int)(SMBC_RENDER_HEIGHT * scale),
      SDL_WINDOW_SHOWN);
  if (video->window == NULL) {
    return false;
  }

  /* Integer scaling by default; nearest sampling keeps pixels sharp. */
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
  video->renderer = SDL_CreateRenderer(
      video->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (video->renderer == NULL) {
    return false;
  }
  SDL_RenderSetLogicalSize(video->renderer, (int)SMBC_RENDER_WIDTH,
                           (int)SMBC_RENDER_HEIGHT);
  SDL_RenderSetIntegerScale(video->renderer, SDL_TRUE);

  video->texture = SDL_CreateTexture(
      video->renderer, SDL_PIXELFORMAT_ARGB8888,
      SDL_TEXTUREACCESS_STREAMING, (int)SMBC_RENDER_WIDTH,
      (int)SMBC_RENDER_HEIGHT);
  return video->texture != NULL;
}

void video_present(SmbcVideo *video, const uint8_t *fb) {
  for (unsigned i = 0; i < SMBC_RENDER_PIXELS; i++) {
    const uint8_t *c = SMBC_2C02_RGB[fb[i] & 0x3fu];
    video->rgb[i] = 0xff000000u | ((uint32_t)c[0] << 16) |
                    ((uint32_t)c[1] << 8) | (uint32_t)c[2];
  }
  SDL_UpdateTexture(video->texture, NULL, video->rgb,
                    (int)(SMBC_RENDER_WIDTH * sizeof(uint32_t)));
  SDL_RenderClear(video->renderer);
  SDL_RenderCopy(video->renderer, video->texture, NULL, NULL);
  SDL_RenderPresent(video->renderer);
}

void video_close(SmbcVideo *video) {
  if (video->texture != NULL) {
    SDL_DestroyTexture(video->texture);
  }
  if (video->renderer != NULL) {
    SDL_DestroyRenderer(video->renderer);
  }
  if (video->window != NULL) {
    SDL_DestroyWindow(video->window);
  }
}
