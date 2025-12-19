#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SCALE 1
#define PHYSICAL_DISTANCE 1

#define DELTA_T 1

#define SENSOR_COUNT 11
int lineSensorData[2][SENSOR_COUNT];
int k_P;
int k_Theta;
volatile int u_styrsignal; // Styrsignal

typedef enum { SENSOR_DATA, UPDATE_PARAMETERS } PackageKind;

typedef struct {
	int lineSensorData[2][SENSOR_COUNT];
} SensorData;

typedef struct {
	int k_P;
	int k_Theta;
} UpdateParameters;

typedef struct {
	PackageKind kind;
	union {
		SensorData sensorData;
		UpdateParameters updateParameters;
	} data;
} Package;

Package readI2CData()
{
	// TODO
	Package package;
	return package;
}

void checkForPickupSpotOrKorsning()
{
	// TODO
}

void MainLoop()
{
	Package package = readI2CData();
	switch (package.kind) {
	case SENSOR_DATA:
		checkForPickupSpotOrKorsning();
		memcpy(&package.data.sensorData.lineSensorData, &lineSensorData,
		       sizeof(lineSensorData));
	case UPDATE_PARAMETERS:
		k_P = package.data.updateParameters.k_P;
		k_Theta = package.data.updateParameters.k_Theta;
	}
}

int PDReglering(int sensor)
{
	int sum1 = 0;
	int sum2 = 0;

	for (int i = 0; i < SENSOR_COUNT; ++i) {
		sum1 += lineSensorData[sensor][i] * i;
		sum2 += lineSensorData[sensor][i];
	}

	return sum1 / sum2;
}

void SendToMotors(int u_styrsignal)
{
	// TODO
}

void Timer_ISR()
{
	SendToMotors(u_styrsignal);
	int error0 = PDReglering(0);
	int error1 = PDReglering(1);
	u_styrsignal = k_P * error0 +
		       k_Theta * (error0 - error1) * SCALE / PHYSICAL_DISTANCE;
}

void setup()
{
	// Do hardware setup
}

int main()
{
	setup();
	while (true) {
		MainLoop();
	}
	return 0;
}
