#include "esp_log.h"
#include "nvs_flash.h"
#include "host/ble_hs.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gatt/ble_svc_gatt.h"
#include "services/gap/ble_svc_gap.h"

#include "ble.h"
#include "gatt_svcs.h"

void ble_advertise(void);
uint8_t own_addr_type;

int ble_gap_event(struct ble_gap_event *event, void *arg)
{
	struct ble_gap_conn_desc desc;
	int rc;

	switch (event->type)
	{
	case BLE_GAP_EVENT_CONNECT:
		/* A new connection was established or a connection attempt failed. */
		ESP_LOGI(BLE_MODULE_TAG, "connection %s; status=%d ",
						 event->connect.status == 0 ? "established" : "failed",
						 event->connect.status);
		if (event->connect.status == 0)
		{
			rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
			assert(rc == 0);
			// bleprph_print_conn_desc(&desc);
		}
		ESP_LOGI(BLE_MODULE_TAG, "\n");

		if (event->connect.status != 0)
		{
			/* Connection failed; resume advertising. */
			ble_advertise();
		}
		return 0;

	case BLE_GAP_EVENT_DISCONNECT:
		ESP_LOGI(BLE_MODULE_TAG, "disconnect; reason=%d ", event->disconnect.reason);
		// bleprph_print_conn_desc(&event->disconnect.conn);
		ESP_LOGI(BLE_MODULE_TAG, "\n");

		/* Connection terminated; resume advertising. */
		ble_advertise();
		return 0;

	case BLE_GAP_EVENT_CONN_UPDATE:
		/* The central has updated the connection parameters. */
		ESP_LOGI(BLE_MODULE_TAG, "connection updated; status=%d ", event->conn_update.status);
		rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
		assert(rc == 0);
		// bleprph_print_conn_desc(&desc);
		ESP_LOGI(BLE_MODULE_TAG, "\n");
		return 0;

	case BLE_GAP_EVENT_ADV_COMPLETE:
		ESP_LOGI(BLE_MODULE_TAG, "advertise complete; reason=%d", event->adv_complete.reason);
		ble_advertise();
		return 0;

	case BLE_GAP_EVENT_ENC_CHANGE:
		/* Encryption has been enabled or disabled for this connection. */
		ESP_LOGI(BLE_MODULE_TAG, "encryption change event; status=%d ", event->enc_change.status);
		rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
		assert(rc == 0);
		// bleprph_print_conn_desc(&desc);
		ESP_LOGI(BLE_MODULE_TAG, "\n");
		return 0;

	case BLE_GAP_EVENT_NOTIFY_TX:
		ESP_LOGI(BLE_MODULE_TAG, "notify_tx event; conn_handle=%d attr_handle=%d "
														 "status=%d is_indication=%d",
						 event->notify_tx.conn_handle,
						 event->notify_tx.attr_handle,
						 event->notify_tx.status,
						 event->notify_tx.indication);
		return 0;

	case BLE_GAP_EVENT_SUBSCRIBE:
		ESP_LOGI(BLE_MODULE_TAG, "subscribe event; conn_handle=%d attr_handle=%d "
														 "reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
						 event->subscribe.conn_handle,
						 event->subscribe.attr_handle,
						 event->subscribe.reason,
						 event->subscribe.prev_notify,
						 event->subscribe.cur_notify,
						 event->subscribe.prev_indicate,
						 event->subscribe.cur_indicate);
		return 0;

	case BLE_GAP_EVENT_MTU:
		ESP_LOGI(BLE_MODULE_TAG, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
						 event->mtu.conn_handle,
						 event->mtu.channel_id,
						 event->mtu.value);
		return 0;

	default:
		ESP_LOGI(BLE_MODULE_TAG, "Unhandled GAP event: %d\n", event->type);
	}

	return 0;
}

/**
 * Enables advertising with the following parameters:
 *     o General discoverable mode.
 *     o Undirected connectable mode.
 */
