/*
 * ultrasoundsensor.h
 *
 * Created: 2025-11-06 11:05:20
 *  Author: ferpe211
 */ 


#ifndef ULTRASOUNDSENSOR_H_
#define ULTRASOUNDSENSOR_H_

#include <avr/io.h>
#include <stdint.h>

/*
 * ultrasoundsensor.h
 * 
 *
 * Connections:
 *   - Trigger pin: PD3 (output)
 *   - Echo pin:    PD2 (input)
 *
 * Constants:
 *   - DIVISION_CONSTANT = 34 (convert pulse time to distance in cm)
 *
 * Author: ferpe211
 * Created: 2025-11-05
 */

#define DIVISION_CONSTANT 34


/**
 * @brief Initializes the ultrasonic sensor pins and timers.
 * 
 * Configures PB3 as output (trigger) and PB2 as input (echo).
 * Call once at startup before using the sensor.
 */
void ultraSoundSensorInit(void);

/**
 * @brief Measures the distance to an obstacle in centimeters.
 *
 * @return Distance in centimeters (uint16_t).
 */
uint16_t getObstacleDistance(void);

/**
 * @brief Measures the echo pulse duration using Timer1.
 *
 * This function waits for the echo signal to go HIGH and LOW,
 * measures the duration in microseconds, and returns the result.
 *
 * @return Pulse duration in microseconds (uint16_t).
 */
uint16_t getPulseTime(void);



#endif /* ULTRASOUNDSENSOR_H_ */
