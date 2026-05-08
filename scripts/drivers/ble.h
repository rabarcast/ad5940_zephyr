#ifndef FALL_BLE_H
#define FALL_BLE_H

#include <zephyr/types.h>
#include <math.h>

void ble_init(void);
int ble_send(uint8_t *data, uint16_t len);

#endif