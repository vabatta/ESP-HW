#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *LOG_TAG = "INFO";

void print_device_info(void)
{
	// Get flash size
	uint32_t flash_size;
	int ret = esp_flash_get_size(NULL, &flash_size);
	if (ret != ESP_OK)
	{
		ESP_LOGE(LOG_TAG, "%s get flash size failed: %d", __func__, ret);
		return;
	}

	// Get chip information
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	unsigned major_rev = chip_info.revision / 100;
	unsigned minor_rev = chip_info.revision % 100;

	// Print information using ESP_LOG
	ESP_LOGD(LOG_TAG, "ESP32 System Information:");
	ESP_LOGD(LOG_TAG, "  Chip model: %s", CONFIG_IDF_TARGET);
	ESP_LOGD(LOG_TAG, "  Cores: %d", chip_info.cores);
	// for (int i = 0; i < chip_info.cores; i++)
	// {
	// 	ESP_LOGD(LOG_TAG, "  Core %d frequency: %" PRIu32 " MHz", i, esp_clk_cpu_freq() / 1000000);
	// }
	ESP_LOGD(LOG_TAG, "  Silicon revision v%d.%d", major_rev, minor_rev);
	ESP_LOGD(LOG_TAG, "  Features: %s%s%s", (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "Wi-Fi " : "",
					 (chip_info.features & CHIP_FEATURE_BLE) ? "BLE " : "",
					 (chip_info.features & CHIP_FEATURE_BT) ? "Bluetooth " : "");

	ESP_LOGD(LOG_TAG, "  IDF version: %s", esp_get_idf_version());

	ESP_LOGD(LOG_TAG, "%" PRIu32 "MB %s flash", flash_size / (uint32_t)(1024 * 1024),
					 (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

	ESP_LOGD(LOG_TAG, "Available SRAM: %d bytes", heap_caps_get_free_size(MALLOC_CAP_8BIT));
	ESP_LOGD(LOG_TAG, "Available PSRAM: %d bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
	ESP_LOGD(LOG_TAG, "Minimum free heap size: %" PRIu32 " bytes", esp_get_minimum_free_heap_size());
}
