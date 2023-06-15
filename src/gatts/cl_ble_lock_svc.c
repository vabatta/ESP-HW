// Library
#include <string.h>
// ESP32
#include "esp_log.h"
#include "esp_err.h"
// Bluetooth includes.
#include "host/ble_hs.h"
// Local includes.
#include "uuid_utils.h"
#include "cl_ble_lock_svc.h"
#include "cl_phy_lock_svc.h"

// -- INTERNAL FUNCTIONS --
int cl_ble_lock_svc_state_char_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
int cl_ble_lock_svc_state_desc_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
int cl_ble_lock_svc_req_claim_char_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
int cl_ble_lock_svc_req_release_char_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

// -- RUNTIME VARIABLES --
static const char *LOG_TAG = "BLELS";

/**
 * Bluetooth LE GATT uuid for the lock/unlock service.
 *
 * 91bad492-b950-4226-aa2b-4ede9fa42f59
 */
const ble_uuid128_t cl_ble_lock_svc_uuid = BLE_UUID128_INIT(0x91, 0xBA, 0xD4, 0x92, 0xB9, 0x50, 0x42, 0x26, 0xAA, 0x2B, 0x4E, 0xDE, 0x9F, 0xA4, 0x2F, 0x59);

/**
 * Bluetooth LE GATT uuid for the lock/unlock state characteristic.
 * This characteristic has a NOTIFY property.
 *
 * d57e24d2-1fb7-4b9c-a685-6e6f2c2e9cf7
 */
const ble_uuid128_t cl_ble_lock_svc_state_char_uuid = BLE_UUID128_INIT(0xf7, 0x9c, 0x2e, 0x2c, 0x6f, 0x6e, 0x85, 0xa6, 0x9c, 0x4b, 0xb7, 0x1f, 0xd2, 0x24, 0x7e, 0xd5);
uint16_t cl_ble_lock_svc_state_char_val_handle;

/**
 * Bluetooth LE GATT uuid for the lock/unlock state descriptor.
 *
 * b3124a83-10af-47d2-80e3-5e5b0c6b7f94
 */
const ble_uuid128_t cl_ble_lock_svc_state_desc_uuid = BLE_UUID128_INIT(0x94, 0x7f, 0x6b, 0x0c, 0x5b, 0x5e, 0xe3, 0x80, 0xd2, 0x47, 0xaf, 0x10, 0x83, 0x4a, 0x12, 0xb3);

/**
 * Bluetooth LE GATT uuid for the request claim characteristic.
 *
 * 8f3aebd7-3d0a-4b5f-9a5d-7e5d9ecce4d8
 */
const ble_uuid128_t cl_ble_lock_svc_req_claim_char_uuid = BLE_UUID128_INIT(0xd8, 0xe4, 0xcc, 0x9e, 0x5d, 0x7e, 0x5d, 0x9a, 0x5f, 0x4b, 0x0a, 0x3d, 0xd7, 0xeb, 0x3a, 0x8f);
uint16_t cl_ble_lock_svc_req_claim_char_val_handle;

/**
 * Bluetooth LE GATT uuid for the request release characteristic.
 *
 * 68e84b3d-3b31-4c64-b613-2c378d5a906a
 */
const ble_uuid128_t cl_ble_lock_svc_req_release_char_uuid = BLE_UUID128_INIT(0x6a, 0x90, 0x5a, 0x8d, 0x37, 0x2c, 0x13, 0xb6, 0x64, 0x4c, 0x31, 0x3b, 0x3d, 0x4b, 0xe8, 0x68);
uint16_t cl_ble_lock_svc_req_release_char_val_handle;

