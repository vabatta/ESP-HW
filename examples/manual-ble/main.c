#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "sdkconfig.h"
#include "ble/ble.h"

void app_main(void)
{
	int ret = ble_init();
	if (ret != ESP_OK)
	{
		ESP_LOGE("CUBELOCK", "BLE init failed");
		return;
	}
	ESP_LOGI("CUBELOCK", "BLE init success");
}
