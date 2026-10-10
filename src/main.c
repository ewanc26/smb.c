/* main.c — host entry point.
 *
 * Scaffolding, not translated code: runs a fixed number of headless frames
 * so the skeleton links and the frame loop is exercisable until a host
 * video/audio/input frontend arrives.
 */
#include <stdio.h>

#include "core/frame.h"

#define SMBC_HEADLESS_FRAMES 60

int main(void) {
  Smbc smbc;
  smbc_init(&smbc);

  for (int frame = 0; frame < SMBC_HEADLESS_FRAMES; frame++) {
    smbc_frame(&smbc);
  }

  printf("smbc: ran %d frames (counter=%u)\n", SMBC_HEADLESS_FRAMES,
         (unsigned)smbc.state.ram[SMBC_FRAME_COUNTER]);
  return 0;
}
