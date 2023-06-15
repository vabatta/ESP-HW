#ifndef _BLE_SVC_H_
#define _BLE_SVC_H_

#include "esp_err.h"

#define BLE_DEVICE_NAME "CubeLock-123456"
#define BLE_PASSKEY 123456

extern esp_err_t ble_svc_init();

#endif // _BLE_SVC_H_
