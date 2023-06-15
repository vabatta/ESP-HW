#ifndef _CL_BLE_LOCK_SVC_H_
#define _CL_BLE_LOCK_SVC_H_

#include "host/ble_hs.h"
#include "cl_ble_svc.h"

extern const ble_uuid128_t cl_ble_lock_svc_uuid;

extern uint16_t cl_ble_lock_svc_state_char_val_handle;

extern uint16_t cl_ble_lock_svc_req_claim_char_val_handle;

extern uint16_t cl_ble_lock_svc_req_release_char_val_handle;

/**
 * Initialize the BLE lock service by adding it to the BLE GATT server db.
 */
extern int cl_ble_lock_svc_init(void);

#endif // _CL_BLE_LOCK_SVC_H_