// Library
#include <string.h>
// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
// ESP32
#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs.h"
// Local
#include "utils.h"
#include "cl_phy_lock_svc.h"

// -- DEFINES --
#define NVS_OWNER_NAMESPACE "LSVCO"					 //<! Lock Service Owner namespace used in NVS
#define NVS_OWNER_KEY_PREFIX "UUID_"				 //<! Lock Owner UUID key prefix used in NVS for each byte of the UUID
#define NVS_STATE_NAMESPACE "LSVCS"					 //<! Lock Service State namespace used in NVS
#define NVS_STATE_KEY "STATE"								 //<! Lock State key used in NVS
#define GPIO_QUEUE_PTR_SIZE sizeof(uint32_t) //<! Size of the GPIO queue ptrs
#define GPIO_QUEUE_PTR_NUM 10								 //<! Number of GPIO queue ptrs
#define GPIO_QUEUE_STACK 4096								 //<! Stack size of the GPIO queue task
#define GPIO_QUEUE_PRIO 10									 //<! Priority of the GPIO queue task

// -- INTERNAL FUNCTION DECLARATIONS --
static void gpio_isr_handler(void *arg);
static void process_gpio_queue(void *arg);
static inline void set_state(uint8_t state);
static void set_physical_lock_open(void);
static void set_physical_lock_closed(void);
static void set_alarm_on(void);
static void set_alarm_off(void);
static uint8_t read_physical_lock_position();
static esp_err_t load_ownership(uint8_t *uuid);
static esp_err_t save_ownership(const uint8_t *uuid);
static esp_err_t load_state(uint8_t *state);
static esp_err_t save_state(const uint8_t state);
static inline uint8_t is_null_uuid(const uint8_t *uuid);

// -- RUNTIME VARIABLES --
static const char *LOG_TAG = "physvc_lock";

static uint8_t null_owner[16] = {0};									 //!< 16 null-bytes used to clear the lock ownership
static uint8_t current_owner[16] = {0};								 //!< The current owner of the lock, 16 null-bytes otherwise
static uint8_t current_state = PHY_LOCK_STATE_UNKNOWN; //!< One of the PHY_LOCK_STATE_* value)s

static QueueHandle_t lock_gpio_queue = NULL; //!< Queue to handle GPIO events from ISR

