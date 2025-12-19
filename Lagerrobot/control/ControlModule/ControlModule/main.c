#define ENABLE_BIT_DEFINITIONS
#include <math.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <string.h>

#define F_CPU 16000000UL
#include <util/delay.h>

#include "i2c.h"

typedef unsigned char byte;

#define PARSE_AS(ptr, type) (*(type *)(ptr))

#define BAUDRATE 1000000UL
#define MYUBRR ((F_CPU / (16UL * BAUDRATE)) - 1)

// --- Control Table Address ---
// EEPROM AREA
#define P_MODEL_NUMBER_L 0
#define P_MODOEL_NUMBER_H 1
#define P_VERSION 2
#define P_ID 3
#define P_BAUD_RATE 4
#define P_RETURN_DELAY_TIME 5
#define P_CW_ANGLE_LIMIT_L 6
#define P_CW_ANGLE_LIMIT_H 7
#define P_CCW_ANGLE_LIMIT_L 8
#define P_CCW_ANGLE_LIMIT_H 9
#define P_SYSTEM_DATA2 10
#define P_LIMIT_TEMPERATURE 11
#define P_DOWN_LIMIT_VOLTAGE 12
#define P_UP_LIMIT_VOLTAGE 13
#define P_MAX_TORQUE_L 14
#define P_MAX_TORQUE_H 15
#define P_RETURN_LEVEL 16
#define P_ALARM_LED 17
#define P_ALARM_SHUTDOWN 18
#define P_OPERATING_MODE 19
#define P_DOWN_CALIBRATION_L 20
#define P_DOWN_CALIBRATION_H 21
#define P_UP_CALIBRATION_L 22
#define P_UP_CALIBRATION_H 23
#define P_TORQUE_ENABLE (24)
#define P_LED (25)
#define P_CW_COMPLIANCE_MARGIN (26)
#define P_CCW_COMPLIANCE_MARGIN (27)
#define P_CW_COMPLIANCE_SLOPE (28)
#define P_CCW_COMPLIANCE_SLOPE (29)
#define P_GOAL_POSITION_L (30)
#define P_GOAL_POSITION_H (31)
#define P_GOAL_SPEED_L (32)
#define P_GOAL_SPEED_H (33)
#define P_TORQUE_LIMIT_L (34)
#define P_TORQUE_LIMIT_H (35)
#define P_PRESENT_POSITION_L (36)
#define P_PRESENT_POSITION_H (37)
#define P_PRESENT_SPEED_L (38)
#define P_PRESENT_SPEED_H (39)
#define P_PRESENT_LOAD_L (40)
#define P_PRESENT_LOAD_H (41)
#define P_PRESENT_VOLTAGE (42)
#define P_PRESENT_TEMPERATURE (43)
#define P_REGISTERED_INSTRUCTION (44)
#define P_PAUSE_TIME (45)
#define P_MOVING (46)
#define P_LOCK (47)
#define P_PUNCH_L (48)
#define P_PUNCH_H (49)

// --- Instruction ---
#define INST_PING 0x01
#define INST_READ 0x02
#define INST_WRITE 0x03
#define INST_REG_WRITE 0x04
#define INST_ACTION 0x05
// #define INST_RESET 0x06
#define INST_DIGITAL_RESET 0x07
#define INST_SYSTEM_READ 0x0C
#define INST_SYSTEM_WRITE 0x0D
#define INST_SYNC_WRITE 0x83
#define INST_SYNC_REG_WRITE 0x84

// --- Global constants
#define GLOBALSERVOSPEED 100 // Values between 0x000 and 0x3FF
#define CLAW_SPEED 0x088 // Values between 0x000 and 0x3FF

byte GlobalDriveSpeed = 60; // Values between 0x00 and 0xFE
byte GlobalSteeringSlowdown = 0; // Values between 0 and GlobalDriveSpeed
byte GlobalTurnSpeed = 150; // Values between 0x00 and 0xFE

