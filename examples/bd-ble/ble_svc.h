#ifndef _BLE_SVC_H
#define _BLE_SVC_H

#include "esp_err.h"

#define BLE_DEVICE_NAME "CubeLock-123456"
#define BLE_PASSKEY 123456

extern esp_err_t ble_svc_init();

#endif // _BLE_SVC_H
