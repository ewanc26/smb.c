/* tests/host/padset.c — gamepad hot-plug bookkeeping with mocked ids (#23). */
#include <assert.h>
#include <stdio.h>

#include "../../src/host/padset.h"

int main(void) {
  /* the first pad connected becomes active; later ones do not steal it */
  {
    SmbcPadSet set;
    padset_init(&set);
    assert(padset_active_id(&set) == -1);
    assert(padset_add(&set, 7));
    assert(padset_active_id(&set) == 7);
    assert(padset_add(&set, 9));
    assert(padset_active_id(&set) == 7);
    assert(set.count == 2);
  }

  /* duplicates and overflow are refused */
  {
    SmbcPadSet set;
    padset_init(&set);
    assert(padset_add(&set, 1));
    assert(!padset_add(&set, 1));
    assert(padset_add(&set, 2));
    assert(padset_add(&set, 3));
    assert(padset_add(&set, 4));
    assert(!padset_add(&set, 5)); /* full at SMBC_MAX_PADS */
    assert(set.count == SMBC_MAX_PADS);
  }

  /* removing a pad before the active one keeps the same pad active */
  {
    SmbcPadSet set;
    padset_init(&set);
    padset_add(&set, 1);
    padset_add(&set, 2);
    padset_add(&set, 3);
    assert(padset_select(&set, 2)); /* pad 3 active */
    bool was_active = true;
    assert(padset_remove(&set, 1, &was_active));
    assert(!was_active);
    assert(padset_active_id(&set) == 3);
    assert(padset_index_of(&set, 3) == 1);
  }

  /* removing the active pad falls back to the first remaining one */
  {
    SmbcPadSet set;
    padset_init(&set);
    padset_add(&set, 1);
    padset_add(&set, 2);
    padset_select(&set, 1); /* pad 2 active */
    bool was_active = false;
    assert(padset_remove(&set, 2, &was_active));
    assert(was_active);
    assert(padset_active_id(&set) == 1);
  }

  /* removing the last pad leaves nothing selected; unknown ids change nothing */
  {
    SmbcPadSet set;
    padset_init(&set);
    padset_add(&set, 5);
    bool was_active = false;
    assert(!padset_remove(&set, 99, &was_active));
    assert(!was_active);
    assert(padset_active_id(&set) == 5);
    assert(padset_remove(&set, 5, &was_active));
    assert(was_active);
    assert(padset_active_id(&set) == -1);
    assert(set.count == 0);
  }

  /* reconnecting after everything was removed selects the new pad */
  {
    SmbcPadSet set;
    padset_init(&set);
    padset_add(&set, 5);
    padset_remove(&set, 5, NULL);
    assert(padset_add(&set, 6));
    assert(padset_active_id(&set) == 6);
  }

  /* selection by index is range-checked */
  {
    SmbcPadSet set;
    padset_init(&set);
    padset_add(&set, 1);
    assert(!padset_select(&set, 1));
    assert(padset_active_id(&set) == 1);
  }

  printf("padset: ok\n");
  return 0;
}
