/* core/initialize.c — see core/initialize.h for the contract. */
#include "initialize.h"

#include "../data/music.h"
#include "area.h"
#include "frame.h"
#include "memory.h"

/* smb.asm: InitializeArea */
void initialize_area(Smbc *smbc) {
  CoreState *st = &smbc->state;

  /* clear all memory again, only as far as $074b */
  initialize_memory(st, 0x4b);

  /* ClrTimersLoop — clear out memory between $0780 and $07a1 */
  for (int x = 0x21; x >= 0; x--) {
    st->ram[SMBC_TIMERS + x] = 0x00u;
  }

  /* StartPage — a saved entry page (AltEntranceControl set) wins over the
   * halfway page; the value is stored as the screen-left page, the current
   * page and the backloading flag alike */
  uint8_t page = st->ram[SMBC_HALFWAY_PAGE];
  if (st->ram[SMBC_ALT_ENTRANCE_CONTROL] != 0x00u) {
    page = st->ram[SMBC_ENTRANCE_PAGE];
  }
  st->ram[SMBC_SCREEN_LEFT_PAGE_LOC] = page;
  st->ram[SMBC_CURRENT_PAGE_LOC] = page;
  st->ram[SMBC_BACKLOADING_FLAG] = page;

  /* GetScreenPosition leaves A holding the right page location; on odd
   * pages the name table is $2480 rather than $2080, and the page's LSB
   * becomes the high nybble of the block buffer column position */
  uint8_t lsb = get_screen_position(st) & 0x01u;
  st->ram[SMBC_CURRENT_NT_ADDR_HIGH] = (lsb != 0x00u) ? 0x24u : 0x20u;
  st->ram[SMBC_CURRENT_NT_ADDR_LOW] = 0x80u;
  st->ram[SMBC_BLOCK_BUFFER_COLUMN_POS] = (uint8_t)(lsb << 4);

  /* set area object lengths for all empty and prime the renderer */
  st->ram[SMBC_AREA_OBJECT_LENGTH] -= 0x01u;
  st->ram[SMBC_AREA_OBJECT_LENGTH + 1u] -= 0x01u;
  st->ram[SMBC_AREA_OBJECT_LENGTH + 2u] -= 0x01u;
  st->ram[SMBC_COLUMN_SETS] = 0x0bu; /* 12 column sets */

  get_area_data_addrs(smbc); /* enemy and level addresses, plus header */

  /* activate secondary hard mode for areas 5-3 and beyond (zero-based:
   * world 5, level 3), or whenever primary hard mode is already set */
  if (st->ram[SMBC_PRIMARY_HARD_MODE] != 0x00u) {
    st->ram[SMBC_SECONDARY_HARD_MODE]++;
  } else {
    uint8_t world = st->ram[SMBC_WORLD_NUMBER];
    if (world > SMBC_WORLD5) {
      st->ram[SMBC_SECONDARY_HARD_MODE]++;
    } else if (world == SMBC_WORLD5 &&
               st->ram[SMBC_LEVEL_NUMBER] >= SMBC_LEVEL3) {
      st->ram[SMBC_SECONDARY_HARD_MODE]++;
    }
  }

  /* CheckHalfway — a halfway page overwrites the starting position taken
   * from the header */
  if (st->ram[SMBC_HALFWAY_PAGE] != 0x00u) {
    st->ram[SMBC_PLAYER_ENTRANCE_CTRL] = 0x02u;
  }

  /* DoneInitArea — silence the music, disable screen output, advance the
   * mode task */
  st->ram[SMBC_AREA_MUSIC_QUEUE] = SMBC_MUSIC_SILENCE;
  st->ram[SMBC_DISABLE_SCREEN_FLAG] = 0x01u;
  st->ram[SMBC_OPER_MODE_TASK]++;
}
