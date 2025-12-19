"""
This file handles all sending/reciving of data between the communication module and the sensor/control modules via I2C. 
"""


from enum import Enum
import smbus3 as smbus
import struct

# Docs
# https://www.electronicwings.com/raspberry-pi/python-based-i2c-functions-for-raspberry-pi

# example values
SENSOR_MODULE = 0x10
CONTROL_MODULE = 0x20

LED_COUNT = 11  # Adjust based on your actual LED count on each line sensor

# CONTROL MODULE OPCODES:
CMD_PICKUP_ITEM_LEFT = 0x0A
CMD_PICKUP_ITEM_RIGHT = 0x0B
CMD_TURN_LEFT = 0x0C
CMD_TURN_RIGHT = 0x0D
CMD_DRIVE_FORWARD = 0x0E
CMD_DRIVE_BACKWARD = 0x0F
CMD_STOP = 0x10
CMD_SET_DRIVE_SPEED = 0x11
CMD_SET_TURN_SPEED = 0x12
CMD_UPDATE_STEER_SIGNAL = 0x13
CMD_DROP_OFF_ITEMS = 0x14

# SENSOR MODULE OPCODES:
CMD_ULTRASOUND = 0x04
CMD_SENSOR_TEST = 0x05
CMD_GYRO_STATE = 0x06
CMD_GYRO_RESET = 0x07
CMD_GYRO_RAW = 0x08
CMD_GYRO_ANGLE_RESET = 0x0A
CMD_GYRO_ANGLE = 0x0B
CMD_READ_TAPE_SENSOR_FRONT  = 0x0C
CMD_READ_TAPE_SENSOR_BACK   = 0x0D
CMD_ENABLE_TAPE_SENSOR = 0x0E
CMD_ENABLE_GYROS = 0x0F
CMD_ENABLE_ULTRASOUND = 0x10
CMD_DISABLE_TAPE_SENSOR = 0x11
CMD_DISABLE_GYROS = 0x12
CMD_DISABLE_ULTRASOUND = 0x13
CMD_DROP_OFF_ITEMS = 0x14
CMD_TURN_ARM = 0x15
CMD_TURN_BASE_AXLE = 0x16
CMD_TURN_MIDDLE_AXLE = 0x17
CMD_TURN_TOP_AXLE = 0x18
CMD_TURN_CLAW = 0x19
CMD_OPEN_CLAW = 0x1A
CMD_CLOSE_CLAW = 0x1B
CMD_START_PICKUP_LEFT = 0x1C
CMD_START_PICKUP_RIGHT = 0x1D
CMD_END_PICKUP = 0x1E


#HERE STARTS THE FUNCTIONS RELATED TO THE CONTROL MODULE
def i2c_set_drive_speed(speed: int):
    # automatically closes the bus when done
    with smbus.SMBus(1) as bus:  # 1 = /dev/i2c-1 (port I2C1)
        bus.write_byte_data(CONTROL_MODULE, CMD_SET_DRIVE_SPEED, speed)

def i2c_set_turn_speed(speed: int):
    # automatically closes the bus when done
    with smbus.SMBus(1) as bus:  # 1 = /dev/i2c-1 (port I2C1)
        bus.write_byte_data(CONTROL_MODULE, CMD_SET_TURN_SPEED, speed)


def i2c_stop():
    # automatically closes the bus when done
    with smbus.SMBus(1) as bus:  # 1 = /dev/i2c-1 (port I2C1)
        bus.write_byte_data(CONTROL_MODULE, CMD_STOP, 0x01)  # sends value 1

# direction: 0 = forward, 1 = backward
def i2c_move(direction: int): 
    # automatically closes the bus when done
    with smbus.SMBus(1) as bus:  # 1 = /dev/i2c-1 (port I2C1)
        if direction == 0:
            bus.write_byte_data(CONTROL_MODULE, CMD_DRIVE_FORWARD, 0x01)
        elif direction == 1:
            bus.write_byte_data(CONTROL_MODULE, CMD_DRIVE_BACKWARD, 0x01)
    return

