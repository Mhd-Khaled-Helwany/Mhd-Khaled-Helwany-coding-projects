"""
This file handles the control algorithm and the logic for when the left/right side of the linesensor is "active".
The control algorithm works exactly as describes in the TekDok, 
the sensor logic checks if a specified number of diodes gives values over a certain threshold
"""

import math

IDEAL_GRAVITY = 6
DIODE_DISTANCE = 10.16  # Distance between each diode in millimeter
SENSOR_DISTANCE = 69.2  # Distance between rows of diodes in millimeter
LINE_THRESHOLD = 200
SENSOR_REQ = 3


def calculate_gravity(sensor_data):
    weighted_sum = sum([sensor_data[k] * (k + 1) for k in range(len(sensor_data))])
    center_of_gravity = weighted_sum / sum(sensor_data)
    return center_of_gravity


def control_algorithm(both_sensor_data, proportional_parameter, angle_parameter):
    front_gravity = calculate_gravity(both_sensor_data["front_sensor"])
    rear_gravity = calculate_gravity(both_sensor_data["rear_sensor"])
    error = front_gravity - IDEAL_GRAVITY

    angle = math.atan(
        (front_gravity - rear_gravity) * (DIODE_DISTANCE / SENSOR_DISTANCE)
    )

    signal = proportional_parameter * error + angle_parameter * angle

    return signal


def set_active_sides(sensor_data):
    left_sensor_data = sensor_data[:5]
    right_sensor_data = sensor_data[-5:]

    left_sensors_high = 0
    for sensor in left_sensor_data:
        if sensor > LINE_THRESHOLD:
            left_sensors_high +=1

    right_sensors_high = 0
    for sensor in right_sensor_data:
            if sensor > LINE_THRESHOLD:
                right_sensors_high +=1

    left_sensor_active = left_sensors_high >= SENSOR_REQ
    right_sensor_active = right_sensors_high >= SENSOR_REQ

    return (left_sensor_active, right_sensor_active)
