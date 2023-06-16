// ESP32 and utils
#include "esp_log.h"
#include "utils.h"
// Bluetooth host stack
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
// Other BT related services
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
// Local
#include "cl_ble_svc.h"
#include "gatts/cl_ble_lock_svc.h"

// -- INTERNAL FUNCTIONS --
void ble_advertise(void);

// -- RUNTIME VARIABLES
static const char *LOG_TAG = "blesvc";

uint8_t own_addr_type;

int ble_gap_event(struct ble_gap_event *event, void *arg)
{
	// struct ble_gap_conn_desc desc;
	// int ret;

	switch (event->type)
	{
	case BLE_GAP_EVENT_CONNECT:
		// A new connection was established or a connection attempt failed.
		ESP_LOGI(LOG_TAG, "connection %s; status=%d ",
						 event->connect.status == 0 ? "established" : "failed",
						 event->connect.status);
		if (event->connect.status != 0)
		{
			// Connection has failed, resume advertising.
			ble_advertise();
		}
		return 0;

	case BLE_GAP_EVENT_DISCONNECT:
		ESP_LOGI(LOG_TAG, "disconnect; reason=%d ", event->disconnect.reason);
		// Connection was terminated, resume advertising.
		ble_advertise();
		return 0;

	case BLE_GAP_EVENT_ADV_COMPLETE:
		ESP_LOGI(LOG_TAG, "advertise complete; reason=%d", event->adv_complete.reason);
		// Advertise was terminated, resume advertising.
		ble_advertise();
		return 0;

	case BLE_GAP_EVENT_CONN_UPDATE:
		/* The central has updated the connection parameters. */
		ESP_LOGI(LOG_TAG, "connection updated; status=%d ", event->conn_update.status);
		return 0;

	case BLE_GAP_EVENT_ENC_CHANGE:
		/* Encryption has been enabled or disabled for this connection. */
		ESP_LOGI(LOG_TAG, "encryption change event; status=%d ", event->enc_change.status);
		return 0;

	case BLE_GAP_EVENT_NOTIFY_TX:
		ESP_LOGI(LOG_TAG, "notify_tx event; conn_handle=%d attr_handle=%d "
											"status=%d is_indication=%d",
						 event->notify_tx.conn_handle,
						 event->notify_tx.attr_handle,
						 event->notify_tx.status,
						 event->notify_tx.indication);
		return 0;

	case BLE_GAP_EVENT_SUBSCRIBE:
		ESP_LOGI(LOG_TAG, "subscribe event; conn_handle=%d attr_handle=%d "
											"reason=%d prevn=%d curn=%d previ=%d curi=%d",
						 event->subscribe.conn_handle,
						 event->subscribe.attr_handle,
						 event->subscribe.reason,
						 event->subscribe.prev_notify,
						 event->subscribe.cur_notify,
						 event->subscribe.prev_indicate,
						 event->subscribe.cur_indicate);
		return 0;

	case BLE_GAP_EVENT_MTU:
		ESP_LOGI(LOG_TAG, "mtu update event; conn_handle=%d cid=%d mtu=%d",
						 event->mtu.conn_handle,
						 event->mtu.channel_id,
						 event->mtu.value);
		return 0;

	default:
		ESP_LOGI(LOG_TAG, "Unhandled GAP event; type=%d", event->type);
	}

	return 0;
}

void ble_advertise(void)
{
	int ret;

	struct ble_hs_adv_fields fields;
	memset(&fields, 0, sizeof fields);

	/* Advertise two flags:
	 *     o Discoverability in forthcoming advertisement (general)
	 *     o BLE-only (BR/EDR unsupported).
	 */
	fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

	const char *name = ble_svc_gap_device_name();
	fields.name = (uint8_t *)name;
	fields.name_len = strlen(name);
	fields.name_is_complete = 1;
	fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
	fields.tx_pwr_lvl_is_present = 1;

	ret = ble_gap_adv_set_fields(&fields);
	if (ret != 0)
	{
		ESP_LOGE(LOG_TAG, "error setting advertisement data; ret=%d", ret);
		return;
	}

	struct ble_gap_adv_params adv_params;
	memset(&adv_params, 0, sizeof adv_params);
	adv_params.filter_policy = BLE_HCI_SCAN_FILT_NO_WL;
	adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
	adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

	// Begin advertising.
	ret = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
	if (ret != 0)
	{
		ESP_LOGE(LOG_TAG, "error enabling advertisement; ret=%d", ret);
		return;
	}
}

void ble_on_reset(int reason)
{
	ESP_LOGE(LOG_TAG, "Resetting state; reason=%d", reason);
}

void ble_on_sync(void)
{
	int ret;
	ble_addr_t addr;

	// generate new non-resolvable private address
	ret = ble_hs_id_gen_rnd(0, &addr);
	if (ret != 0)
	{
		ESP_LOGE(LOG_TAG, "error generating random address; ret=%d", ret);
		return;
	}

	// set generated address
	ret = ble_hs_id_set_rnd(addr.val);
	if (ret != 0)
	{
		ESP_LOGE(LOG_TAG, "error setting random address; ret=%d", ret);
		return;
	}

	// Make sure we have proper identity address set (random preferred)
	ret = ble_hs_util_ensure_addr(1);
	if (ret != 0)
	{
		ESP_LOGE(LOG_TAG, "error determining address; ret=%d", ret);
		return;
	}

	// Figure out address to use while advertising (privacy enabled)
	ret = ble_hs_id_infer_auto(1, &own_addr_type);
	if (ret != 0)
	{
		ESP_LOGE(LOG_TAG, "error determining address type; ret=%d", ret);
		return;
	}

	ESP_LOGI(LOG_TAG, "BT device address set; addr=%s", ADDR_TO_STRING(addr.val));

	// Begin advertising after sync
	ble_advertise();
}

void ble_host_task(void *param)
{
	ESP_LOGI(LOG_TAG, "BLE Host Task Started");
	// This function will return only when nimble_port_stop() is executed
	nimble_port_run();
	// Free task memory
	nimble_port_freertos_deinit();
}

esp_err_t cl_ble_svc_init()
{
	// Holder for initialization result.
	esp_err_t ret;

	// Initialize NimBLE host & controller stack
	ret = nimble_port_init();
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "Failed to init nimble; ret=%d ", ret);
		return ret;
	}

	// Set GAP device name
	ret = ble_svc_gap_device_name_set(CL_BLE_DEVICE_NAME);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "Failed to set device name; ret=%d ", ret);
		return ret;
	}

	// Initialize the GAP service
	ble_svc_gap_init();
	// Initialize the GATT service
	ble_svc_gatt_init();

	// Add GATT services
	ret = cl_ble_lock_svc_init();

	// Check that all services were added, each reports failure individually
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "Failed to init services; ret=%d ", ret);
		return ret;
	}

	// Host configs
	ble_hs_cfg.reset_cb = ble_on_reset;
	ble_hs_cfg.sync_cb = ble_on_sync;
	// ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
	ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

	// Security Manager configs
	ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;
	ble_hs_cfg.sm_sc = 1;
	ble_hs_cfg.sm_mitm = 1;
	ble_hs_cfg.sm_bonding = 1;
	ble_hs_cfg.sm_our_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC;
	ble_hs_cfg.sm_their_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC;
	ble_hs_cfg.sm_our_key_dist |= BLE_SM_PAIR_KEY_DIST_ID;
	ble_hs_cfg.sm_their_key_dist |= BLE_SM_PAIR_KEY_DIST_ID;

	// Start the task that will handle NimBLE host events
	nimble_port_freertos_init(ble_host_task);

	return ESP_OK;
};