char GlobalSteerSignalAmplitude = 0;
char GlobalSteerSignalPositive = 0;

short GlobalArmRotation = 0;
short GlobalTopRotation = 0;
short GlobalMiddleRotation = 0;
short GlobalBaseRotation = 0;
short GlobalClawRotation = 0;

#define MAX_PACKET_SIZE 40
byte packetBuffer[MAX_PACKET_SIZE]; // Somewhere to store packets

bool globalForward = false;

#define MOTOR1ROTATIONMIN 0
#define MOTOR1ROTATIONMAX 360

#define MOTOR2ROTATIONMIN 60
#define MOTOR2ROTATIONMAX 240

#define MOTOR3ROTATIONMIN 60
#define MOTOR3ROTATIONMAX 240

#define MOTOR4ROTATIONMIN 60
#define MOTOR4ROTATIONMAX 260

#define MOTOR5ROTATIONMIN 40
#define MOTOR5ROTATIONMAX 240

#define MOTOR6ROTATIONMIN 60
#define MOTOR6ROTATIONMAX 240

#define MOTOR7ROTATIONMIN 0
#define MOTOR7ROTATIONMAX 300

#define MOTOR8ROTATIONMIN 0
#define MOTOR8ROTATIONMAX 150

// Control Commands
#define CMD_PICKUP_ITEM_LEFT      0x0A
#define CMD_PICKUP_ITEM_RIGHT     0x0B
#define CMD_ROTATE_LEFT           0x0C
#define CMD_ROTATE_RIGHT          0x0D
#define CMD_DRIVE_FORWARD         0x0E
#define CMD_DRIVE_BACKWARD        0x0F
#define CMD_STOP                  0x10
#define CMD_SET_DRIVE_SPEED       0x11
#define CMD_SET_TURN_SPEED        0x12
#define CMD_UPDATE_STEER_SIGNAL   0x13
#define CMD_DROP_OFF_ITEMS        0x14
#define CMD_TURN_ARM              0x15
#define CMD_TURN_BASE_AXLE        0x16
#define CMD_TURN_MIDDLE_AXLE      0x17
#define CMD_TURN_TOP_AXLE         0x18
#define CMD_TURN_CLAW             0x19
#define CMD_OPEN_CLAW             0x1A
#define CMD_CLOSE_CLAW            0x1B
#define CMD_START_PICKUP_LEFT     0x1C
#define CMD_START_PICKUP_RIGHT    0x1D
#define CMD_END_PICKUP            0x1E

// Headers
void PortInitialize(void);
void HandleI2C(void);

// UART / Robot Arm
void USART_Init(void);
void USART_Transmit(byte data);
byte USART_Receive(void);
void DynamixelSetReceiveMode(void);
void DynamixelSetTransmitMode(void);

void CreateDynamixelPacket(byte *packet, byte id, byte instruction, byte *parameters, byte parameters_length);
void TransmitDynamixelPacket(byte *packet);

short DegreesToAngle(float degrees);

// NOTE: Speed and angle range from 0x0000 - 0x03FF if hex
void TurnSingle(byte id, short angle, short speed);
void TurnPairMirrored(byte id0, byte id1, short angle0, short speed);

void TurnArmRotationAxle(short angleInDegrees);
void TurnBaseAxle(short angle);
void TurnMiddleAxle(short angleInDegrees);
void TurnTopAxle(short angleInDegrees);
void TurnClawRotationAxle(short angleInDegrees);

void InitRobotArmServos(void);

void ClawOpen(void);
void ClawClose(void);

void ReturnToDefaultPosition(void);
void GrabObjectRight(void);
void GrabObjectLeft(void);
void DropObject(void);

void RobotArm_DropOfItems(void);
void RobotArm_PickupRight(void);
void RobotArm_PickupLeft(void);

void StartLeftPickup(void);
void StartRightPickup(void);
void EndPickupSequence(void);

// PWM / Wheel Motors
void PWM_Init(void);
void WheelMotors_Init(void);