void ble_advertise(void)
{
	struct ble_gap_adv_params adv_params;
	struct ble_hs_adv_fields fields;
	const char *name;
	int rc;

	/**
	 *  Set the advertisement data included in our advertisements:
	 *     o Flags (indicates advertisement type and other general info).
	 *     o Advertising tx power.
	 *     o Device name.
	 *     o 16-bit service UUIDs (alert notifications).
	 */

	memset(&fields, 0, sizeof fields);

	/* Advertise two flags:
	 *     o Discoverability in forthcoming advertisement (general)
	 *     o BLE-only (BR/EDR unsupported).
	 */
	fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

	name = ble_svc_gap_device_name();
	fields.name = (uint8_t *)name;
	fields.name_len = strlen(name);
	fields.name_is_complete = 1;

	rc = ble_gap_adv_set_fields(&fields);
	if (rc != 0)
	{
		ESP_LOGE(BLE_MODULE_TAG, "error setting advertisement data; rc=%d\n", rc);
		return;
	}

	/* Begin advertising. */
	memset(&adv_params, 0, sizeof adv_params);
	adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
	adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
	rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
	if (rc != 0)
	{
		ESP_LOGE(BLE_MODULE_TAG, "error enabling advertisement; rc=%d\n", rc);
		return;
	}
}

void ble_on_reset(int reason)
{
	ESP_LOGE(BLE_MODULE_TAG, "Resetting state; reason=%d\n", reason);
}

void ble_on_sync(void)
{
	int rc;

	/* Make sure we have proper identity address set (public preferred) */
	// rc = ble_hs_util_ensure_addr(0);
	// assert(rc == 0);

	/* Figure out address to use while advertising (no privacy for now) */
	rc = ble_hs_id_infer_auto(0, &own_addr_type);
	if (rc != 0)
	{
		ESP_LOGE(BLE_MODULE_TAG, "error determining address type; rc=%d\n", rc);
		return;
	}

	/* Printing ADDR */
	// uint8_t addr_val[6] = {0};
	// rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);

	// ESP_LOGI(BLE_MODULE_TAG, "Device Address: ");
	// print_addr(addr_val);
	// ESP_LOGI(BLE_MODULE_TAG, "\n");
	/* Begin advertising. */
	ble_advertise();
}

void ble_host_task(void *param)
{
	ESP_LOGI(BLE_MODULE_TAG, "BLE Host Task Started");
	// This function will return only when nimble_port_stop() is executed
	nimble_port_run();
	// Free task memory
	nimble_port_freertos_deinit();
}

esp_err_t ble_svc_init()
{
	// Holder for initialization result.
	esp_err_t ret;

	// 1 - Initialize NVS — it is used to store PHY calibration data
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	else if (ret != ESP_OK)
	{
		ESP_LOGE(BLE_MODULE_TAG, "Failed to init nvs %d ", ret);
		return ret;
	}

	// 2 - Initialize NimBLE host & controller stack
	ret = nimble_port_init();
	if (ret != ESP_OK)
	{
		ESP_LOGE(BLE_MODULE_TAG, "Failed to init nimble %d ", ret);
		return ret;
	}

	// 3 - Initialize NimBLE configuration
	// 3.1 - Device name
	ret = ble_svc_gap_device_name_set(BLE_DEVICE_NAME);
	if (ret != ESP_OK)
	{
		ESP_LOGE(BLE_MODULE_TAG, "Failed to set device name %d ", ret);
		return ret;
	}
	// 3.2 - GAP service
	ble_svc_gap_init();
	// 3.3 - GATT service
	ble_svc_gatt_init();
	// 3.3.1 - Config GATT services
	ret = ble_gatts_count_cfg(gatt_svcs);
	if (ret != ESP_OK)
	{
		ESP_LOGE(BLE_MODULE_TAG, "Failed to count gatt config %d ", ret);
		return ret;
	}
	// 3.3.2 - Queues GATT services
	ret = ble_gatts_add_svcs(gatt_svcs);
	if (ret != ESP_OK)
	{
		ESP_LOGE(BLE_MODULE_TAG, "Failed to add gatt services %d ", ret);
		return ret;
	}
	// 3.4 - Host configs
	ble_hs_cfg.reset_cb = ble_on_reset;
	ble_hs_cfg.sync_cb = ble_on_sync;
	// ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
	ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

	ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_DISP_ONLY;
	ble_hs_cfg.sm_sc = 1;
	ble_hs_cfg.sm_mitm = 1;
	ble_hs_cfg.sm_bonding = 1;
	ble_hs_cfg.sm_our_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC;
	ble_hs_cfg.sm_their_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC;
	ble_hs_cfg.sm_our_key_dist |= BLE_SM_PAIR_KEY_DIST_ID;
	ble_hs_cfg.sm_their_key_dist |= BLE_SM_PAIR_KEY_DIST_ID;

	nimble_port_freertos_init(ble_host_task);

	return ESP_OK;
};
