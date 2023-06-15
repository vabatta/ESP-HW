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
#include "ble_svc_gatts.h"
#include "utils.h"
// Services
#include "ble/lock_gatts.h"
#include "ble_adv.h"

static const char *LOG_TAG = "BLE_SVC_GATTS";

// Single profile for the application.
// static gatts_profile_inst profile_tab[PROFILE_NUM] = {
// 		[PROFILE_APP_IDX] = {
// 				// NOTE: we use one event handler instead of a gatts_profile_event_handler because we have only one profile.
// 				.gatts_cb = gatts_event_handler,
// 				.gatts_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
// 		},
// };

void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
	switch (event)
	{
	case ESP_GATTS_REG_EVT:
		if (param->reg.status == ESP_GATT_OK)
		{
			ESP_ERROR_CHECK(esp_ble_gatts_create_attr_tab(lock_gatts_svc_db, gatts_if, LOCK_SVC_IDX_NB, LOCK_SVC_INST_ID));
			ESP_LOGI(LOG_TAG, "register application successfully, app_id %04x, interface %d", param->reg.app_id, gatts_if);
		}
		else
		{
			ESP_LOGE(LOG_TAG, "register application failed, app_id %04x, status %d", param->reg.app_id, param->reg.status);
		}
		break;

	case ESP_GATTS_CREAT_ATTR_TAB_EVT:
		ESP_LOGD(LOG_TAG, "registering service %d", param->add_attr_tab.svc_inst_id); // UUID_TO_STRING(param->add_attr_tab.svc_uuid.uuid.uuid128)
		if (param->add_attr_tab.status != ESP_GATT_OK)
		{
			ESP_LOGE(LOG_TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
		}
		else if (param->add_attr_tab.num_handle != LOCK_SVC_IDX_NB)
		{
			ESP_LOGE(LOG_TAG, "create attribute table abnormally, num_handle (%d) doesn't equal to LOCK_SVC_IDX_NB(%d)", param->add_attr_tab.num_handle, LOCK_SVC_IDX_NB);
		}
		else
		{
			memcpy(lock_gatts_svc_handle_table, param->add_attr_tab.handles, sizeof(lock_gatts_svc_handle_table));
			ESP_ERROR_CHECK(esp_ble_gatts_start_service(lock_gatts_svc_handle_table[LOCK_SVC_IDX]));
		}
		break;

	case ESP_GATTS_CONNECT_EVT:
		esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
		break;

	case ESP_GATTS_DISCONNECT_EVT:
		esp_ble_gap_start_advertising(&cubelock_adv_params);
		break;

	case ESP_GATTS_READ_EVT:
		lock_gatts_svc_read_handler(event, gatts_if, param);
		break;

	// case ESP_GATTS_WRITE_EVT:
	// {
	// 	ESP_LOGD(LOG_TAG, "%s ESP_GATTS_WRITE_EVT", __func__);
	// 	break;
	// }
	//... add other event cases here
	default:
		ESP_LOGD(LOG_TAG, "%s GATT Unhandled %d", __func__, event);
		break;
	}
}
