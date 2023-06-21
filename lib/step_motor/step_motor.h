#ifndef _STEP_MOTOR_H_
#define _STEP_MOTOR_H_

#include <driver/gpio.h>

/**
 * This driver is meant for a 4-wire stepper motor driver (ULN2003) connected to a 28BYJ-48 stepper motor.
 *
 * Supports forward and backward rotations.
 */

#define STEP_DRIVER_PIN_IN1 GPIO_NUM_14
#define STEP_DRIVER_PIN_IN2 GPIO_NUM_13
#define STEP_DRIVER_PIN_IN3 GPIO_NUM_12
#define STEP_DRIVER_PIN_IN4 GPIO_NUM_11

#define STEP_FULL_ROTATION 511 //!< Approximate number of steps for a full rotation.
#define STEP_DELAY 15					 //!< Delay between steps in ms.

/**
 * Initialize the step motor driver.
 *
 * @return Always returns ESP_OK.
 */
extern esp_err_t step_motor_init();

/**
 * Rotate the motor by a number of steps.
 *
 * @param steps Number of steps to rotate.
 *
 * @note If steps is negative, the motor will rotate backwards.
 * @note A full rotation is set in the `STEP_FULL_ROTATION` macro.
 */
extern void step_motor_step(int steps);

/**
 * Rotate the motor forward.
 *
 * @param angle Angle to rotate in degrees.
 *
 * @note If steps is negative, the motor will rotate backwards.
 */
extern void step_motor_angle(int8_t angle);

#endif
