#ifndef _LOCK_GATTS_H_
#define _LOCK_GATTS_H_

#include "esp_gatts_api.h"

// Definitions
#define LOCK_SVC_INST_ID 0

// Attributes Indexes
#define LOCK_SVC_IDX_NB 5
#define LOCK_SVC_IDX 0
#define LOCK_SVC_IDX_CHAR_STATE 1
#define LOCK_SVC_IDX_CHAR_STATE_VAL 2
#define LOCK_SVC_IDX_CHAR_CLAIM 3
#define LOCK_SVC_IDX_CHAR_CLAIM_VAL 4

// UUIDs
extern uint8_t lock_service_uuid[16];
extern uint8_t lock_state_uuid[16];
extern uint8_t request_claim_uuid[16];

// Attribute Database
extern esp_gatts_attr_db_t lock_gatts_svc_db[LOCK_SVC_IDX_NB];

// Handle table
extern uint16_t lock_gatts_svc_handle_table[LOCK_SVC_IDX_NB];

// Functions
void lock_gatts_svc_attr_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
void lock_gatts_svc_read_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
void lock_gatts_svc_write_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

#endif /* _LOCK_GATTS_H_ */