void WheelMotors_Init(void);
void WheelMotors_DriveForwardWithSteering(void);
void WheelMotors_DriveForward(void);
void WheelMotors_DriveBackwards(void);
void WheelMotors_RotateClockwise(void);
void WheelMotors_RotateCounterClockwise(void);
void WheelMotors_Stop(void);

void UpdateSteerSignal(float signal);

int main(void)
{
	// Initialization
	PortInitialize();
	I2C_init(0x20);
	USART_Init();
	WheelMotors_Init();
	InitRobotArmServos();
	ReturnToDefaultPosition();
	
	
	//Testing

	//Main loop
	while (1)
	{
		HandleI2C();

		if (globalForward)
		{
			WheelMotors_DriveForwardWithSteering();
		}
	}
}

void HandleI2C(void)
{
	if (rx_done)
	{
		rx_done = 0;
		uint8_t command = rx_buffer[0];
		signed char diff = rx_buffer[1];
				
		switch (command)
		{
			case CMD_PICKUP_ITEM_LEFT:
				RobotArm_PickupLeft();
				break;
			case CMD_PICKUP_ITEM_RIGHT:
				RobotArm_PickupRight();
				break;
			case CMD_ROTATE_LEFT:
				globalForward = false;
				WheelMotors_RotateCounterClockwise();
				break;
			case CMD_ROTATE_RIGHT:
				globalForward = false;
				WheelMotors_RotateClockwise();
				break;
			case CMD_DRIVE_FORWARD:
				globalForward = true;
				break;
			case CMD_DRIVE_BACKWARD:
				globalForward = false;
				WheelMotors_DriveBackwards();
				break;
			case CMD_STOP:
				globalForward = false;
				WheelMotors_Stop();
				break;
			case CMD_SET_DRIVE_SPEED:
				GlobalDriveSpeed = rx_buffer[1];
				break;		
			case CMD_SET_TURN_SPEED:
				GlobalTurnSpeed = rx_buffer[1];
				break;
			case CMD_UPDATE_STEER_SIGNAL:
				UpdateSteerSignal(PARSE_AS(&rx_buffer[1], float));
				break;
			case CMD_DROP_OFF_ITEMS:
				RobotArm_DropOfItems();
				break;
			case CMD_TURN_ARM:
				TurnArmRotationAxle(GlobalArmRotation + diff);
				break;
			case CMD_TURN_BASE_AXLE:
				TurnBaseAxle(GlobalBaseRotation + diff);
				break;
			case CMD_TURN_MIDDLE_AXLE:
				TurnMiddleAxle(GlobalMiddleRotation + diff);
				break;
			case CMD_TURN_TOP_AXLE:
				TurnTopAxle(GlobalTopRotation + diff);
				break;
			case CMD_TURN_CLAW:
				TurnClawRotationAxle(GlobalClawRotation + diff);
				break;
			case CMD_OPEN_CLAW:
				ClawOpen();
				break;
			case CMD_CLOSE_CLAW:
				ClawClose();
				break;
			case CMD_START_PICKUP_LEFT:
				StartLeftPickup();
				break;
			case CMD_START_PICKUP_RIGHT:
				StartRightPickup();
				break;
			case CMD_END_PICKUP:
				EndPickupSequence();
				break;
			default:
				break; // TODO: Log unsupported command?
		}
	}
}

char GlobalSteerSigned = 0;

void UpdateSteerSignal(float signal)
{
	if (fabs(signal) >= 0xFF)
	{
		GlobalSteerSignalAmplitude = 0xFF;
	}
	else
	{
		GlobalSteerSignalAmplitude = (byte)(fabs(signal));
	}
	
	GlobalSteerSignalPositive = (signal>=0);	
}

