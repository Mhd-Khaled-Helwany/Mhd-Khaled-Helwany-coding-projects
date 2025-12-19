"""
Used by robot_controller to decide the next course of action for the robot. e.g "drive forward", "pickup", "turning". This file
then initiates the action by calling robot_controller
"""

import time
from nav_algorithm import findPath, remove_position
from i2c_communicator import i2c_tape_sensor
from enum import Enum

START_POSITION = (11, 12) # Coordinate where the robot starts with it's task

class AutonomousState(Enum):
    RESTING = "resting"
    LINE_FOLLOWING = "line_following"
    TURNING = "turning"
    PICKING_UP = "picking_up"

class autonomous:
    i: int # Current index of "path" list
    items: any
    path: any
    robot: any
    state: AutonomousState
    startPos: tuple
    hasEnteredFactory: bool
    currentCoord : tuple

    def __init__(self, robot):
        self.i = 0
        self.robot = robot
        self.state = AutonomousState.RESTING
        self.startPos = START_POSITION
        self.items = []
        self.hasEnteredFactory = False
        self.currentCoord = self.startPos

    def startAuto(self):
        self.path = findPath(self.items, self.startPos, 1)
        self.i = 0
        print(self.path)
        self.hasEnteredFactory = False
        self.robot.set_angle_parameter(300)
        self.robot.set_proportional_parameter(40)
        self.currentCoord = self.startPos

        self.action()
        return
    

    def getCurrentDir(self):
        if self.i == 0:
            return 1  # Default starting direction
        else:
            coords = self.path[0]
            prev = coords[self.i - 1]
            curr = coords[self.i]

            dx = curr[0] - prev[0]
            dy = curr[1] - prev[1]

            if dx == 1:
                return 2  # Right
            elif dx == -1:
                return -2  # Left
            elif dy == 1:
                return -1  # Down
            elif dy == -1:
                return 1  # Up

    def handle_obstacle(self):
        currentCoord = self.path[0][self.i]
        remove_position(currentCoord)
        currentDir = self.getCurrentDir()
        currentCoord = self.path[0][self.i-1]

        self.path = findPath(self.items, currentCoord, currentDir)
        for step in self.path:
            print(step)
        self.i = 0
        self.action()        
        return
        
    def action(self):
        print("state", self.state)

       

        if self.state != AutonomousState.LINE_FOLLOWING:
            return

        # Execute navigation decision at crossing
        coords = self.path[0]
        directions = self.path[1]

        # Check if next location is a pickup point
        is_pickup_location = False
        pickup_direction = 0
        turn_direction = 0

        current_coord = coords[self.i]
        self.currentCoord = current_coord
        print(self.i)
        print(len(coords))

        # Stops the robot if we have finished the task (i.e reached end of the list)
        if(self.hasEnteredFactory == False):
            if(current_coord == (11,11)):
                #update steer params when we have entered factory
                self.robot.set_angle_parameter(300)
                self.robot.set_proportional_parameter(40)
                self.hasEnteredFactory = True
        else:
            if(current_coord == (11,11)):
                #update steer params when we have left factory
                self.robot.set_angle_parameter(300)
                self.robot.set_proportional_parameter(40)
                self.hasEnteredFactory = False

        #Robot kills itself...
        if(self.i+1 >= len(coords)):
            self.robot.stopAuto()
            return
        if(self.i+2 == len(coords)):
            print("LAST LAP!")
            self.robot.searchCrossings = False
            self.robot.endSeq = True

        # Checks if we have found a pickup station
        if current_coord in self.items:
            is_pickup_location = True
            pickup_direction = 0 if self.robot.get_left_sensor_activated() else 1
            print(current_coord)
        else:
            turn_direction = directions[self.i] if self.i < len(directions) else 1
            print(current_coord)

        # Execute action
        if is_pickup_location:
            # Execute pickup, save turn for later
            self.items.remove(current_coord)
            print("pickup")
            self.robot.stop()
            print("robot left", self.robot.get_left_sensor_activated())
            print("robot right", self.robot.get_right_sensor_activated())
            self.robot.pickup(pickup_direction)
            self.state = AutonomousState.PICKING_UP
            print(f"Starting pickup: direction={pickup_direction}")
        else:
            # Execute turn
            if turn_direction == 1:
                print("straight")
                self.robot.move()
                self.state = AutonomousState.LINE_FOLLOWING
            else:
                print("turn", turn_direction)
                # Start turn
                self.robot.stop()
                self.robot.gyro_reset_tick = 5
                self.robot.reset_gyro() # Sets reference angle to 0 before turn 
                self.target_angle = 90 if turn_direction==-2 or turn_direction==2 else 180 
                turnDir = 0 if turn_direction == -2 else 1 

                self.robot.turn(turnDir)
                self.state = AutonomousState.TURNING
                print(f"Starting turn: target={self.target_angle}°")

        # Advance path
        if self.i + 1 < len(coords):
            self.i += 1