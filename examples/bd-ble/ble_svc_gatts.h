#ifndef _BLE_SVC_GATTS_H_
#define _BLE_SVC_GATTS_H_

#include "esp_err.h"

#define PROFILE_NUM 1
#define PROFILE_APP_IDX 0
#define SINGLE_APP_ID 0x42

typedef struct
{
	esp_gatts_cb_t gatts_cb;
	esp_gatt_if_t gatts_if;
	uint16_t conn_id;
	uint16_t service_handle;
	esp_gatt_srvc_id_t service_id;
	uint16_t char_handle;
	esp_bt_uuid_t char_uuid;
	esp_gatt_perm_t perm;
	esp_gatt_char_prop_t property;
	uint16_t descr_handle;
	esp_bt_uuid_t descr_uuid;
} gatts_profile_inst;

// void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

// extern esp_err_t ble_gatts_init();

extern void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

#endif