void InitRobotArmServos(void)
{
	// Sets no status return message on all servos
	byte returnParams[] = { P_RETURN_LEVEL, 0x01};
	CreateDynamixelPacket(packetBuffer, 0xFE, INST_WRITE, returnParams, sizeof(returnParams));
	TransmitDynamixelPacket(packetBuffer);
	
	// Sets return delay time to 4us
	byte delayParams[] = {P_RETURN_DELAY_TIME , 0x02};
	CreateDynamixelPacket(packetBuffer, 0xFE, INST_WRITE, delayParams, sizeof(delayParams));
	TransmitDynamixelPacket(packetBuffer);
	
	// Sets max torque for all servos except claw
	byte torqueParams[] = { P_TORQUE_LIMIT_L, 0xFF, 0x03};
	CreateDynamixelPacket(packetBuffer, 0xFE, INST_WRITE, torqueParams, sizeof(torqueParams));
	TransmitDynamixelPacket(packetBuffer);
	
	// Sets max torque for claw to not break wares
	byte torqueParamsClaw[] = { P_TORQUE_LIMIT_L, 0x33, 0x02};
	CreateDynamixelPacket(packetBuffer, 0x08, INST_WRITE, torqueParamsClaw, sizeof(torqueParamsClaw));
	TransmitDynamixelPacket(packetBuffer);
	
	// Sets speed for all servos
	byte speedParams[] = { P_GOAL_SPEED_L, 0x88, 0x00};
	CreateDynamixelPacket(packetBuffer, 0xFE, INST_WRITE, speedParams, sizeof(speedParams));
	TransmitDynamixelPacket(packetBuffer);
	
	// Sets min/max rotation values for each servo individually
	short minMaxArray[] = {MOTOR1ROTATIONMIN, MOTOR1ROTATIONMAX, MOTOR2ROTATIONMIN, MOTOR2ROTATIONMAX, MOTOR3ROTATIONMIN, MOTOR3ROTATIONMAX, MOTOR4ROTATIONMIN, MOTOR4ROTATIONMAX,
	MOTOR5ROTATIONMIN, MOTOR5ROTATIONMAX, MOTOR6ROTATIONMIN, MOTOR6ROTATIONMAX, MOTOR7ROTATIONMIN, MOTOR7ROTATIONMAX, MOTOR8ROTATIONMIN, MOTOR8ROTATIONMAX};
	for (int i = 0; i < 8; ++i)
	{
		// Sets minimum allowed angles
		int currentId = i+1;
		int currentValueIndex = i*2;
		short minAngle = DegreesToAngle(minMaxArray[currentValueIndex]);
		byte minRotParams[] = { P_CW_ANGLE_LIMIT_L, (byte)minAngle, (byte)(minAngle >> 8)};
		CreateDynamixelPacket(packetBuffer, currentId, INST_WRITE, minRotParams, sizeof(minRotParams));
		TransmitDynamixelPacket(packetBuffer);

		// Sets maximum allowed angles
		currentValueIndex++;
		short maxAngle = DegreesToAngle(minMaxArray[currentValueIndex]);
		byte maxRotParams[] = { P_CCW_ANGLE_LIMIT_L, (byte)maxAngle, (byte)(maxAngle >> 8)};
		CreateDynamixelPacket(packetBuffer, currentId, INST_WRITE, maxRotParams, sizeof(maxRotParams));
		TransmitDynamixelPacket(packetBuffer);
	}
}


void RobotArm_DropOfItems(void)
{
	//Sets max torque for claw because box is durable
	byte torqueParamsClaw[] = { P_TORQUE_LIMIT_L, 0x33, 0x03};
	CreateDynamixelPacket(packetBuffer, 0x08, INST_WRITE, torqueParamsClaw, sizeof(torqueParamsClaw));
	TransmitDynamixelPacket(packetBuffer);
	//Goes into position
	TurnArmRotationAxle(90);
	TurnBaseAxle(145);
	TurnMiddleAxle(120);
	TurnTopAxle(215);
	TurnClawRotationAxle(205);
	_delay_ms(2000);
	//Swoops in and grabs it
	TurnArmRotationAxle(110);
	_delay_ms(1000);
	ClawClose();
	_delay_ms(2000);
	//Picks it up off the screws 
	TurnBaseAxle(143);
	TurnTopAxle(218);
	_delay_ms(500);
	TurnBaseAxle(141);
	TurnTopAxle(220);
	TurnArmRotationAxle(109);
	_delay_ms(500);
	TurnBaseAxle(139);
	TurnTopAxle(222);
	_delay_ms(500);
	//drops it
	TurnArmRotationAxle(60);
	TurnMiddleAxle(150);
	TurnTopAxle(220);
	TurnBaseAxle(150);
	_delay_ms(1000);
	TurnBaseAxle(200); //Changed here
	_delay_ms(500);
	TurnTopAxle(180); //Changed here
	ClawOpen();
	_delay_ms(1000);
	ReturnToDefaultPosition();
}

