/* data/title.h — title screen tables.
 *
 * Owns: the world-select VRAM template, the menu mushroom icon, and the
 * demo action/timing scripts that GameMenuRoutine, DrawMushroomIcon and
 * DemoEngine (core/title) consume.
 */
#ifndef SMBC_DATA_TITLE_H
#define SMBC_DATA_TITLE_H

#include <stdint.h>

#define SMBC_WSELECT_BUFFER_TEMPLATE_SIZE 6u
#define SMBC_MUSHROOM_ICON_DATA_SIZE      8u
#define SMBC_DEMO_ACTION_DATA_SIZE        21u
#define SMBC_DEMO_TIMING_DATA_SIZE        22u

/* smb.asm: WSelectBufferTemplate — VRAM buffer update for the world
 * select display (world number byte written separately) */
extern const uint8_t SMBC_WSELECT_BUFFER_TEMPLATE[SMBC_WSELECT_BUFFER_TEMPLATE_SIZE];

/* smb.asm: MushroomIconData — VRAM buffer update drawing the menu
 * mushroom icon at the 1-player position */
extern const uint8_t SMBC_MUSHROOM_ICON_DATA[SMBC_MUSHROOM_ICON_DATA_SIZE];

/* smb.asm: DemoActionData — controller bits per demo action */
extern const uint8_t SMBC_DEMO_ACTION_DATA[SMBC_DEMO_ACTION_DATA_SIZE];

/* smb.asm: DemoTimingData — frames per demo action; a zero entry ends
 * the demo */
extern const uint8_t SMBC_DEMO_TIMING_DATA[SMBC_DEMO_TIMING_DATA_SIZE];

#endif /* SMBC_DATA_TITLE_H */
