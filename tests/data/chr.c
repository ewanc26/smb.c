/* tests/data/chr.c — exercises src/data/chr.c: the extracted pattern
 * tables are the expected 8KB, byte-for-byte (CRC-32 of the donor ROM's
 * CHR section). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../../src/data/chr.h"

/* bitwise CRC-32 (IEEE), as used to checksum the donor CHR */
static uint32_t crc32(const uint8_t *data, uint32_t length) {
  uint32_t crc = 0xffffffffu;
  for (uint32_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; bit++) {
      crc = (crc >> 1) ^ (crc & 1u ? 0xedb88320u : 0u);
    }
  }
  return ~crc;
}

int main(void) {
  /* first tile's first plane rows, from the donor CHR */
  assert(SMBC_CHR_ROM[0] == 0x03u);
  assert(SMBC_CHR_ROM[1] == 0x0fu);
  assert(SMBC_CHR_ROM[8] == 0x00u); /* second bitplane starts here */
  assert(SMBC_CHR_SIZE == 0x2000u);
  assert(crc32(SMBC_CHR_ROM, SMBC_CHR_SIZE) == 0x867b51adu);

  printf("tests/data/chr: ok\n");
  return 0;
}
