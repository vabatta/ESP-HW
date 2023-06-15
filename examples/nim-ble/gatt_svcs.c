#include "esp_log.h"
#include "host/ble_hs.h"
#include "services/gatt/ble_svc_gatt.h"
#include "gatt_svcs.h"
#include "ble_svc.h"

const char *LOG_TAG = "GATT";

const struct ble_gatt_svc_def gatt_svcs[] = {
		/* The locking/unlocking service */
		{
				.type = BLE_GATT_SVC_TYPE_PRIMARY,
				.uuid = BLE_UUID128_DECLARE(BLE_DEVICE_LOCK_SVC_UUID),
				.characteristics = (struct ble_gatt_chr_def[]){
						{.uuid = BLE_UUID128_DECLARE(BLE_DEVICE_LOCK_CHAR_UUID),
						 .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC,
						 .access_cb = ble_device_lock_char_write},
						{0},
				},
		},

		{0}, /* No more services */
};

int ble_device_lock_char_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	uint16_t len;
	uint8_t buf[16];

	len = OS_MBUF_PKTLEN(ctxt->om);
	if (len > sizeof(buf))
	{
		return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
	}

	/* Copy data from attribute to local buffer */
	ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &len);

	/* Handle the write request. For example, "1" for lock and "0" for unlock */
	if (buf[0] == '1')
	{
		ESP_LOGI(LOG_TAG, "Device locked");
	}
	else if (buf[0] == '0')
	{
		ESP_LOGI(LOG_TAG, "Device unlocked");
	}
	else
	{
		ESP_LOGI(LOG_TAG, "Invalid command");
		return BLE_ATT_ERR_UNLIKELY;
	}

	return 0;
}
