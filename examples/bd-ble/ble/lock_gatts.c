#include <string.h>
#include "esp_log.h"
// GATTS library.
#include "esp_gatts_api.h"
// Local includes.
#include "lock_gatts.h"

static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint8_t character_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t character_prop_write = ESP_GATT_CHAR_PROP_BIT_WRITE;

static uint8_t lock_state = 0;
static uint8_t request_claim[16] = {0};

/**
 * The UUID used to identify the lock service.
 *
 * b3124a83-10af-47d2-80e3-5e5b0c6b7f94
 */
uint8_t lock_service_uuid[16] = {0x94, 0x7f, 0x6b, 0x0c, 0x5b, 0x5e, 0xe3, 0x80, 0xd2, 0x47, 0xaf, 0x10, 0x83, 0x4a, 0x12, 0xb3};

/**
 * The UUID used to read the lock state.
 *
 * d57e24d2-1fb7-4b9c-a685-6e6f2c2e9cf7
 */
uint8_t lock_state_uuid[16] = {0xf7, 0x9c, 0x2e, 0x2c, 0x6f, 0x6e, 0x85, 0xa6, 0x9c, 0x4b, 0xb7, 0x1f, 0xd2, 0x24, 0x7e, 0xd5};

/**
 * The UUID used to request a claim.
 *
 * 68e84b3d-3b31-4c64-b613-2c378d5a906a
 */
uint8_t request_claim_uuid[16] = {0x6a, 0x90, 0x5a, 0x8d, 0x37, 0x2c, 0x13, 0xb6, 0x64, 0x4c, 0x31, 0x3b, 0x3d, 0x4b, 0xe8, 0x68};

uint16_t lock_gatts_svc_handle_table[LOCK_SVC_IDX_NB];

/* Lock State Characteristic definition */
esp_gatts_attr_db_t lock_gatts_svc_db[LOCK_SVC_IDX_NB] = {
		// Service Declaration
		[LOCK_SVC_IDX] = {
				.attr_control = {.auto_rsp = ESP_GATT_RSP_BY_APP},
				.att_desc = {
						.uuid_length = ESP_UUID_LEN_16,
						.uuid_p = (uint8_t *)&primary_service_uuid, // lock_service_uuid,
						.perm = ESP_GATT_PERM_READ,
						.max_length = sizeof(lock_service_uuid),
						.length = sizeof(lock_service_uuid),
						.value = lock_service_uuid,
				},
		},
		// Lock State Characteristic Declaration
		[LOCK_SVC_IDX_CHAR_STATE] = {
				.attr_control = {.auto_rsp = ESP_GATT_AUTO_RSP},
				.att_desc = {
						.uuid_length = ESP_UUID_LEN_16,
						.uuid_p = (uint8_t *)&character_declaration_uuid,
						.perm = ESP_GATT_PERM_READ_ENCRYPTED,
						.max_length = sizeof(uint16_t),
						.length = sizeof(uint16_t),
						.value = (uint8_t *)&character_prop_read,
				},
		},
		// Lock State Characteristic Value
		[LOCK_SVC_IDX_CHAR_STATE_VAL] = {
				.attr_control = {.auto_rsp = ESP_GATT_AUTO_RSP},
				.att_desc = {
						.uuid_length = ESP_UUID_LEN_128,
						.uuid_p = lock_state_uuid,
						.perm = ESP_GATT_PERM_READ_ENCRYPTED,
						.max_length = sizeof(lock_state),
						.length = sizeof(lock_state),
						.value = &lock_state,
				},
		},
		// Request Claim Characteristic Declaration
		[LOCK_SVC_IDX_CHAR_CLAIM] = {
				.attr_control = {.auto_rsp = ESP_GATT_AUTO_RSP},
				.att_desc = {
						.uuid_length = ESP_UUID_LEN_16,
						.uuid_p = (uint8_t *)&character_declaration_uuid,
						.perm = ESP_GATT_PERM_WRITE_ENCRYPTED,
						.max_length = sizeof(uint16_t),
						.length = sizeof(uint16_t),
						.value = (uint8_t *)&character_prop_write,
				},
		},
		// Request Claim Characteristic Value
		[LOCK_SVC_IDX_CHAR_CLAIM_VAL] = {
				.attr_control = {.auto_rsp = ESP_GATT_AUTO_RSP},
				.att_desc = {
						.uuid_length = ESP_UUID_LEN_128,
						.uuid_p = request_claim_uuid,
						.perm = ESP_GATT_PERM_WRITE_ENCRYPTED,
						.max_length = sizeof(request_claim),
						.length = sizeof(request_claim),
						.value = request_claim,
				},
		},
};

void lock_gatts_svc_attr_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
	// Initialize your lock state or any other setup for your lock here.
	// You can also handle the registration and creation of service and attribute table here.
}

void lock_gatts_svc_read_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
	if (param->read.handle == lock_gatts_svc_handle_table[LOCK_SVC_IDX_CHAR_STATE_VAL])
	{
		esp_gatt_rsp_t rsp;
		memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
		rsp.attr_value.handle = param->read.handle;

		// Call the function to get the current lock state
		// lock_state = phy_lock_svc_get_state();

		rsp.attr_value.len = sizeof(lock_state);
		rsp.attr_value.value[0] = lock_state;
		esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
	}
}

void lock_gatts_svc_write_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
	// Handle any write events related to your lock service here.
}