void RobotArm_PickupRight(void)
{
	GrabObjectRight();
	_delay_ms(1000);
	EndPickupSequence();
}

void RobotArm_PickupLeft(void)
{
	GrabObjectLeft();
	_delay_ms(1000);
	EndPickupSequence();
}

void EndPickupSequence(void)
{
	DropObject();
	_delay_ms(1000);
	ReturnToDefaultPosition();
}

void DropObject(void)
{
	TurnBaseAxle(60);
	_delay_ms(500);
	TurnTopAxle(150);
	TurnArmRotationAxle(130);
	TurnMiddleAxle(60);
	TurnClawRotationAxle(150);
	_delay_ms(1000);
	TurnBaseAxle(85);
	_delay_ms(500);
	ClawOpen();
}

void ReturnToDefaultPosition(void)
{
	ClawOpen();
	TurnBaseAxle(60);
	_delay_ms(500);
	TurnTopAxle(60);
	TurnArmRotationAxle(150);
	TurnMiddleAxle(60);
	TurnClawRotationAxle(150);
	
	_delay_ms(1000);
	// RobotArm_TorqueDisable();
}

void GrabObjectRight(void)
{
	StartRightPickup();
	_delay_ms(1000);
	
	// Swoop in and grab it
	TurnArmRotationAxle(240);
	_delay_ms(500);
	ClawClose();
}

void GrabObjectLeft(void)
{
	StartLeftPickup();
	_delay_ms(1000);
	
	// Swoop in and grab it
	TurnArmRotationAxle(60);
	_delay_ms(500);
	ClawClose();
}

void StartLeftPickup(void)
{
	// Set grab pos to right height but off 30 degrees
	TurnClawRotationAxle(60);
	TurnArmRotationAxle(90);
	TurnBaseAxle(150);
	TurnMiddleAxle(130);
	_delay_ms(500);
	TurnTopAxle(185);
	TurnBaseAxle(185);
}

void StartRightPickup(void)
{
	// Set grab pos to right height but off 30 degrees
	TurnClawRotationAxle(60);
	TurnArmRotationAxle(210);
	TurnBaseAxle(150);
	TurnMiddleAxle(130);
	_delay_ms(500);
	TurnTopAxle(185);
	TurnBaseAxle(185);
}

void TurnArmRotationAxle(short angleInDegrees)
{
	GlobalArmRotation = angleInDegrees;
	TurnSingle(0x01, DegreesToAngle(angleInDegrees), GLOBALSERVOSPEED);
}

void TurnBaseAxle(short angleInDegrees)
{
	GlobalBaseRotation = angleInDegrees;
	TurnPairMirrored(0x02, 0x03, DegreesToAngle(angleInDegrees), GLOBALSERVOSPEED);
}

void TurnMiddleAxle(short angleInDegrees)
{
	GlobalMiddleRotation = angleInDegrees;
	TurnPairMirrored(0x04, 0x05, DegreesToAngle(angleInDegrees), GLOBALSERVOSPEED);
}

void TurnTopAxle(short angleInDegrees)
{
	GlobalTopRotation = angleInDegrees;
	TurnSingle(0x06, DegreesToAngle(angleInDegrees), GLOBALSERVOSPEED);
}

