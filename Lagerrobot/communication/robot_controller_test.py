import robot_controller

# Testing  
if __name__ == '__main__':
    # Test expectation
    robot = robot_controller.RobotController()
    
    # robot.startAuto([(3,5), (3,3), (9,9)], (3,7))
    # robot.startAuto([(3,7)], (3,7))  # Expect pickUp
    # robot.startAuto([(3,5)], (3,7))  # Expect straight
    robot.startAuto([(4,7)], (3,7))  # Expect turn

    robot.set_left_sensor_activated(True)
    robot.startMonitoring()
    # robot_controller.time.
    # robot.set_left_sensor_activated(False)
    # robot.set_left_sensor_activated(True)
    # robot.set_left_sensor_activated(False)
    # robot.set_left_sensor_activated(True)
    # robot.set_left_sensor_activated(False)
    # robot.set_left_sensor_activated(True)
    # robot.set_left_sensor_activated(False)
    # robot.set_left_sensor_activated(True)
    robot.stopMonitoring()