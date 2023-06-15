#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_bt.h>
#include <esp_gap_ble_api.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <host/ble_hs.h>

#define TAG "BLE_DEVICE"

/* Passkey for pairing */
#define PASSKEY "123456"

/* BLE device name */
#define DEVICE_NAME "MyBLEDevice"

/* Connection parameters */
#define CONN_INTERVAL_MIN 0x0008
#define CONN_INTERVAL_MAX 0x0010
#define CONN_LATENCY 0
#define SUPERVISION_TIMEOUT 0x012C

/* Advertising parameters */
#define ADV_INTERVAL_MS 100
#define ADV_DURATION_SEC 0

/* Service and characteristic UUIDs */
#define SERVICE_UUID 0x1234
#define CHARACTERISTIC_READ_UUID 0x5678
#define CHARACTERISTIC_WRITE_UUID 0x9ABC

/* BLE event handler */
static int ble_event_handler(struct ble_gap_event *event, void *arg)
{
	switch (event->type)
	{
	case BLE_GAP_EVENT_CONNECT:
		ESP_LOGI(TAG, "BLE_GAP_EVENT_CONNECT");
		break;
	case BLE_GAP_EVENT_DISCONNECT:
		ESP_LOGI(TAG, "BLE_GAP_EVENT_DISCONNECT");
		break;
	case BLE_GAP_EVENT_PASSKEY_ACTION:
		ESP_LOGI(TAG, "BLE_GAP_EVENT_PASSKEY_ACTION");
		if (event->passkey.params.action == BLE_SM_IOACT_DISP)
		{
			ESP_LOGI(TAG, "Passkey: %s", event->passkey.passkey);
		}
		break;
	default:
		break;
	}
	return 0;
}

/* Initialize BLE stack */
static void ble_stack_init()
{
	esp_err_t ret;

	/* Initialize NimBLE port */
	nimble_port_init();

	/* Initialize the NimBLE host stack */
	ret = ble_hs_init();
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to initialize BLE host stack");
		return;
	}

	/* Set the device name */
	ret = ble_svc_gap_device_name_set(DEVICE_NAME);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to set device name");
		return;
	}

	/* Set connection parameters */
	struct ble_gap_conn_params conn_params = {
			.itvl_min = CONN_INTERVAL_MIN,
			.itvl_max = CONN_INTERVAL_MAX,
			.latency = CONN_LATENCY,
			.supervision_timeout = SUPERVISION_TIMEOUT};
	ret = ble_gap_conn_params_set(&conn_params);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to set connection parameters");
		return;
	}

	/* Register BLE event handler */
	ble_hs_cfg.sync_cb = ble_event_handler;

	ESP_LOGI(TAG, "BLE stack initialized");
}

/* Start advertising */
static void ble_start_advertising()
{
	esp_err_t ret;

	/* Set advertising parameters */
	struct ble_gap_adv_params adv_params = {
			.conn_mode = BLE_GAP_CONN_MODE_UND,
			.disc_mode = BLE_GAP_DISC_MODE_NON,
			.itvl_min = ADV_INTERVAL_MS * 1000 / BLE_HCI_ADV_ITVL,
			.itvl_max = ADV_INTERVAL_MS * 1000 / BLE_HCI_ADV_ITVL,
			.channel_map = BLE_GAP_ADV_CHAN_ALL,
			.filter_policy = BLE_GAP_ADV_FP_ANY,
			.high_duty_cycle = 0,
	};

	/* Set advertising data */
	const uint8_t adv_data[] = {
			/* Flags */
			0x02, BLE_GAP_AD_TYPE_FLAGS, BLE_GAP_AD_FLAG_GENERAL_DISC | BLE_GAP_AD_FLAG_BR_EDR_NOT_SUPPORTED,
			/* Service UUID */
			0x03, BLE_GAP_AD_TYPE_16BIT_SVC_UUID_COMPLETE, 0x12, 0x18};
	ret = ble_gap_adv_set_data(adv_data, sizeof(adv_data));
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to set advertising data");
		return;
	}

	/* Start advertising */
	ret = ble_gap_adv_start(&adv_params, NULL, 0, NULL, NULL);
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to start advertising");
		return;
	}

	ESP_LOGI(TAG, "Advertising started");
}

/* Read characteristic callback */
static int read_char_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	const char *value = "Hello, GATT Client!";
	int len = strlen(value);

	ESP_LOGI(TAG, "Read request received for characteristic");

	/* Send the value of the characteristic */
	return os_mbuf_append(ctxt->om, value, len);
}

/* Write characteristic callback */
static int write_char_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	ESP_LOGI(TAG, "Write request received for characteristic");

	/* Print the received data */
	ESP_LOGI(TAG, "Received data: %.*s", OS_MBUF_PKTLEN(ctxt->om), (char *)OS_MBUF_PKTPOS(ctxt->om));

	/* Process the received data as needed */

	return 0;
}

/* Initialize and register the service */
static void ble_init_service()
{
	uint16_t service_handle;
	uint16_t read_char_handle;
	uint16_t write_char_handle;

	/* Create a new GATT service */
	ble_uuid16_t service_uuid;
	service_uuid.u.type = BLE_UUID_TYPE_16;
	service_uuid.u16 = SERVICE_UUID;
	ble_gatts_add_service(&service_uuid, BLE_GATT_SVC_TYPE_PRIMARY, &service_handle);

	/* Create a read characteristic */
	ble_uuid16_t read_char_uuid;
	read_char_uuid.u.type = BLE_UUID_TYPE_16;
	read_char_uuid.u16 = CHARACTERISTIC_READ_UUID;
	ble_gatts_add_characteristic(service_handle, &read_char_uuid, BLE_GATT_CHR_F_READ,
															 BLE_ATT_ERR_SUCCESS, NULL, NULL, &read_char_handle);

	/* Set the read characteristic's callback */
	ble_gatts_set_access_cb(read_char_handle, read_char_cb, NULL);

	/* Create a write characteristic */
	ble_uuid16_t write_char_uuid;
	write_char_uuid.u.type = BLE_UUID_TYPE_16;
	write_char_uuid.u16 = CHARACTERISTIC_WRITE_UUID;
	ble_gatts_add_characteristic(service_handle, &write_char_uuid, BLE_GATT_CHR_F_WRITE,
															 BLE_ATT_ERR_SUCCESS, NULL, NULL, &write_char_handle);

	/* Set the write characteristic's callback */
	ble_gatts_set_access_cb(write_char_handle, write_char_cb, NULL);

	ESP_LOGI(TAG, "Service and characteristics registered");
}

void app_main()
{
	/* Initialize BLE stack */
	ble_stack_init();

	/* Start advertising */
	ble_start_advertising();

	/* Initialize and register the service */
	ble_init_service();

	while (1)
	{
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
