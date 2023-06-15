#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *LOG_TAG = "device_info";

void print_device_info(void)
{
	// Get flash size
	uint32_t flash_size;
	int ret = esp_flash_get_size(NULL, &flash_size);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "get flash size failed; ret=%d", ret);
		return;
	}

	// Get chip information
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);

	// Print information using ESP_LOG
	ESP_LOGD(LOG_TAG, "ESP32 System Information:");
	ESP_LOGD(LOG_TAG, "  Chip model: %s", CONFIG_IDF_TARGET);
	ESP_LOGD(LOG_TAG, "  Cores: %d", chip_info.cores);
	ESP_LOGD(LOG_TAG, "  Features: %s%s%s",
					 (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "Wi-Fi " : "",
					 (chip_info.features & CHIP_FEATURE_BLE) ? "BLE " : "",
					 (chip_info.features & CHIP_FEATURE_BT) ? "Bluetooth " : "");

	ESP_LOGD(LOG_TAG, "  IDF version: %s", esp_get_idf_version());

	ESP_LOGD(LOG_TAG, "  Flash: %" PRIu32 "MB %s", flash_size / (uint32_t)(1024 * 1024),
					 (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

	ESP_LOGD(LOG_TAG, "  Available SRAM: %d bytes", heap_caps_get_free_size(MALLOC_CAP_8BIT));
	ESP_LOGD(LOG_TAG, "  Available PSRAM: %d bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
	ESP_LOGD(LOG_TAG, "  Minimum free heap: %" PRIu32 " bytes", esp_get_minimum_free_heap_size());
}
