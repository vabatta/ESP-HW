// C library includes.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// FreeRTOS libraries.
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
// ESP system libraries.
#include "esp_system.h"
#include "esp_log.h"
// Bluedroid base library.
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
// GATT and GAP libraries.
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
// Local includes.
#include "ble_svc.h"
#include "ble_svc_gap.h"
#include "ble_svc_gatts.h"
#include "ble_adv.h"

static const char *LOG_TAG = "BLE";

esp_err_t ble_svc_init()
{
	// Holder for initialization result.
	esp_err_t ret;

	ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s Failed to release BT classic controller memory %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	ret = esp_bt_controller_init(&bt_cfg);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s init controller failed: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = esp_bluedroid_init();
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = esp_bluedroid_enable();
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = esp_ble_gatts_register_callback(gatts_event_handler);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s gatts register error: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = esp_ble_gap_register_callback(gap_event_handler);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s gap register error: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = esp_ble_gatts_app_register(SINGLE_APP_ID);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s gatts app register error: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = esp_ble_gap_set_device_name(BLE_DEVICE_NAME);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s gap set device name error: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = esp_ble_gap_config_local_privacy(true);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s gap config local privacy error: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = esp_ble_gap_config_adv_data(&cubelock_adv_config);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s gap config adv data error: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = esp_ble_gap_config_adv_data(&cubelock_scan_rsp_config);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s gap config scan response data error: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	uint32_t passkey = BLE_PASSKEY;
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));

	esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
	esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));

	esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
	esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));

	uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
	esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));

	uint8_t oob_support = ESP_BLE_OOB_DISABLE;
	esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t));

	uint8_t key_size = 16;
	esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));

	uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));

	uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
	esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

	esp_ble_gap_start_advertising(&cubelock_adv_params);

	return ESP_OK;
};
