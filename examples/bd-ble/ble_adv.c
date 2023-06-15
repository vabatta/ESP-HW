#include "ble_adv.h"
#include "ble/lock_gatts.h"
#include "ble_constants.h"

esp_ble_adv_params_t cubelock_adv_params = {
		.adv_int_min = 0x100,
		.adv_int_max = 0x100,
		.adv_type = ADV_TYPE_IND,
		.own_addr_type = BLE_ADDR_TYPE_RPA_PUBLIC,
		.channel_map = ADV_CHNL_ALL,
		.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

esp_ble_adv_data_t cubelock_adv_config = {
		.set_scan_rsp = false,
		.include_name = true,
		.include_txpower = true,
		.min_interval = 0x0006, // slave connection min interval, Time = min_interval * 1.25 msec
		.max_interval = 0x0010, // slave connection max interval, Time = max_interval * 1.25 msec
		.manufacturer_len = sizeof(manufacturer_name),
		.p_manufacturer_data = manufacturer_name,
		.service_uuid_len = sizeof(lock_service_uuid),
		.p_service_uuid = lock_service_uuid,
		.flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

esp_ble_adv_data_t cubelock_scan_rsp_config = {
		.set_scan_rsp = true,
		// .include_name = true,
		// .manufacturer_len = sizeof(manufacturer_name),
		// .p_manufacturer_data = manufacturer_name,
};
