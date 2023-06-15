#ifndef _PHY_LOCK_SVC_H_
#define _PHY_LOCK_SVC_H_

#define LOCK_SENSOR_OUT_PIN GPIO_NUM_21		//!< GPIO pin from where the locks starts
#define LOCK_SENSOR_IN_PIN GPIO_NUM_47		//!< GPIO pin when lock is closed
#define LOCK_SENSOR_ALARM_PIN GPIO_NUM_48 //!< GPIO pin connected to the alarm, set on high on trigger

#define ESP_ERR_PHY_LOCK_BASE 0x11000
#define ESP_ERR_PHY_LOCK_INVALID_POSITION 0x11001

#define PHY_LOCK_STATE_UNKNOWN 255
#define PHY_LOCK_STATE_UNCLAIMED 0
#define PHY_LOCK_STATE_CLAIMED 1
#define PHY_LOCK_STATE_REQUESTED_CLAIM 2
#define PHY_LOCK_STATE_REQUESTED_RELEASE 3
#define PHY_LOCK_STATE_SUPPORT 4

#define PHY_LOCK_POSITION_UNKNOWN 255
#define PHY_LOCK_POSITION_OPEN 0
#define PHY_LOCK_POSITION_CLOSED 1

/**
 * Initialize the lock by setting up the GPIO pins connected to the lock, the step motor and loading state.
 * It also load data from the NVS flash back into memory, and initialize the lock state.
 *
 * @note This function must be called before any other phy_lock_svc function.
 *
 * @return Returns ESP_OK if successful, otherwise esp_err accordingly.
 */
extern int phy_lock_svc_init(void);

/**
 * Get the current state of the lock.
 *
 * @return Returns the current state of the lock.
 */
extern uint8_t phy_lock_svc_get_state(void);

/**
 * Request to claim the lock to a certain owner identified by UUID.
 *
 * @param uuid The UUID of the owner.
 *
 * @return Returns ESP_OK if successful. ESP_ERR_INVALID_STATE if the lock cannot be claimed from
 * its current state.
 */
extern esp_err_t phy_lock_svc_request_claim(uint8_t *uuid);

/**
 * Request to release the lock from a certain owner identified by UUID.
 *
 * @param uuid The UUID of the owner.
 *
 * @return Returns ESP_OK if successful. ESP_ERR_INVALID_STATE if the lock cannot be released from
 * its current state.
 */
extern esp_err_t phy_lock_svc_request_release(uint8_t *uuid);

#endif // _PHY_LOCK_SVC_H_
