""""
This file is run on the Raspberry Pi on startup. It kickstarts the backend by creating a RobotController object and calling it.  
It also initializes the web server.
"""


from robot_controller import RobotController
from api import app, set_robot
from flask_cors import CORS
import argparse
import logging

def main(args):
    """Main entry point"""

    # NOTE: Uncomment the below line enable INFO-logging from the robot's logger.
    # app.logger.setLevel(logging.INFO)

    robot = RobotController(logger=app.logger, debug=args.debug)

    robot.startMonitoring()
    set_robot(robot)

    log = logging.getLogger('werkzeug')
    log.setLevel(logging.ERROR)
    CORS(app)
    app.run(host="0.0.0.0", port=5001, debug=False)    


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog="communication",
        description="Handles all communication of the program",
    )

    parser.add_argument("-d", "--debug", action="store_true")

    args = parser.parse_args()
    main(args)
