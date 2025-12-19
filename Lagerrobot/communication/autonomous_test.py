from autonomous import autonomous
import robot_controller

# Testing  
if __name__ == '__main__':
    # Test expectation
    robot = robot_controller.RobotController()
    auto = autonomous(robot)
    auto.startAuto([(3,5), (3,3), (9,9)], (3,7))
    auto.awaitsignal()