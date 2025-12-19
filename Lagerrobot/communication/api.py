"""
This file uses flask to create the API endpoints and handle http traffic
"""


from flask import Flask, jsonify, request
import threading
from i2c_communicator import *
from robot_controller import RobotController

app = Flask(__name__)
robot: RobotController = None


def set_robot(robot_instance):
    """Set the robot instance for API endpoints to use"""
    global robot
    robot = robot_instance


@app.route("/move", methods=["POST"])
def move():
    """API endpoint to move the robot forward or backward"""
    data = request.json
    direction = data.get("direction", 0)  # 0 = forward, 1 = backward
    robot.move(direction)
    return jsonify({"success": True})


@app.route("/turn", methods=["POST"])
def turn():
    """API endpoint to turn the robot left or right"""
    data = request.json
    direction = data.get("direction", 0)  # 0 = left, 1 = right
    robot.turn(direction)
    return jsonify({"success": True})


@app.route("/pickup", methods=["POST"])
def pickup():
    """API endpoint to pick up an item left or right"""
    data = request.json
    direction = data.get("direction", 0)  # 0 = left, 1 = right
    robot.pickup(direction)
    return jsonify({"success": True})


@app.route("/dropoff", methods=["POST"])
def dropoff():
    """API endpoint to drop off items"""
    i2c_drop_off_items()
    return jsonify({"success": True})


@app.route("/stop", methods=["POST"])
def stop():
    """API endpoint to stop the robot"""
    robot.stop()
    return jsonify({"success": True})


@app.route("/setDriveSpeed", methods=["POST"])
def set_drive_speed():
    """API endpoint to set the drive speed"""
    data = request.json
    speed = data.get("speed", 100)  # default speed
    robot.set_drive_speed(speed)
    return jsonify({"success": True})


@app.route("/setTurnSpeed", methods=["POST"])
def set_turn_speed():
    """API endpoint to set the turn speed"""
    data = request.json
    speed = data.get("speed", 100)  # default speed
    robot.set_turn_speed(speed)
    return jsonify({"success": True})


@app.route("/getTurnSpeed", methods=["GET"])
def get_turn_speed():
    """API endpoint to set the turn speed"""
    speed = robot.get_turn_speed()
    return jsonify({"speed": speed})


@app.route("/getDriveSpeed", methods=["GET"])
def get_drive_speed():
    """API endpoint to set the drive speed"""
    speed = robot.get_drive_speed()
    return jsonify({"speed": speed})


@app.route("/setSpeed", methods=["POST"])
def set_speed():
    """API endpoint to set turn and drive speed"""
    data = request.json
    turn_speed = data.get(
        "turnSpeed",
    )
    drive_speed = data.get(
        "driveSpeed",
    )
    robot.set_turn_speed(turn_speed)
    robot.set_drive_speed(drive_speed)
    return jsonify({"success": True})


@app.route("/updateControl", methods=["POST"])
def update_control():
    data = request.json

    if not "proportional" in data or not "angle" in data:
        return jsonify({"success": False, "message": "No parameters to update"}), 400

    proportional = data.get("proportional")
    angle = data.get("angle")

    robot.set_angle_parameter(angle)
    robot.set_proportional_parameter(proportional)
    return jsonify({"success": True})


@app.route("/setAngleParameter", methods=["PUT"])
def set_angle_parameter():
    data = request.json
    if data.get("angle_parameter") == robot.angle_parameter:
        print("WARNING: Angle parameter was set to the previous value")
    angle_parameter = data.get("angle_parameter", robot.angle_parameter)
    robot.set_proportional_parameter(angle_parameter)


@app.route("/getProportionalParameter", methods=["GET"])
def get_proportional_parameter():
    return jsonify(robot.get_proportional_parameter())


@app.route("/getAngleParameter", methods=["GET"])
def get_angle_parameter():
    return jsonify(robot.get_angle_parameter())


@app.route("/test", methods=["GET"])
def test():
    """Test API endpoint"""
    message = "Hello, world!"
    return jsonify(message)


@app.route("/getMode", methods=["GET"])
def get_mode():
    """API endpoint to get current mode"""
    mode = robot.get_mode()
    return jsonify({"mode": mode})


@app.route("/switchAuto", methods=["POST"])
def switchAuto():
    """API endpoint to change mode"""
    # Switch between manual and autonomous mode
    robot.set_mode("auto" if robot.mode == "manual" else "manual")
    if robot.mode == "auto":
        robot.startAuto()
    if (robot.mode == "manual"):
        robot.stop()
    return jsonify({"success": True, "new_mode": robot.mode})


