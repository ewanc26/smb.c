/* core/state.c — see core/state.h for the contract. */
#include "state.h"

void state_init(CoreState *state) {
  *state = (CoreState){0};
}