void TurnClawRotationAxle(short angleInDegrees)
{
	GlobalClawRotation = angleInDegrees;
	TurnSingle(0x07, DegreesToAngle(angleInDegrees), GLOBALSERVOSPEED);
}

void ClawOpen(void)
{
	TurnSingle(0x08, DegreesToAngle(150), CLAW_SPEED);
}

void ClawClose(void)
{
	TurnSingle(0x08, 0x000, CLAW_SPEED);
}

void TurnPairMirrored(byte id0, byte id1, short angle0, short speed)
{
	// Set same speed
	byte speed_params[] = { P_GOAL_SPEED_L, (byte)speed, (byte)(speed >> 8)};
	CreateDynamixelPacket(packetBuffer, id0, INST_WRITE, speed_params, sizeof(speed_params));
	TransmitDynamixelPacket(packetBuffer);
	CreateDynamixelPacket(packetBuffer, id1, INST_WRITE, speed_params, sizeof(speed_params));
	TransmitDynamixelPacket(packetBuffer);

	// Calculate angles for both motors (angle1 is mirrored)
	short angle1 = 0x3FF - angle0;

	// Set angles
	byte position_params[] = {
		P_GOAL_POSITION_L, 0x02, 
		id0, (byte)angle0, (byte)(angle0 >> 8), 
		id1, (byte)angle1, (byte)(angle1 >> 8)
	};
	CreateDynamixelPacket(packetBuffer, 0xFE, INST_SYNC_WRITE, position_params, sizeof(position_params));
	TransmitDynamixelPacket(packetBuffer);
}

void TurnSingle(byte id, short angle, short speed)
{
	// Set speed
	byte speed_params[] = { P_GOAL_SPEED_L, (byte)speed, (byte)(speed >> 8)};
	CreateDynamixelPacket(packetBuffer, id, INST_WRITE, speed_params, sizeof(speed_params));
	TransmitDynamixelPacket(packetBuffer);
	
	// Set angle
	byte position_params[] = { P_GOAL_POSITION_L, (byte)angle, (byte)(angle >> 8) };
	CreateDynamixelPacket(packetBuffer, id, INST_WRITE, position_params, sizeof(position_params));
	TransmitDynamixelPacket(packetBuffer);
}

// Refer to data sheet
void CreateDynamixelPacket(byte *packet, byte id, byte instruction, byte *parameters, byte parameters_length)
{
	packet[0] = 0xFF;
	packet[1] = 0xFF;
	packet[2] = id;
	packet[3] = 2 + parameters_length;
	packet[4] = instruction;
	for (int i = 0; i < parameters_length; ++i) {
		packet[5 + i] = parameters[i];
	}

	byte checksum = 0;
	byte packet_length = 6 + parameters_length;
	for (int i = 2; i < packet_length - 1; i++) {
		checksum += packet[i];
	}
	checksum = ~checksum;

	packet[5 + parameters_length] = checksum;
}

short DegreesToAngle(float degrees)
{
	return (short)(0x3FF * (degrees / 300));
}

void PortInitialize(void)
{
	DDRA = DDRB = DDRC = DDRD = 0; // Set all port to input direction first.
	PORTB = PORTC = PORTD = 0x00; // PortData initialize to 0
	PORTD = 0x03; // pull up on UART lines
	DDRD = 0x04; // snitch signal on PD4
}

// ATmega1284P page 201
void USART_Init(void)
{
	UBRR0H = (byte)(MYUBRR >> 8);
	UBRR0L = (byte)MYUBRR;
	UCSR0A = (1 << U2X0); // Set double baud rate
	UCSR0C = (1 << UCSZ01)|(1 << UCSZ00); // 8 data bits, no parity bits, 1 stop bit
	DynamixelSetReceiveMode();
}

void USART_Transmit(byte data)
{
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = data;
}

byte USART_Receive(void)
{
	while (!(UCSR0A & (1 << RXC0)));
	return UDR0;
}