int cl_phy_lock_svc_init()
{
	if (current_state != PHY_LOCK_STATE_UNKNOWN)
	{
		ESP_LOGE(LOG_TAG, "%s Lock service already initialized", __func__);
		return ESP_ERR_INVALID_STATE;
	}

	// TODO init servo

	// Lock sensor out
	gpio_reset_pin(LOCK_SENSOR_OUT_PIN);
	gpio_set_direction(LOCK_SENSOR_OUT_PIN, GPIO_MODE_OUTPUT);
	gpio_set_pull_mode(LOCK_SENSOR_OUT_PIN, GPIO_PULLUP_ONLY);
	gpio_set_level(LOCK_SENSOR_OUT_PIN, 1);
	// Lock sensor in
	gpio_reset_pin(LOCK_SENSOR_IN_PIN);
	gpio_set_direction(LOCK_SENSOR_IN_PIN, GPIO_MODE_INPUT);
	gpio_set_pull_mode(LOCK_SENSOR_IN_PIN, GPIO_PULLDOWN_ONLY);
	gpio_set_intr_type(LOCK_SENSOR_IN_PIN, GPIO_INTR_ANYEDGE);
	// Lock alarm
	gpio_reset_pin(LOCK_SENSOR_ALARM_PIN);
	gpio_set_direction(LOCK_SENSOR_ALARM_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(LOCK_SENSOR_ALARM_PIN, 0);
	ESP_LOGD(LOG_TAG, "%s Lock GPIOs initialized: OUT GPIO_%d -> IN GPIO_%d, ALR GPIO_%d", __func__, LOCK_SENSOR_OUT_PIN, LOCK_SENSOR_IN_PIN, LOCK_SENSOR_ALARM_PIN);

	// create a queue to handle gpio event from isr and run it in background
	lock_gpio_queue = xQueueCreate(GPIO_QUEUE_PTR_NUM, GPIO_QUEUE_PTR_SIZE);
	xTaskCreate(process_gpio_queue, "process_gpio_queue", GPIO_QUEUE_STACK, NULL, GPIO_QUEUE_PRIO, NULL);
	ESP_LOGD(LOG_TAG, "%s Interrupt queue initialized", __func__);

	// install interrupt on lock sensor to detect when the lock changes
	int ret = gpio_isr_handler_add(LOCK_SENSOR_IN_PIN, gpio_isr_handler, (void *)LOCK_SENSOR_IN_PIN);
	ESP_LOGD(LOG_TAG, "%s Interrupt handler attached to GPIO_%d", __func__, LOCK_SENSOR_IN_PIN);

	// Load the lock ownership into memory
	ret = load_ownership(current_owner);
	if (ret != ESP_OK)
	{
		// go into support mode
		set_state(PHY_LOCK_STATE_SUPPORT);
		ESP_LOGE(LOG_TAG, "%s Failed to load lock ownership: %s", __func__, esp_err_to_name(ret));
		return ret;
	}
	ESP_LOGD(LOG_TAG, "%s Lock ownership loaded: %s", __func__, UUID_TO_STRING(current_owner));

	// set state according to the ownership
	if (is_null_uuid(current_owner))
	{
		uint8_t current_position = read_physical_lock_position();
		ESP_LOGD(LOG_TAG, "%s Physical lock position read: %d", __func__, current_position);
		if (current_position == PHY_LOCK_POSITION_OPEN)
		{
			ESP_LOGE(LOG_TAG, "%s Lock has been tampered", __func__);
			set_state(PHY_LOCK_STATE_SUPPORT);
			// TODO: what should we do here?
			ESP_LOGI(LOG_TAG, "%s Alarm on", __func__);
			set_alarm_on();
			return ESP_ERR_INVALID_STATE;
		}
		else
		{
			set_state(PHY_LOCK_STATE_CLAIMED);
		}
	}
	else
	{
		set_state(PHY_LOCK_STATE_UNCLAIMED);
	}

	return ESP_OK;
}

uint8_t cl_phy_lock_svc_get_state(void)
{
	return current_state;
}

esp_err_t cl_phy_lock_svc_request_claim(uint8_t *uuid)
{
	switch (current_state)
	{
	case PHY_LOCK_STATE_UNCLAIMED:
		set_state(PHY_LOCK_STATE_REQUESTED_CLAIM);
		memcpy(current_owner, uuid, 16);
		ESP_LOGI(LOG_TAG, "%s Accept claim: unclaimed -> %s", __func__, UUID_TO_STRING(current_owner));

		// if the lock is closed, we need to open it first
		if (read_physical_lock_position() == PHY_LOCK_POSITION_CLOSED)
		{
			ESP_LOGD(LOG_TAG, "%s Opening physical lock", __func__);
			set_physical_lock_open();
		}

		return ESP_OK;
	}
	// TODO: handle other states with better error reporting?

	return ESP_ERR_INVALID_STATE;
}

esp_err_t cl_phy_lock_svc_request_release(uint8_t *uuid)
{
	switch (current_state)
	{
	case PHY_LOCK_STATE_CLAIMED:
		if (memcmp(current_owner, uuid, 16) == 0)
		{
			set_state(PHY_LOCK_STATE_REQUESTED_RELEASE);
			ESP_LOGI(LOG_TAG, "%s Accepted release: claimed -> %s", __func__, UUID_TO_STRING(uuid));

			// if the lock is closed, we need to open it first
			if (read_physical_lock_position() == PHY_LOCK_POSITION_CLOSED)
			{
				ESP_LOGD(LOG_TAG, "%s Opening physical lock", __func__);
				set_physical_lock_open();
			}

			return ESP_OK;
		}
		else
		{
			ESP_LOGE(LOG_TAG, "%s Rejected release: owner doesn't match", __func__);
			ESP_LOGD(LOG_TAG, "%s Current: %s <=> Requester: %s", __func__, UUID_TO_STRING(current_owner), UUID_TO_STRING(uuid));
			return ESP_ERR_INVALID_STATE;
		}
	}

	return ESP_ERR_INVALID_STATE;
}

/**
 * @internal
 * @brief Sets the state of the lock.
 *
 * @param state The new state of the lock.
 */
static inline void set_state(uint8_t state)
{
	ESP_LOGD(LOG_TAG, "%s State changed from %d -> %d", __func__, current_state, state);
	current_state = state;
}

/**
 * @internal
 * @brief Releases the physical lock so that it can be opened.
 *
 */
static void set_physical_lock_open()
{
	ESP_LOGD(LOG_TAG, "%s set lock open", __func__);
}

/**
 * @internal
 * @brief Locks the physical lock so that it can't be opened.
 *
 */
static void set_physical_lock_closed()
{
	ESP_LOGD(LOG_TAG, "%s set lock closed", __func__);
}

/**
 * @internal
 * @brief Turn on the lock alarm.
 * This function turns on the GPIO connected to the lock alarm.
 */
static void set_alarm_on()
{
	gpio_set_level(LOCK_SENSOR_ALARM_PIN, 1);
	ESP_LOGD(LOG_TAG, "%s set GPIO_%d on HIGH", __func__, LOCK_SENSOR_ALARM_PIN);
}

/**
 * @internal
 * @brief Turn off the lock alarm.
 * This function turns off the GPIO connected to the lock alarm.
 */
static void set_alarm_off()
{
	gpio_set_level(LOCK_SENSOR_ALARM_PIN, 0);
	ESP_LOGD(LOG_TAG, "%s set GPIO_%d on LOW", __func__, LOCK_SENSOR_ALARM_PIN);
}

/**
 * @internal
 * @brief Read the physical lock position.
 * This function reads the GPIO connected to the lock mechanism.
 *
 * @return 1 if the lock is locked, 0 if the lock is unlocked.
 */
static uint8_t read_physical_lock_position()
{
	return gpio_get_level(LOCK_SENSOR_IN_PIN);
}

/**
 * @internal
 * @brief Save the lock state to the NVS.
 *
 * @param state The lock state to save.
 */
static esp_err_t save_state(const uint8_t state)
{
	ESP_LOGD(LOG_TAG, "%s Saving lock state: %d", __func__, state);
	nvs_handle_t nvs_handle;
	esp_err_t ret = nvs_open(NVS_STATE_NAMESPACE, NVS_READWRITE, &nvs_handle);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s Error opening NVS handle: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = nvs_set_u8(nvs_handle, NVS_STATE_KEY, state);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s Error setting lock state: %s", __func__, esp_err_to_name(ret));
	}

	// if the operation failed
	if (ret != ESP_OK)
	{
		nvs_erase_all(nvs_handle);
	}

	ret = nvs_commit(nvs_handle);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s Error committing lock state: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	nvs_close(nvs_handle);
	return ESP_OK;
}

