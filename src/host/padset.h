/* host/padset.h — which gamepads are connected and which one is active (#23).
 *
 * Owns: the list of connected controllers, identified by a stable id the host
 * assigns (the SDL instance id in practice), and the selected controller.
 * Adding the first pad selects it; removing the selected pad selects the next
 * one, or none. Nothing latches: the input for a tick is read from the active
 * pad only, so a removed pad cannot leave a button stuck.
 *
 * Not owned: opening or closing SDL controllers (host/input.c), key or pad
 * mapping (host/keymap.c). Selection does not survive a restart; that is a
 * documented gap against #23.
 */
#ifndef SMBC_HOST_PADSET_H
#define SMBC_HOST_PADSET_H

#include <stdbool.h>
#include <stdint.h>

#define SMBC_MAX_PADS 4u

typedef struct {
  int32_t ids[SMBC_MAX_PADS];
  uint8_t count;
  int8_t active; /* index into ids, or -1 */
} SmbcPadSet;

void padset_init(SmbcPadSet *set);

/* Connect a pad. False if it is already present or the set is full. The first
 * pad connected becomes active. */
bool padset_add(SmbcPadSet *set, int32_t id);

/* Disconnect a pad. False if it was not present. was_active reports whether
 * the removed pad was the selected one; the set then selects the first
 * remaining pad, or none. */
bool padset_remove(SmbcPadSet *set, int32_t id, bool *was_active);

/* Select the pad at index (0-based, in connection order). False if out of
 * range. */
bool padset_select(SmbcPadSet *set, unsigned index);

/* Id of the active pad, or -1. */
int32_t padset_active_id(const SmbcPadSet *set);

/* Index of an id in the set, or -1. */
int padset_index_of(const SmbcPadSet *set, int32_t id);

#endif /* SMBC_HOST_PADSET_H */