# direction: 0 = left, 1 = right
def i2c_turn(direction: int):
    # automatically closes the bus when done
    with smbus.SMBus(1) as bus:  # 1 = /dev/i2c-1 (port I2C1)
        if direction == 0:
            bus.write_byte_data(CONTROL_MODULE, CMD_TURN_LEFT, 0x01)
        elif direction == 1:
            bus.write_byte_data(CONTROL_MODULE, CMD_TURN_RIGHT, 0x01)
    return

# direction: 0 = left, 1 = right
def i2c_pickup(direction: int):
    # automatically closes the bus when done
    with smbus.SMBus(1) as bus:  # 1 = /dev/i2c-1 (port I2C1)
        if direction == 0:
            bus.write_byte_data(CONTROL_MODULE, CMD_PICKUP_ITEM_LEFT, 0x01)
        elif direction == 1:
            bus.write_byte_data(CONTROL_MODULE, CMD_PICKUP_ITEM_RIGHT, 0x01)
    return

def i2c_dropoff():
    # automatically closes the bus when done
    with smbus.SMBus(1) as bus:  # 1 = /dev/i2c-1 (port I2C1)
        bus.write_byte_data(CONTROL_MODULE, CMD_DROP_OFF_ITEMS, 0x01)
    return

def i2c_reset_gyro():
    # automatically closes the bus when done
    with smbus.SMBus(1) as bus:  # 1 = /dev/i2c-1 (port I2C1)
        bus.write_byte_data(SENSOR_MODULE, CMD_GYRO_RESET, 0)

# Returns the current gyro angle, around 504 units = 90 degrees
def i2c_gyro_angle():
    """
    Read gyro angle as signed 16-bit integer.
    Range: -32768 to +32767 units
    """
    with smbus.SMBus(1) as bus:
        angle_unsigned = bus.read_word_data(SENSOR_MODULE, CMD_GYRO_ANGLE)
    
    # Convert unsigned to signed
    if angle_unsigned > 32767:
        angle_signed = angle_unsigned - 65536
    else:
        angle_signed = angle_unsigned
    
    return angle_signed

def i2c_update_steer_signal(signal: int):
    # automatically closes the bus when done
    with smbus.SMBus(1) as bus:  # 1 = /dev/i2c-1 (port I2C1)
        signal_bytes = struct.pack('<f', signal)
        bus.write_i2c_block_data(CONTROL_MODULE, CMD_UPDATE_STEER_SIGNAL, list(signal_bytes))

#HERE STARTS THE FUNCTIONS RELATED TO THE SENSOR MODULE
def i2c_sensor_test():
    with smbus.SMBus(1) as bus:  # 1 = /dev/i2c-1 (port I2C1)
        test_data = bus.read_i2c_block_data(SENSOR_MODULE, CMD_SENSOR_TEST, 4)
    return test_data

# Returns distance in cm
def i2c_ultrasound():
    with smbus.SMBus(1) as bus:  # 1 = /dev/i2c-1 (port I2C1)
        distance = bus.read_word_data(SENSOR_MODULE, CMD_ULTRASOUND)
    return distance

# Returns the state of the gyro sensor
def i2c_gyro_state():
    with smbus.SMBus(1) as bus:  # 1 = /dev/i2c-1 (port I2C1)
        state = bus.read_word_data(SENSOR_MODULE, CMD_GYRO_STATE)
    return state

# Returns raw gyro data
def i2c_gyro_raw():
    with smbus.SMBus(1) as bus:  # 1 = /dev/i2c-1 (port I2C1)
        raw_data = bus.read_word_data(SENSOR_MODULE, CMD_GYRO_RAW)
    return raw_data

def i2c_turn_arm_rotation_axle(diff_angle):
    with smbus.SMBus(1) as bus:
        bus.write_byte_data(CONTROL_MODULE, CMD_TURN_ARM, diff_angle)
    return

