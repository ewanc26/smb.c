/* host/audio.c — audio device seam. */
#include "audio.h"

#define SMBC_AUDIO_RATE 44100
#define SMBC_AUDIO_CHANNELS 2
#define SMBC_AUDIO_CHUNK 512 /* frames per callback */

void smbc_audio_render(Smbc *smbc, float *out, size_t frames) {
  /* #35: pull mixed APU samples from the core here. Silence until
   * synthesis lands — the device contract (rate, format, pull model)
   * is what this module fixes in place. */
  (void)smbc;
  for (size_t i = 0; i < frames * SMBC_AUDIO_CHANNELS; i++) {
    out[i] = 0.0f;
  }
}

static void audio_callback(void *userdata, uint8_t *stream, int len) {
  SmbcAudio *audio = userdata;
  smbc_audio_render(audio->smbc, (float *)stream,
                    (size_t)len / (sizeof(float) * SMBC_AUDIO_CHANNELS));
}

bool audio_open(SmbcAudio *audio, Smbc *smbc) {
  audio->smbc = smbc;
  SDL_AudioSpec want = {
      .freq = SMBC_AUDIO_RATE,
      .format = AUDIO_F32SYS,
      .channels = SMBC_AUDIO_CHANNELS,
      .samples = SMBC_AUDIO_CHUNK,
      .callback = audio_callback,
      .userdata = audio,
  };
  audio->device = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
  if (audio->device == 0) {
    return false;
  }
  SDL_PauseAudioDevice(audio->device, 0);
  return true;
}

void audio_close(SmbcAudio *audio) {
  if (audio->device != 0) {
    SDL_CloseAudioDevice(audio->device);
  }
}