const struct ble_gatt_svc_def cl_ble_lock_svc_def[] =
		{{
				 .type = BLE_GATT_SVC_TYPE_PRIMARY,
				 .uuid = &cl_ble_lock_svc_uuid.u,
				 .characteristics = (struct ble_gatt_chr_def[]){
						 // State characteristic
						 {
								 .uuid = &cl_ble_lock_svc_state_char_uuid.u,
								 .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
								 .access_cb = cl_ble_lock_svc_state_char_cb,
								 .min_key_size = CL_BLE_MIN_GATT_ENC_KEY_LEN,
								 .val_handle = &cl_ble_lock_svc_state_char_val_handle,
								 .descriptors = (struct ble_gatt_dsc_def[]){
										 // Descriptor explaining the state values
										 {
												 .uuid = &cl_ble_lock_svc_state_desc_uuid.u,
												 .min_key_size = CL_BLE_MIN_GATT_ENC_KEY_LEN,
												 .att_flags = BLE_ATT_F_READ,
												 .access_cb = cl_ble_lock_svc_state_desc_cb,
										 },
										 {0},
								 },
						 },
						 // Request claim characteristic
						 {
								 .uuid = &cl_ble_lock_svc_req_claim_char_uuid.u,
								 .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC, // REVIEW maybe AUTHEN?
								 .access_cb = cl_ble_lock_svc_req_claim_char_cb,
								 .min_key_size = CL_BLE_MIN_GATT_ENC_KEY_LEN,
								 .val_handle = &cl_ble_lock_svc_req_claim_char_val_handle,
								 // TODO add descriptors explaining the request claim values and errors
						 },
						 // Request release characteristic
						 {
								 .uuid = &cl_ble_lock_svc_req_release_char_uuid.u,
								 .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC, // REVIEW maybe AUTHEN and AUTHOR?
								 .access_cb = cl_ble_lock_svc_req_release_char_cb,
								 .min_key_size = CL_BLE_MIN_GATT_ENC_KEY_LEN,
								 .val_handle = &cl_ble_lock_svc_req_release_char_val_handle,
								 // TODO add descriptors explaining the request claim values and errors
						 },
						 {0},
				 },
		 },
		 {0}};

int cl_ble_lock_svc_state_char_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	uint8_t phy_svc_state = cl_phy_lock_svc_get_state();
	ESP_LOGD(LOG_TAG, "Current lock state: %d", phy_svc_state);
	int ret = os_mbuf_append(ctxt->om, &phy_svc_state, sizeof(phy_svc_state));
	return ret == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

int cl_ble_lock_svc_state_desc_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	const char *state_explanations[] = {
			[PHY_LOCK_STATE_UNKNOWN] = "[uint 255] Unknown: The device has not yet initialized the lock state. This should never happen.",
			[PHY_LOCK_STATE_UNCLAIMED] = "[uint 0] Unclaimed: The device can accept claims requests as it has no owner yet.",
			[PHY_LOCK_STATE_CLAIMED] = "[uint 1] Claimed: The device is claimed and in use by a user and cannot be claimed by another user.",
			[PHY_LOCK_STATE_REQUESTED_CLAIM] = "[uint 2] Requested Claim: The device has received a claim request and is waiting for the user to confirm the claim.",
			[PHY_LOCK_STATE_REQUESTED_RELEASE] = "[uint 3] Requested Release: The device has received a release request and is waiting for the user to confirm the release.",
			[PHY_LOCK_STATE_SUPPORT] = "[uint 4] Support: The device is in support mode and need maintenance. Unavailable.",
	}; // NOTE: Adjust this according to PHY_LOCK_STATE_* in cl_phy_lock_svc.h

	uint8_t phy_svc_state = cl_phy_lock_svc_get_state();

	// we cannot exceed the size of the state_explanations array as the states are known
	const char *explanation = state_explanations[phy_svc_state];

	// Copy the explanation into the response buffer
	int ret = os_mbuf_append(ctxt->om, explanation, strlen(explanation));
	return ret == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

