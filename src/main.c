// Library
#include <stdio.h>
// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
// ESP32
#include "sdkconfig.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
// Local
#include "device_info.h"
#include "cl_ble_svc.h"
#include "cl_phy_lock_svc.h"

// -- RUNTIME VARIABLES --
static const char *LOG_TAG = "main";

void app_main(void)
{
	esp_err_t ret;

	print_device_info();

	// Initialize NVS flash memory.
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "Failed to init nvs; ret=%s", esp_err_to_name(ret));
		return;
	}
	ESP_LOGI(LOG_TAG, "NVS flash initialized");

	// Initialize the GPIO interrupt service.
	ret = gpio_install_isr_service(ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_SHARED);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "Failed to install GPIO isr service; ret=%s", esp_err_to_name(ret));
		return;
	}
	ESP_LOGI(LOG_TAG, "GPIO isr service installed");

	// Initialize the physical lock.
	ret = cl_phy_lock_svc_init();
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "Failed to init physical lock; ret=%s", esp_err_to_name(ret));
		return;
	}
	ESP_LOGI(LOG_TAG, "Physical lock initialized");

	// Initialize the BLE service.
	ret = cl_ble_svc_init();
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "BLE init failed; ret=%s", esp_err_to_name(ret));
		return;
	}
	ESP_LOGI(LOG_TAG, "BLE init success");
}
