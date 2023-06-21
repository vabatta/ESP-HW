#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "step_motor.h"

static const char *LOG_TAG = "step_motor";

static const uint8_t STEP_MATRIX[4][4] = {
		{1, 1, 0, 0},
		{0, 1, 1, 0},
		{0, 0, 1, 1},
		{1, 0, 0, 1}};

esp_err_t step_motor_init()
{
	// Resets the GPIO pins to their default state.
	gpio_reset_pin(STEP_DRIVER_PIN_IN1);
	gpio_reset_pin(STEP_DRIVER_PIN_IN2);
	gpio_reset_pin(STEP_DRIVER_PIN_IN3);
	gpio_reset_pin(STEP_DRIVER_PIN_IN4);
	// Sets the GPIO pins to output mode.
	gpio_set_direction(STEP_DRIVER_PIN_IN1, GPIO_MODE_OUTPUT);
	gpio_set_direction(STEP_DRIVER_PIN_IN2, GPIO_MODE_OUTPUT);
	gpio_set_direction(STEP_DRIVER_PIN_IN3, GPIO_MODE_OUTPUT);
	gpio_set_direction(STEP_DRIVER_PIN_IN4, GPIO_MODE_OUTPUT);

	ESP_LOGD(LOG_TAG, "GPIO pins initialized");

	return ESP_OK;
}

void step_motor_step(int count)
{
	uint8_t direction = (count >= 0) ? 1 : 0;
	count = abs(count);

	int start_index = (direction == 1) ? 0 : 3;
	int end_index = (direction == 1) ? 4 : -1;
	int step = (direction == 1) ? 1 : -1;

	for (int x = 0; x < count; x++)
	{
		for (int i = start_index; i != end_index; i += step)
		{
			uint8_t bit = STEP_MATRIX[i][0];
			gpio_set_level(STEP_DRIVER_PIN_IN1, bit);
			bit = STEP_MATRIX[i][1];
			gpio_set_level(STEP_DRIVER_PIN_IN2, bit);
			bit = STEP_MATRIX[i][2];
			gpio_set_level(STEP_DRIVER_PIN_IN3, bit);
			bit = STEP_MATRIX[i][3];
			gpio_set_level(STEP_DRIVER_PIN_IN4, bit);

			vTaskDelay(STEP_DELAY / portTICK_PERIOD_MS);
		}
	}

	// Reset to default state
	gpio_set_level(STEP_DRIVER_PIN_IN1, 0);
	gpio_set_level(STEP_DRIVER_PIN_IN2, 0);
	gpio_set_level(STEP_DRIVER_PIN_IN3, 0);
	gpio_set_level(STEP_DRIVER_PIN_IN4, 0);

	vTaskDelay(STEP_DELAY / portTICK_PERIOD_MS);
}

void step_motor_angle(int8_t angle)
{
	step_motor_step(STEP_FULL_ROTATION * angle / 360);
}
