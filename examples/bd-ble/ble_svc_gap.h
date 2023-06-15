#ifndef _BLE_SVC_GAP_H_
#define _BLE_SVC_GAP_H_

#include "esp_gap_ble_api.h"

// extern esp_err_t ble_gap_init();

extern void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

#endif // _BLE_SVC_GAP_H_