/**
 * @internal
 * @brief Load the lock state from the NVS.
 *
 * @param state The lock state to load.
 */
static esp_err_t load_state(uint8_t *state)
{
	ESP_LOGD(LOG_TAG, "%s Loading lock state", __func__);
	nvs_handle_t nvs_handle;
	esp_err_t ret = nvs_open(NVS_STATE_NAMESPACE, NVS_READONLY, &nvs_handle);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s Error opening NVS handle: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = nvs_get_u8(nvs_handle, NVS_STATE_KEY, state);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s Error getting lock state: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	nvs_close(nvs_handle);
	return ESP_OK;
}

/**
 * @internal
 * @brief Save the lock ownership to NVS.
 *
 * @return esp_err_t This is the results of the NVS operation.
 */
static esp_err_t save_ownership(const uint8_t *uuid)
{
	nvs_handle_t nvs_handle;
	esp_err_t ret = nvs_open(NVS_OWNER_NAMESPACE, NVS_READWRITE, &nvs_handle);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s Error opening NVS handle: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	for (int i = 0; i < 16; i++)
	{
		char key[32];
		sprintf(key, "%s%d", NVS_OWNER_KEY_PREFIX, i);
		ret = nvs_set_u8(nvs_handle, key, uuid[i]);
		if (ret != ESP_OK)
		{
			ESP_LOGE(LOG_TAG, "%s Error setting NVS value: %s", __func__, esp_err_to_name(ret));
			break;
		}
	}

	// if any of the set operations failed, erase all the values from the owner namespace
	if (ret != ESP_OK)
	{
		nvs_erase_all(nvs_handle);
	}

	// commit the changes to NVS
	ret = nvs_commit(nvs_handle);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s Error committing NVS: %s", __func__, esp_err_to_name(ret));
	}

	nvs_close(nvs_handle);
	return ret;
}

/**
 * @internal
 * @brief Load the lock ownership from NVS.
 *
 * @return esp_err_t This is the results of the NVS operation.
 */
static esp_err_t load_ownership(uint8_t *uuid)
{
	nvs_handle_t nvs_handle;
	esp_err_t ret = nvs_open(NVS_OWNER_NAMESPACE, NVS_READONLY, &nvs_handle);
	// if the no ownership is found, return esp_ok
	if (ret == ESP_ERR_NVS_NOT_FOUND)
	{
		ESP_LOGD(LOG_TAG, "%s No lock ownership found", __func__);
		return ESP_OK;
	}
	// otherwise process all other errors
	else if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s Error opening NVS: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	for (int i = 0; i < 16; i++)
	{
		char key[32];
		sprintf(key, "%s%d", NVS_OWNER_KEY_PREFIX, i);
		uint8_t value = 0;
		ret = nvs_get_u8(nvs_handle, key, &value);
		if (ret != ESP_OK)
		{
			ESP_LOGE(LOG_TAG, "%s Error getting NVS value: %s", __func__, esp_err_to_name(ret));
			break;
		}
		uuid[i] = value;
	}

	nvs_close(nvs_handle);
	return ret;
}

