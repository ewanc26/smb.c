/* host/padset.c — see host/padset.h for the contract. */
#include "padset.h"

#include <stddef.h>

void padset_init(SmbcPadSet *set) {
  for (unsigned i = 0; i < SMBC_MAX_PADS; i++) {
    set->ids[i] = -1;
  }
  set->count = 0;
  set->active = -1;
}

int padset_index_of(const SmbcPadSet *set, int32_t id) {
  for (unsigned i = 0; i < set->count; i++) {
    if (set->ids[i] == id) {
      return (int)i;
    }
  }
  return -1;
}

bool padset_add(SmbcPadSet *set, int32_t id) {
  if (set->count >= SMBC_MAX_PADS || padset_index_of(set, id) >= 0) {
    return false;
  }
  set->ids[set->count++] = id;
  if (set->active < 0) {
    set->active = 0;
  }
  return true;
}

bool padset_remove(SmbcPadSet *set, int32_t id, bool *was_active) {
  int at = padset_index_of(set, id);
  if (was_active != NULL) {
    *was_active = false;
  }
  if (at < 0) {
    return false;
  }
  bool removed_active = set->active == at;
  for (unsigned i = (unsigned)at; i + 1 < set->count; i++) {
    set->ids[i] = set->ids[i + 1];
  }
  set->count--;
  set->ids[set->count] = -1;

  if (removed_active) {
    set->active = set->count > 0 ? 0 : -1;
  } else if (set->active > at) {
    set->active--; /* the selected pad moved down one slot */
  }
  if (was_active != NULL) {
    *was_active = removed_active;
  }
  return true;
}

bool padset_select(SmbcPadSet *set, unsigned index) {
  if (index >= set->count) {
    return false;
  }
  set->active = (int8_t)index;
  return true;
}

int32_t padset_active_id(const SmbcPadSet *set) {
  return set->active < 0 ? -1 : set->ids[set->active];
}