def i2c_turn_top_axle(diff_angle):
    with smbus.SMBus(1) as bus:
        bus.write_byte_data(CONTROL_MODULE, CMD_TURN_TOP_AXLE, diff_angle)
    return

def i2c_turn_middle_axle(diff_angle):
    with smbus.SMBus(1) as bus:
        bus.write_byte_data(CONTROL_MODULE, CMD_TURN_MIDDLE_AXLE, diff_angle)
    return

def i2c_turn_base_axle(diff_angle):
    with smbus.SMBus(1) as bus:
        bus.write_byte_data(CONTROL_MODULE, CMD_TURN_BASE_AXLE, diff_angle)
    return

def i2c_claw_rotation_axle(diff_angle):
    with smbus.SMBus(1) as bus:
        bus.write_byte_data(CONTROL_MODULE, CMD_TURN_CLAW, diff_angle)
    return

def i2c_claw_close():
    with smbus.SMBus(1) as bus:
        bus.write_byte_data(CONTROL_MODULE, CMD_CLOSE_CLAW, 0)
    return

def i2c_claw_open():
    with smbus.SMBus(1) as bus:
        bus.write_byte_data(CONTROL_MODULE, CMD_OPEN_CLAW, 0)
    return

def i2c_drop_off_items():
    with smbus.SMBus(1) as bus:
        bus.write_byte_data(CONTROL_MODULE, CMD_DROP_OFF_ITEMS, 0)
    return

def i2c_start_pickup_left():
    with smbus.SMBus(1) as bus:
        bus.write_byte_data(CONTROL_MODULE, CMD_START_PICKUP_LEFT, 0)
    return

def i2c_start_pickup_right():
    with smbus.SMBus(1) as bus:
        bus.write_byte_data(CONTROL_MODULE, CMD_START_PICKUP_RIGHT, 0)
    return

def i2c_end_pickup():
    with smbus.SMBus(1) as bus:
        bus.write_byte_data(CONTROL_MODULE, CMD_END_PICKUP, 0)
    return

def format_data(data):
    formatted_data = []
    for i in range(LED_COUNT):
        low_byte = data[2 * i]
        high_byte = data[2 * i + 1]
        word = (high_byte << 8) | low_byte
        formatted_data.append(word)
    formatted_data.reverse()
    return formatted_data

# Returns tape sensor data as a dictionary with front and rear sensor lists
def i2c_tape_sensor():
    with smbus.SMBus(1) as bus:  # 1 = /dev/i2c-1 (port I2C1)
        back_raw = bus.read_i2c_block_data(SENSOR_MODULE, CMD_READ_TAPE_SENSOR_BACK, 22)
        front_raw = bus.read_i2c_block_data(SENSOR_MODULE, CMD_READ_TAPE_SENSOR_FRONT, 22)
        
        front = format_data(front_raw)
        back = format_data(back_raw)
        tape_sensor_data = {"front_sensor": front, "rear_sensor": back}

    return tape_sensor_data

class SensorKind(Enum):
    GYROS = 0
    ULTRASOUND = 1
    TAPE_SENSOR = 2

def i2c_toggle_sensor(sensor: SensorKind, enabled: bool):
    with smbus.SMBus(1) as bus:  # 1 = /dev/i2c-1 (port I2C1)
        match sensor:
            case SensorKind.GYROS: command = CMD_ENABLE_GYROS if enabled else CMD_DISABLE_GYROS
            case SensorKind.ULTRASOUND: command = CMD_ENABLE_ULTRASOUND if enabled else CMD_DISABLE_ULTRASOUND
            case SensorKind.TAPE_SENSOR: command = CMD_ENABLE_TAPE_SENSOR if enabled else CMD_DISABLE_TAPE_SENSOR
        
        bus.write_byte_data(SENSOR_MODULE, command, 0x01)