@app.route("/setupCargo", methods=["POST"])
def add_cargo():
    """API endpoint to add cargo locations"""
    data = request.json
    pick_up = data.get("pickUp", [])
    pick_up = [tuple(item) for item in pick_up]
    startPos = data.get("pickUp", ())

    col = data.get("y", 0)
    row = data.get("x", 0)
    if data.get("hasCargo", False) == True:
        robot.addCargo(row, col)
    else:
        robot.deleteCargo(row, col)

    return jsonify({"success": True})


@app.route("/getAutoStatus", methods=["GET"])
def get_auto_status():
    """API endpoint to get autonomous mode status"""
    status = str(robot.get_auto_status().value)
    return jsonify({"auto_status": status})


# Sensor endpoints
@app.route("/getSensorTest", methods=["GET"])
def get_sensor_test():
    """API endpoint to get sensor test data"""
    test_data = robot.get_sensor_test()
    return jsonify({"test_data": test_data})


@app.route("/getUltrasound", methods=["GET"])
def get_ultrasound():
    """API endpoint to get ultrasound distance"""
    distance = robot.get_ultrasound()
    return jsonify({"distance_cm": distance})


@app.route("/getGyroAngle", methods=["GET"])
def get_gyro_angle():
    """API endpoint to get gyro angle"""
    angle = robot.get_gyro_angle()
    return jsonify({"angle": angle})


@app.route("/getRawGyro", methods=["GET"])
def get_raw_gyro():
    """API endpoint to get raw gyro data"""
    raw_gyro = robot.get_raw_gyro()
    return jsonify(raw_gyro)


@app.route("/resetGyro", methods=["POST"])
def reset_gyro():
    """API endpoint to reset gyro angle"""
    robot.reset_gyro_angle()
    return jsonify({"success": True})


@app.route("/getLineSensor", methods=["GET"])
def get_line_sensor():
    """API endpoint to get line sensor data"""
    line_sensor_data = robot.get_raw_line_sensor()
    return jsonify(line_sensor_data)

@app.route("/turnArmRotation", methods=["POST"])
def turn_arm_rotation():
    diff_angle = request.json.get("diff_angle", 0)
    i2c_turn_arm_rotation_axle(diff_angle)
    return jsonify({"success": True})

@app.route("/turnTopAxle", methods=["POST"])
def turn_top_axle():
    diff_angle = request.json.get("diff_angle", 0)
    i2c_turn_top_axle(diff_angle)
    return jsonify({"success": True})

@app.route("/turnMiddleAxle", methods=["POST"])
def turn_middle_axle():
    diff_angle = request.json.get("diff_angle", 0)
    i2c_turn_middle_axle(diff_angle)
    return jsonify({"success": True})

@app.route("/turnBaseAxle", methods=["POST"])
def turn_base_axle():
    diff_angle = request.json.get("diff_angle", 0)
    i2c_turn_base_axle(diff_angle)
    return jsonify({"success": True})

@app.route("/clawRotationAxle", methods=["POST"])
def turn_claw_rotation_axle():
    diff_angle = request.json.get("diff_angle", 0)
    i2c_claw_rotation_axle(diff_angle)
    return jsonify({"success": True})

@app.route("/closeClaw", methods=["POST"])
def close_claw():
    i2c_claw_close()
    return jsonify({"success": True})

@app.route("/openClaw", methods=["POST"])
def open_claw():
    i2c_claw_open()
    return jsonify({"success": True})

@app.route("/startPickup", methods=["POST"])
def start_pickup():
    direction = request.json.get("direction", 0)
    if direction == "left":
        i2c_start_pickup_left()
    elif direction == "right":
        i2c_start_pickup_right()

    return jsonify({"success": True})

@app.route("/endPickup", methods=["POST"])
def end_pickup():
    robot.endPickup()
    return jsonify({"success": True})

@app.route("/getCurrentCoord", methods=["GET"])
def get_current_coord():
    coord = robot.auto.currentCoord
    return jsonify({"current_coord": coord})

@app.route("/getAutoPositioning", methods=["GET"])
def get_auto_positioning():
    current_position = robot.auto.currentCoord
    in_factory = robot.auto.hasEnteredFactory

    if robot.auto.items:
        destination_position = robot.auto.items[0]
    else:
        destination_position = None
    
    return jsonify({
        "current_position": current_position,
        "in_factory": in_factory,
        "destination_position": destination_position,
    })

@app.route("/getSteerSignal", methods=["GET"])
def get_steer_signal():
    steer_signal = robot.steer_signal
    return jsonify(steer_signal)