/**
 * @internal
 * @brief Returns 0 if the given uuid is the null uuid.
 */
static inline uint8_t is_null_uuid(const uint8_t *uuid)
{
	return memcmp(uuid, null_owner, 16) != 0;
}

static void lock_sensor_trigger()
{
	uint8_t read_position = read_physical_lock_position();
	ESP_LOGD(LOG_TAG, "%s Lock sensor triggered: %d", __func__, read_position);
	switch (current_state)
	{
	case PHY_LOCK_STATE_REQUESTED_CLAIM:
		// it has been locked
		if (read_position == PHY_LOCK_POSITION_CLOSED)
		{
			// close the lock
			set_physical_lock_closed();
			// commit the ownership
			ESP_LOGI(LOG_TAG, "%s Commit ownership: %s", __func__, UUID_TO_STRING(current_owner));
			esp_err_t ret = save_ownership(current_owner);
			if (ret != ESP_OK)
			{
				ESP_LOGE(LOG_TAG, "%s Error saving ownership: %s", __func__, esp_err_to_name(ret));
				// release the lock
				set_physical_lock_open();
				// TODO: somehow report error
				return;
			}

			// set the state to claimed
			set_state(PHY_LOCK_STATE_CLAIMED);
			// TODO: somehow notify success
		}
		break;
	case PHY_LOCK_STATE_CLAIMED:
		// TODO implement proper handling of alarm and tamper notification
		// it has been unlocked / tempered with
		if (read_position == PHY_LOCK_POSITION_OPEN)
		{
			ESP_LOGI(LOG_TAG, "%s Alarm on", __func__);
			set_alarm_on();
		}
		// NOTE ideally we don't turn off the alarm once it has been triggered
		else
		{
			ESP_LOGI(LOG_TAG, "%s Alarm off", __func__);
			set_alarm_off();
		}
		break;
	case PHY_LOCK_STATE_REQUESTED_RELEASE:
		// TODO: if it has been opened after the requested release, clear the ownership and move to unclaimed state
		// it has been opened, means we commit the release
		if (read_position == PHY_LOCK_POSITION_OPEN)
		{
			// clear the ownership
			ESP_LOGI(LOG_TAG, "%s Clear ownership", __func__);
			esp_err_t ret = save_ownership(null_owner);
			if (ret != ESP_OK)
			{
				ESP_LOGE(LOG_TAG, "%s Error clearing ownership: %s", __func__, esp_err_to_name(ret));
				// TODO: somehow report error
				return;
			}
			// clear the current owner memory address
			memcpy(current_owner, null_owner, 16);
			set_state(PHY_LOCK_STATE_UNCLAIMED);
		}
		// TODO handle if the position is closed (should not happen)
		break;
	}
}

/**
 * @internal
 * @brief Handle the lock GPIO interrupt and queue it for processing with the GPIO num as param.
 */
static void gpio_isr_handler(void *arg)
{
	uint32_t gpio_num = (intptr_t)arg;
	xQueueSendFromISR(lock_gpio_queue, &gpio_num, NULL);
}

/**
 * @internal
 * @brief Process the lock GPIO queue.
 * This function is called from the @{gpio_isr_handler} and processes the GPIO queue.
 *
 * @param arg This is the GPIO number that triggered the interrupt.
 */
static void process_gpio_queue(void *arg)
{
	uint32_t gpio_num;
	// debounce variables to prevent multiple triggers
	TickType_t lastChangeTime = xTaskGetTickCount();
	TickType_t debounceDelay = pdMS_TO_TICKS(200);
	TickType_t currentTime;
	TickType_t elapsedTime;

	// Loop forever
	for (;;)
	{
		if (xQueueReceive(lock_gpio_queue, &gpio_num, portMAX_DELAY))
		{
			// Debounce the signal
			currentTime = xTaskGetTickCount();
			elapsedTime = currentTime - lastChangeTime;

			// Only process the signal if it has been stable for longer than the debounce delay
			if (elapsedTime >= debounceDelay)
			{
				switch (gpio_num)
				{
				case LOCK_SENSOR_IN_PIN:
					lock_sensor_trigger();
					break;

				default:
					break;
				}
			}

			// Update the last change time
			lastChangeTime = currentTime;
		}
	}
}
