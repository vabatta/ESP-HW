#ifndef _CL_BLE_SVC_H_
#define _CL_BLE_SVC_H_

#include "esp_err.h"

#define CL_BLE_DEVICE_NAME "CubeLock-123456"
#define CL_BLE_PASSKEY 123456
#define CL_BLE_MIN_GATT_ENC_KEY_LEN 16

/**
 * Initialize the BLE service.
 * It sets up the BLE host and controller stack, sets the device name, sets the GAP parameters,
 * sets the GATTS services and starts advertising.
 * It runs in a task and returns only when nimble_port_stop() is executed.
 */
extern esp_err_t cl_ble_svc_init();

#endif // _CL_BLE_SVC_H_
