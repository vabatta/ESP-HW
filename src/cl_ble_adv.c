#include "host/ble_hs.h"

const struct ble_gap_adv_params cl_ble_adv_params = {
		.filter_policy = BLE_HCI_SCAN_FILT_NO_WL,
		.conn_mode = BLE_GAP_CONN_MODE_UND,
		.disc_mode = BLE_GAP_DISC_MODE_GEN,
		.itvl_min = 0,
		.itvl_max = 0,
};

struct ble_hs_adv_fields cl_ble_adv_lock_svc = {
		.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO,
		.tx_pwr_lvl_is_present = 1,

};
