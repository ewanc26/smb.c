#ifndef SMBC_DATA_CANNON_H
#define SMBC_DATA_CANNON_H

/* data/cannon.h — the bullet bill cannon tables (smb.asm lines
 * 6760-6822): the LSFR mask that decides whether a cannon fires and the
 * bullet bill's horizontal speed by movement direction. Consumed by
 * core/cannon. */

#include <stdint.h>

/* smb.asm: CannonBitmasks — the LSFR bits tested when deciding if a
 * cannon fires, indexed by the secondary hard mode flag */
extern const uint8_t SMBC_CANNON_BITMASKS[2];

/* smb.asm: BulletBillXSpdData — the bullet bill's horizontal speed,
 * indexed by movement direction minus one (right, then left) */
extern const uint8_t SMBC_BULLET_BILL_X_SPD_DATA[2];

#endif /* SMBC_DATA_CANNON_H */