void DynamixelSetTransmitMode() {
	UCSR0B = (1 << TXEN0)|(0 << RXEN0);
	PORTD |= (1<<2);
}
void DynamixelSetReceiveMode() {
	while(!TXC0);
	UCSR0B = (0 << TXEN0)|(1 << RXEN0);
	PORTD &= ~(1<<2);
}

void TransmitDynamixelPacket(byte *packet)
{
	DynamixelSetTransmitMode();
	byte packetLength = 4 + packet[3];
	for (byte i = 0; i < packetLength; ++i)
	{
		USART_Transmit(packet[i]);
	}
	UCSR0A |= (1<<TXC0); // clear transmit complete flag by writing '1'
	DynamixelSetReceiveMode();
}

void PWM_Init(void)
{
	// Enable PWM outputs on PD4 (Pin 18) and OC2B on PD5 (Pin 19)
	DDRD |= (1 << PIND4)|(1 << PIND5);
	
	TCCR1A |= (1 << COM1A1) | (1 << COM1B1) | (1 << WGM10);
	TCCR1B |= (1 << WGM12) | (3 << CS10); // Prescaler = 8
	
	// Set initial duty
	// 0% = 0x0000, 100% = 0x00FF
	// Wheels will only change direction on falling flank, so you cannot turn with a 100% duty cycle
	OCR1A = 0x00;
	OCR1B = 0x00;
}

void WheelMotors_Init(void)
{
	PWM_Init();
	DDRD |= (1 << PIND6)|(1 << PIND7); // For DIR signals;
	PORTD |= (1 << PIND6);
}


static inline int clampInt(int val, int min, int max) {
	if (val < min) return min;
	if (val > max) return max;
	return val;
}

void WheelMotors_DriveForwardWithSteering(void)
{
	byte DriveSpeed = GlobalDriveSpeed - ((uint16_t)GlobalSteerSignalAmplitude * GlobalSteeringSlowdown / 0xFF);
	
	int SteerLeft  = DriveSpeed;
	int SteerRight = DriveSpeed;

	if (GlobalSteerSignalPositive)
	{
		SteerLeft  += GlobalSteerSignalAmplitude; // Left hand turn
		SteerRight -= GlobalSteerSignalAmplitude; // Left hand turn
	}
	else
	{
		SteerLeft  -= GlobalSteerSignalAmplitude; // Right hand turn
		SteerRight += GlobalSteerSignalAmplitude; // Right hand turn
	}
	
	// Speed
	OCR1A = clampInt(SteerLeft,  0, 0xFF);
	OCR1B = clampInt(SteerRight, 0, 0xFF);
	
	// Direction
	PORTD |= (1 << PIND6);
	PORTD &= ~(1 << PIND7);
}

void WheelMotors_DriveForward(void)
{
	// Speed
	OCR1A = GlobalDriveSpeed;
	OCR1B = GlobalDriveSpeed;
	
	// Direction
	PORTD |= (1 << PIND6);
	PORTD &= ~(1 << PIND7);
}

void WheelMotors_DriveBackwards(void)
{
	// Speed
	OCR1A = GlobalDriveSpeed;
	OCR1B = GlobalDriveSpeed;
	
	// Direction
	PORTD &= ~(1 << PIND6);
	PORTD |= (1 << PIND7);
}

void WheelMotors_RotateClockwise(void)
{
	// Speed
	OCR1A = GlobalTurnSpeed;
	OCR1B = GlobalTurnSpeed;
	
	// Direction
	PORTD |= (1 << PIND6);
	PORTD |= (1 << PIND7);
}

void WheelMotors_RotateCounterClockwise(void)
{
	// Speed
	OCR1A = GlobalTurnSpeed;
	OCR1B = GlobalTurnSpeed;
	
	// Direction
	PORTD &= ~(1 << PIND6);
	PORTD &= ~(1 << PIND7);
}

void WheelMotors_Stop(void)
{
	// Speed
	OCR1A = 0x00;
	OCR1B = 0x00;
}
