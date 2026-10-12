/* host/audio.c — audio device seam. */
#include "audio.h"

#include "../core/frame.h" /* Smbc definition for smbc->apu */

#define SMBC_AUDIO_CHUNK 512 /* frames per callback */

void audio_push_frame(SmbcAudio *audio, Smbc *smbc) {
  float frame_buf[SMBC_APU_MAX_FRAME_SAMPLES];
  size_t n = apu_synth_frame(&audio->synth, &smbc->apu, frame_buf);
  if (audio->device == 0) {
    return; /* log drained, nothing to queue */
  }
  unsigned head = atomic_load_explicit(&audio->head, memory_order_relaxed);
  unsigned tail = atomic_load_explicit(&audio->tail, memory_order_acquire);
  unsigned used = head - tail;
  unsigned free_space = SMBC_AUDIO_RING - used;
  if (n > free_space) {
    /* producer ahead of the device: drop the oldest samples to make
     * room — a brief skip beats unbounded latency growth */
    tail += (unsigned)n - free_space;
    atomic_store_explicit(&audio->tail, tail, memory_order_release);
  }
  for (size_t i = 0; i < n; i++) {
    audio->ring[(head + (unsigned)i) % SMBC_AUDIO_RING] = frame_buf[i];
  }
  atomic_store_explicit(&audio->head, head + (unsigned)n,
                        memory_order_release);
}

void smbc_audio_render(SmbcAudio *audio, float *out, size_t frames) {
  unsigned head = atomic_load_explicit(&audio->head, memory_order_acquire);
  unsigned tail = atomic_load_explicit(&audio->tail, memory_order_relaxed);
  unsigned available = head - tail;
  for (size_t i = 0; i < frames; i++) {
    float s = 0.0f;
    if ((unsigned)i < available) {
      s = audio->ring[(tail + (unsigned)i) % SMBC_AUDIO_RING];
    }
    out[i * SMBC_AUDIO_CHANNELS] = s;
    out[i * SMBC_AUDIO_CHANNELS + 1] = s;
  }
  unsigned consumed = (unsigned)frames < available ? (unsigned)frames
                                                   : available;
  atomic_store_explicit(&audio->tail, tail + consumed,
                        memory_order_release);
}

static void audio_callback(void *userdata, uint8_t *stream, int len) {
  SmbcAudio *audio = userdata;
  smbc_audio_render(audio, (float *)stream,
                    (size_t)len / (sizeof(float) * SMBC_AUDIO_CHANNELS));
}

bool audio_open(SmbcAudio *audio) {
  apu_synth_init(&audio->synth);
  atomic_store(&audio->head, 0);
  atomic_store(&audio->tail, 0);
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
