#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "nvs_flash.h"

#include "ble_svc.h"
#include "phy_lock_svc.h"

extern esp_err_t ble_test(void);

static const char *LOG_TAG = "MAIN";

void app_main(void)
{
	esp_err_t ret;

	// Initialize NVS flash memory.
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s Failed to init nvs %s", __func__, esp_err_to_name(ret));
		return;
	}
	ESP_LOGI(LOG_TAG, "%s NVS flash initialized", __func__);

	// Initialize the GPIO interrupt service.
	ret = gpio_install_isr_service(ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_SHARED);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s Failed to install GPIO isr service %s", __func__, esp_err_to_name(ret));
		return;
	}
	ESP_LOGI(LOG_TAG, "%s GPIO isr service installed", __func__);

	// Initialize the physical lock.
	ret = phy_lock_svc_init();
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s Failed to init physical lock %s", __func__, esp_err_to_name(ret));
		return;
	}
	ESP_LOGI(LOG_TAG, "%s Physical lock initialized", __func__);

	// Initialize the BLE service.
	ret = ble_svc_init();
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s BLE init failed %s", __func__, esp_err_to_name(ret));
		return;
	}
	ESP_LOGI(LOG_TAG, "%s BLE init success", __func__);

	// ret = ble_test();
	// if (ret != ESP_OK)
	// {
	// 	ESP_LOGE(LOG_TAG, "%s BLE test failed %s", __func__, esp_err_to_name(ret));
	// 	return;
	// }

	// uint8_t example_lock_owner[16] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
	// ret = phy_lock_svc_request_release(example_lock_owner);
	// if (ret != ESP_OK)
	// {
	// 	ESP_LOGE(LOG_TAG, "%s Failed to request: %s", __func__, esp_err_to_name(ret));
	// 	return;
	// }
}
