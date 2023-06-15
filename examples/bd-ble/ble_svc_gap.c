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
#include "ble/lock_gatts.h"

static const char *LOG_TAG = "BLE_SVC_GAP";

static char *esp_auth_req_to_str(esp_ble_auth_req_t auth_req)
{
	char *auth_str = NULL;
	switch (auth_req)
	{
	case ESP_LE_AUTH_NO_BOND:
		auth_str = "ESP_LE_AUTH_NO_BOND";
		break;
	case ESP_LE_AUTH_BOND:
		auth_str = "ESP_LE_AUTH_BOND";
		break;
	case ESP_LE_AUTH_REQ_MITM:
		auth_str = "ESP_LE_AUTH_REQ_MITM";
		break;
	case ESP_LE_AUTH_REQ_BOND_MITM:
		auth_str = "ESP_LE_AUTH_REQ_BOND_MITM";
		break;
	case ESP_LE_AUTH_REQ_SC_ONLY:
		auth_str = "ESP_LE_AUTH_REQ_SC_ONLY";
		break;
	case ESP_LE_AUTH_REQ_SC_BOND:
		auth_str = "ESP_LE_AUTH_REQ_SC_BOND";
		break;
	case ESP_LE_AUTH_REQ_SC_MITM:
		auth_str = "ESP_LE_AUTH_REQ_SC_MITM";
		break;
	case ESP_LE_AUTH_REQ_SC_MITM_BOND:
		auth_str = "ESP_LE_AUTH_REQ_SC_MITM_BOND";
		break;
	default:
		auth_str = "INVALID BLE AUTH REQ";
		break;
	}

	return auth_str;
}

void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
	switch (event)
	{
	case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
		// advertising start complete event to indicate advertising start successfully or failed
		if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
		{
			ESP_LOGE(LOG_TAG, "advertising start failed, error status = %x", param->adv_start_cmpl.status);
			break;
		}
		ESP_LOGI(LOG_TAG, "advertising start success");
		break;
	case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
		if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
		{
			ESP_LOGE(LOG_TAG, "Advertising stop failed, error status = %x", param->adv_stop_cmpl.status);
			break;
		}
		ESP_LOGI(LOG_TAG, "advertising stop success");
		break;
	case ESP_GAP_BLE_PASSKEY_REQ_EVT: /* passkey request event */
		ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_PASSKEY_REQ_EVT");
		/* Call the following function to input the passkey which is displayed on the remote device */
		// esp_ble_passkey_reply(heart_rate_profile_tab[HEART_PROFILE_APP_IDX].remote_bda, true, 0x00);
		break;
	case ESP_GAP_BLE_OOB_REQ_EVT:
	{
		ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_OOB_REQ_EVT");
		uint8_t tk[16] = {1}; // If you paired with OOB, both devices need to use the same tk
		esp_ble_oob_req_reply(param->ble_security.ble_req.bd_addr, tk, sizeof(tk));
		break;
	}
	case ESP_GAP_BLE_NC_REQ_EVT:
		/* The app will receive this evt when the IO has DisplayYesNO capability and the peer device IO also has DisplayYesNo capability.
		show the passkey number to the user to confirm it with the number displayed by peer device. */
		esp_ble_confirm_reply(param->ble_security.ble_req.bd_addr, true);
		ESP_LOGI(LOG_TAG, "ESP_GAP_BLE_NC_REQ_EVT, the passkey Notify number:%" PRIu32, param->ble_security.key_notif.passkey);
		break;
	case ESP_GAP_BLE_SEC_REQ_EVT:
		/* send the positive(true) security response to the peer device to accept the security request.
		If not accept the security request, should send the security response with negative(false) accept value*/
		esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
		break;
	case ESP_GAP_BLE_PASSKEY_NOTIF_EVT: /// the app will receive this evt when the IO  has Output capability and the peer device IO has Input capability.
		/// show the passkey number to the user to input it in the peer device.
		ESP_LOGI(LOG_TAG, "The passkey Notify number:%06" PRIu32, param->ble_security.key_notif.passkey);
		break;
	case ESP_GAP_BLE_KEY_EVT:
		// shows the ble key info share with peer device to the user.
		// ESP_LOGI(LOG_TAG, "key type = %s", esp_key_type_to_str(param->ble_security.ble_key.key_type));
		break;
	case ESP_GAP_BLE_AUTH_CMPL_EVT:
		esp_bd_addr_t bd_addr;
		memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
		ESP_LOGI(LOG_TAG, "remote BD_ADDR: %08x%04x",
						 (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
						 (bd_addr[4] << 8) + bd_addr[5]);
		ESP_LOGI(LOG_TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
		ESP_LOGI(LOG_TAG, "pair status = %s", param->ble_security.auth_cmpl.success ? "success" : "fail");
		if (!param->ble_security.auth_cmpl.success)
		{
			ESP_LOGI(LOG_TAG, "fail reason = 0x%x", param->ble_security.auth_cmpl.fail_reason);
		}
		else
		{
			ESP_LOGI(LOG_TAG, "auth mode = %s", esp_auth_req_to_str(param->ble_security.auth_cmpl.auth_mode));
		}
		// show_bonded_devices();
		break;

	default:
		ESP_LOGD(LOG_TAG, "%s GAP Unhandled %d", __func__, event);
		break;
	}
}