int cl_ble_lock_svc_req_claim_char_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
	int ret;

	if (om_len < (BLE_UUID_STR_LEN - 1) || om_len > BLE_UUID_STR_LEN)
	{
		ESP_LOGE(LOG_TAG, "Input mbuf not fitting uuid128 len: %d", om_len);
		return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
	}
	// we expect a string uuid in the format of 8-4-4-4-12 null-terminated
	char uuid_str[BLE_UUID_STR_LEN];
	uint16_t len = 0;
	ret = ble_hs_mbuf_to_flat(ctxt->om, uuid_str, BLE_UUID_STR_LEN, &len);
	if (ret != 0)
	{
		ESP_LOGE(LOG_TAG, "Failed to convert mbuf to flat: %d", ret);
		return BLE_ATT_ERR_UNLIKELY;
	}
	// We add the null-termination if missing
	if (len < BLE_UUID_STR_LEN)
	{
		uuid_str[len] = '\0';
	}
	// let's get the uuid
	uint8_t uuid_bytes[16];
	convert_uuid_to_bytes(uuid_str, uuid_bytes, 1);
	ble_uuid_any_t requested_claim_uuid;
	ret = ble_uuid_init_from_buf(&requested_claim_uuid, uuid_bytes, sizeof(uuid_bytes));
	if (ret != 0)
	{
		ESP_LOGE(LOG_TAG, "Failed to init uuid from buf: %d", ret);
		return BLE_ATT_ERR_UNLIKELY;
	}

	// TODO improve error reporting
	ret = cl_phy_lock_svc_request_claim(requested_claim_uuid.u128.value);
	if (ret != 0)
	{
		ESP_LOGE(LOG_TAG, "Failed to request claim: %d", ret);
		return BLE_ATT_ERR_UNLIKELY;
	}

	ESP_LOGI(LOG_TAG, "Requested claim for %s", uuid_str);

	return 0;
}

int cl_ble_lock_svc_req_release_char_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
	int ret;

	// we accept a uuid not null terminated as we will add it if missing
	if (om_len < (BLE_UUID_STR_LEN - 1) || om_len > BLE_UUID_STR_LEN)
	{
		ESP_LOGE(LOG_TAG, "Input mbuf not fitting uuid128 len: %d", om_len);
		return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
	}
	// we expect a string uuid in the format of 8-4-4-4-12 null-terminated
	char uuid_str[BLE_UUID_STR_LEN];
	uint16_t len = 0;
	ret = ble_hs_mbuf_to_flat(ctxt->om, uuid_str, BLE_UUID_STR_LEN, &len);
	if (ret != 0)
	{
		ESP_LOGE(LOG_TAG, "Failed to convert mbuf to flat: %d", ret);
		return BLE_ATT_ERR_UNLIKELY;
	}
	// We add the null-termination if missing
	if (len < BLE_UUID_STR_LEN)
	{
		uuid_str[len] = '\0';
	}
	// let's get the uuid
	uint8_t uuid_bytes[16];
	convert_uuid_to_bytes(uuid_str, uuid_bytes, 1);
	ble_uuid_any_t requested_release_uuid;
	ret = ble_uuid_init_from_buf(&requested_release_uuid, uuid_bytes, sizeof(uuid_bytes));
	if (ret != 0)
	{
		ESP_LOGE(LOG_TAG, "Failed to init uuid from buf: %d", ret);
		return BLE_ATT_ERR_UNLIKELY;
	}

	// TODO improve error reporting
	ret = cl_phy_lock_svc_request_release(requested_release_uuid.u128.value);
	if (ret != 0)
	{
		ESP_LOGE(LOG_TAG, "Failed to request release: %d", ret);
		return BLE_ATT_ERR_UNLIKELY;
	}

	ESP_LOGI(LOG_TAG, "Requested release for %s", uuid_str);

	return 0;
}

int cl_ble_lock_svc_init(void)
{
	int ret = ble_gatts_count_cfg(cl_ble_lock_svc_def);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "Failed GATT config: %d", ret);
		return ret;
	}

	ret = ble_gatts_add_svcs(cl_ble_lock_svc_def);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "Failed to add GATT service: %d", ret);
		return ret;
	}

	return ret;
}