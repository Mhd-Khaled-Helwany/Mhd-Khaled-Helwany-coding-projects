"""
This file is used to handle controll the robot depending on what we are trying to do. This is done by continously 
running the cycle function which uses internal logic as well as functionallity of other files
to: Read data from the sensor module, interpret the data to determine the correct action, 
send commands to the control module that executes the action.
"""


import threading
import time
from i2c_communicator import *
from flask import jsonify
from data_manipulation import control_algorithm, set_active_sides
from autonomous import autonomous, AutonomousState
import logging

UPDATE_FREQ = 0.0001 # Delay between each cycle
TURN_TIMEOUT = 0.4  # How long we ignore looking for crossing (in seconds)
PICKUP_DURATION = 10.0  # Duration to wait after pickup command (in seconds)

class RobotController:
    position: tuple[int, int]  # x, y
    mode: str  # Possible modes are auto or manual
    speed: int 
    sensor_data: any
    direction: int  # -2 left, 1 straight, 2 right
    proportional_parameter: int
    angle_parameter: int
    raw_line_sensor_data: any
    rear_left_sensor_active: bool
    rear_right_sensor_active: bool
    front_left_sensor_active: bool
    front_right_sensor_active: bool
    debug: bool
    auto: autonomous
    pickup_timer: float
    turn_timer: float 
    monitoring_thread: threading.Thread
    monitoring_running: bool = False
    steer_signal: any
    endedPickup: bool # Flag to indicate if pickup has ended§
    endSeq: bool # Flag to indicate end sequence
    drive_speed: any
    turn_speed: any
    gyro_reset_tick: int


    def __init__(self, logger, debug=False):
        self.auto = autonomous(self)
        self.destinations = list[tuple[int, int]]
        self.proportional_parameter = 40
        self.angle_parameter = 300
        self.raw_line_sensor_data = 0
        self.steer_signal = 0
        self.left_activated = False
        self.right_activated = False
        self.debug = debug
        self.mode = "manual"
        self.turn_timer = time.time()

        self.turn_speed = 250
        self.drive_speed = 60
        i2c_set_turn_speed(self.turn_speed)
        i2c_set_drive_speed(self.drive_speed)

        # Store previous sensor states for edge detection
        self.prev_rear_left_sensor_active = False
        self.prev_rear_right_sensor_active = False

        self.prev_front_left_sensor_active = False
        self.prev_front_right_sensor_active = False

        self.cycle_timer = time.perf_counter()
        self.cycle_i = 0
        self.searchCrossings = True
        self.left_sensor_crossing = False
        self.right_sensor_crossing = False
        self.pickup_timer = time.time()
        self.endedPickup = False
        self.endSeq = False
        self.logger = logger

        self.gyro_reset_tick = 0

    def set_steer_signal(self, steer_signal):
        self.steer_signal = steer_signal
        return
    
    def get_steer_signal(self):
        return self.steer_signal

    def set_proportional_parameter(self, proportional_parameter):
        self.proportional_parameter = proportional_parameter
        return

    def set_angle_parameter(self, angle_parameter):
        self.angle_parameter = angle_parameter
        return

    def get_proportional_parameter(self):
        return self.proportional_parameter

    def get_angle_parameter(self):
        return self.angle_parameter
    
    def get_raw_line_sensor(self):
        return self.raw_line_sensor_data

    def get_left_sensor_activated(self):
        return self.front_left_sensor_active

    def get_right_sensor_activated(self):
        return self.front_right_sensor_active
    
    def set_left_sensor_activated(self, state):
        self.rear_left_sensor_active = state
        return

    def set_right_sensor_activated(self, state):
        self.rear_right_sensor_active = state
        return

    def set_mode(self, mode: str):
        self.mode = mode
        return

    def get_mode(self):
        return self.mode
    
    def get_auto_status(self):
        return self.auto.state

    def set_drive_speed(self, speed: int):
        self.drive_speed = speed
        i2c_set_drive_speed(speed)
        return
    
    def set_turn_speed(self, speed: int):
        self.turn_speed = speed
        i2c_set_turn_speed(speed)
        return

    def get_drive_speed(self):
        return self.drive_speed
    
    def get_turn_speed(self):
        return self.turn_speed

    #direction 0 = forward, 1 = backward
    def move(self, direction: int = 0):
        i2c_move(direction)
        return
    
    def addCargo(self, row, col):
        self.auto.items.append((row, col))
        return
    
    def deleteCargo(self, row, col):
        if (row, col) in self.auto.items:
            self.auto.items.remove((row, col))
        return

    def turn(self, direction: int):
        i2c_turn(direction)
        return

    def pickup(self, direction: int):
        if direction == 0:
            i2c_start_pickup_left()
        else:
            i2c_start_pickup_right()
        return

    def stop(self):
        i2c_stop()
        return

    #Sensor functions
    def get_ultrasound(self):
        distance = i2c_ultrasound()
        return distance
    
    def get_gyro_angle(self):
        angle = i2c_gyro_angle()
        return angle
    
    def reset_gyro_angle(self):
        i2c_reset_gyro()
        return
    
    def get_raw_gyro(self):
        raw_data = i2c_gyro_raw()
        return raw_data

    def get_sensor_test(self):
        test_data = i2c_sensor_test()
        return test_data

    def getStatus(self):
        return jsonify(
            {
                "sensor_data": self.sensor_data,
                "control_data": self.control_data,
                "position": self.position,
                "speed": self.speed,
                "mode": self.mode,
                "timestamp": self.time,
            }
        )

    def startMonitoring(self):
        """Start the monitoring thread"""
        if not self.monitoring_running:
            self.monitoring_running = True
            self.monitoring_thread = threading.Thread(target=self._monitoring_loop, daemon=True)
            self.cycle_timer=time.perf_counter()

            self.monitoring_thread.start()

        return
    
    def stopMonitoring(self):
        """Stop the monitoring thread"""
        if self.monitoring_running:
            self.monitoring_running = False
            if self.monitoring_thread:
                self.monitoring_thread.join(timeout=2.0)
        return

    def _monitoring_loop(self):
        """Internal loop that runs cycle every UPDATE_FREQ seconds"""
        while self.monitoring_running:
            self.cycle()
            # Sleep for UPDATE_FREQ seconds (0.001s = 1ms)
            time.sleep(UPDATE_FREQ)

    def read_and_update_sensors(self):
        """Helper method to read sensors and update steer signal"""
        tape_sensor_data = i2c_tape_sensor()
        self.raw_line_sensor_data = tape_sensor_data
        control_signal = control_algorithm(
            tape_sensor_data, self.proportional_parameter, self.angle_parameter
        )
        self.set_steer_signal(control_signal)
        self.logger.info("Steer signal: %s", control_signal)

        current_rear_left_sensor_active, current_rear_right_sensor_active = set_active_sides(
            tape_sensor_data["rear_sensor"]
        )
        current_front_left_sensor_active, current_front_right_sensor_active = set_active_sides(
            tape_sensor_data["front_sensor"]
        )

        # Only update steer if not at crossing/pickup point
        if not (
            current_rear_left_sensor_active or
            current_rear_right_sensor_active or
            current_front_left_sensor_active or
            current_front_right_sensor_active
        ):
            i2c_update_steer_signal(control_signal)

        return tape_sensor_data

    def reset_gyro(self):
        i2c_reset_gyro()
        return
    
    def dropoff(self):
        i2c_dropoff()
        return
    
    def endPickup(self):
        self.pickup_timer = time.time()
        self.endedPickup = True
        i2c_end_pickup()
        return

    def startAuto(self):
        self.auto.state = AutonomousState.LINE_FOLLOWING
        self.searchCrossings = True
        self.endSeq = False
        self.set_turn_speed(200)
        self.auto.startAuto()

    

    def stopAuto(self):
        print("Stopping autonomous mode")
        self.stop()
        self.mode = "manual"
        self.auto.state = AutonomousState.RESTING
        self.dropoff()

    def obstacleFound(self):
        distance = self.get_ultrasound()
        print("Found obstacle with distance", distance)
        return distance < 15 and distance > 0


    def cycle(self):
        """Main control loop cycle"""
        # Autonomous mode
        if self.mode == "auto":
            import time

            # Early exit: Still in pickup duration
            if self.endedPickup:
                if time.time() - self.pickup_timer >= PICKUP_DURATION:
                    print("Pickup complete")
                    self.auto.state = AutonomousState.LINE_FOLLOWING
                    self.move()
                    self.endedPickup = False
                # When picking up we return so we don't proceed
                return

            # Early exit: Turn timeout still active
            isTimedOut = time.time() - self.turn_timer <= TURN_TIMEOUT
            if isTimedOut:
                return

            # Read sensors (only if not turning)
            if self.auto.state != AutonomousState.TURNING:
                tape_sensor_data = self.read_and_update_sensors()

            # State machine
            if self.auto.state == AutonomousState.TURNING:
                gyro_angle = i2c_gyro_angle()
                if(self.gyro_reset_tick > 0):
                    print("Waiting for gyro to reset...", self.gyro_reset_tick)
                    self.gyro_reset_tick -= 1
                    return
                
                print("angle", abs(gyro_angle))

                target_gyro_units = abs(self.auto.target_angle)
                tolerance = 3

                if abs(gyro_angle) >= target_gyro_units - tolerance:
                    self.auto.state = AutonomousState.LINE_FOLLOWING
                    print("turn done, now moving forward")
                    i2c_update_steer_signal(0)
                    if self.obstacleFound():
                        print("Obstacle detected, stopping")
                        self.stop()
                        self.auto.handle_obstacle()
                        return
                    import time

                    self.turn_timer = time.time()
                    self.move()

            elif self.auto.state == AutonomousState.PICKING_UP:
                # When picking up we return so we don't proceed
                return

            else:  # LINE_FOLLOWING and other states
                self.rear_left_sensor_active, self.rear_right_sensor_active = set_active_sides(
                    tape_sensor_data["rear_sensor"]
                )
                self.front_left_sensor_active, self.front_right_sensor_active = set_active_sides(
                    tape_sensor_data["front_sensor"]
                )

                #These are unused
                rear_left_rising_edge = self.rear_left_sensor_active and not self.prev_rear_left_sensor_active
                rear_right_rising_edge = self.rear_right_sensor_active and not self.prev_rear_right_sensor_active
                
                # Edge detection: only trigger on rising edge (False -> True transition)
                front_left_rising_edge = self.front_left_sensor_active and not self.prev_front_left_sensor_active
                front_right_rising_edge = self.front_right_sensor_active and not self.prev_front_right_sensor_active

                #These are unused
                front_left_falling_edge = not self.front_left_sensor_active and self.prev_front_left_sensor_active
                front_right_falling_edge = not self.front_right_sensor_active and self.prev_front_right_sensor_active

                # End sequence detection
                if self.endSeq:
                    if front_left_rising_edge or front_right_rising_edge:
                        self.stopAuto()
                        return

                # Crossing and pickup detection
                if self.auto.state != AutonomousState.TURNING:
                    if self.searchCrossings:
                        self.left_sensor_crossing = front_left_rising_edge or self.left_sensor_crossing
                        self.right_sensor_crossing = front_right_rising_edge or self.right_sensor_crossing

                        if self.left_sensor_crossing and self.right_sensor_crossing:
                            print("Found crossing")
                            self.searchCrossings = False
                            self.left_sensor_crossing = False
                            self.right_sensor_crossing = False
                            self.auto.action()
                            # We need to check if we started turning from latest action call, again...
                            if self.auto.state != AutonomousState.TURNING:
                                if self.obstacleFound():
                                    print("Obstacle detected, stopping")
                                    self.stop()
                                    self.auto.handle_obstacle()
                                    return
                    else:
                        if front_left_rising_edge ^ front_right_rising_edge:
                            print("Found pickup point")
                            self.searchCrossings = True
                            self.auto.action()

                # Update previous states for next cycle
                self.prev_rear_left_sensor_active = self.rear_left_sensor_active
                self.prev_rear_right_sensor_active = self.rear_right_sensor_active
                self.prev_front_left_sensor_active = self.front_left_sensor_active
                self.prev_front_right_sensor_active = self.front_right_sensor_active

        else:  # Manual mode
            self.read_and_update_sensors()
