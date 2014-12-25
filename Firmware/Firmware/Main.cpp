// Main.cpp : Defines the entry point for the console application.
//
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>

#include "PID_v1.h"
#include "PID_AutoTune_v0.h"
#include "stdafx.h"
#include "arduino.h"

#include "TemperatureReader.h"

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

#define STEPS_REV 400
#define GREGS_GEAR_RATIO 39/11

#define HOT_END_TEMP A0
#define HOT_BED_TEMP A1

#define FAN_SWITCH 3
#define HOT_BED_SWITCH 5
#define HOT_END_SWITCH 6

#define Z_AXIS_DIR 8
#define Z_AXIS_1_STEP 9
#define Z_AXIS_2_STEP 10

#define Y_AXIS_1_DIR 0
#define Y_AXIS_1_STEP 1
#define Y_AXIS_2_STEP 2

#define X_AXIS_1_DIR 11
#define X_AXIS_1_STEP 12

#define EXTRUDER_DIR 4
#define EXTRUDER_STEP 13

int _tmain(int argc, _TCHAR* argv[])
{
    return RunArduinoSketch();
}
byte ATuneModeRemember = 2;

http_client web_client(U("http://archos.azurewebsites.net"));
TemperatureReader temp_reader(HOT_END_TEMP);

double input = 0, kp = 5.125940, ki = .094526, kd = 69.492371, setpoint = 150;
double output = 0;
unsigned int aTuneLookBack = 60;

boolean tuning = true;
unsigned long  modelTime, serialTime;

double aTuneStep = 45, aTuneNoise = 1, aTuneStartValue = 45;

PID heaterPid(&input, &output, &setpoint, kp, ki, kd, DIRECT);
PID_ATune aTune(&input, &output);

const double e_steps_mm = (STEPS_REV * 1) * (GREGS_GEAR_RATIO) / (7 * 3.14159);
const int stepper_steps_mm = (STEPS_REV * 1) / (2 * 20);

int tuneCount = 0;

void AutoTuneHelper(boolean start);
void SerialSend();
void changeAutoTune();
void InitializePins();
void print_mm(int mm, char axis);

void setup()
{
	InitializePins();
	analogWrite(FAN_SWITCH, 180);
	temp_reader.BeginNewRecording(web_client, uri_builder(U("/Temperature/AddTemperatureTest")));
	aTune.SetControlType(1);
	heaterPid.SetMode(AUTOMATIC);
	heaterPid.SetOutputLimits(0, 90);

	if (tuning)
	{
		tuning = false;
		changeAutoTune();
		tuning = true;
	}
	//print_mm(1, 'Z');
	serialTime = 0;
}
void loop()
{
	unsigned long now = millis();

	input = temp_reader.GetEndTemp(web_client, uri_builder(U("/Temperature/AddTemperatureTestData")));


	if (tuning)
	{
		byte val = (aTune.Runtime());
		if (val != 0)
		{
			tuning = false;
		}
		if (!tuning)
		{ //we're done, set the tuning parameters
			kp = aTune.GetKp();
			ki = aTune.GetKi();
			kd = aTune.GetKd();
			Log(L"Kp: %f", kp);
			Log(L"Ki: %f",ki);
			Log(L"Kd: %f\n", kd);
			heaterPid.SetTunings(kp, ki, kd);
			AutoTuneHelper(false);
			tuneCount++;
			if (tuneCount <= 8)
			{
				tuning = false;
				changeAutoTune();
				tuning = true;
				
			}
			else
			{

			}
		}
	}
	else heaterPid.Compute();

	//if (input < 250)
	//{
		analogWrite(HOT_END_SWITCH, output);
	//}
	//else
	//{
	//	while (input > 50)
	//	{
	//		analogWrite(HOT_END_SWITCH, 0);
	//		analogWrite(FAN_SWITCH, 200);
	//		input = temp_reader.GetEndTemp(web_client, uri_builder(U("/Temperature/AddTemperatureTestData")));
	//		Log(L"%f\n", input);
	//	}
	//	//_exit_arduino_loop();
	//}
	//send-receive with processing if it's time
	if (millis()>serialTime)
	{
		SerialSend();
		serialTime += 500;
	}
}

void changeAutoTune()
{
	if (!tuning)
	{
		//Set the output to the desired starting frequency.
		output = aTuneStartValue;
		aTune.SetNoiseBand(aTuneNoise);
		aTune.SetOutputStep(aTuneStep);
		aTune.SetLookbackSec((int)aTuneLookBack);
		AutoTuneHelper(true);
		tuning = true;
	}
	else
	{ //cancel autotune
		aTune.Cancel();
		tuning = false;
		AutoTuneHelper(false);
	}
}

void AutoTuneHelper(boolean start)
{
	if (start)
		ATuneModeRemember = heaterPid.GetMode();
	else
		heaterPid.SetMode(ATuneModeRemember);
}


void SerialSend()
{
	Log(L"Kp: %f,", heaterPid.GetKp());
	Log(L"Ki: %f,", heaterPid.GetKi());
	Log(L"Kd: %f\n", heaterPid.GetKd());
	Log(L"Tuning: " + tuning ? "True\n": "False\n");
	Log(L"setpoint: %f\n", setpoint);
	Log(L"input: %f\n", input);
	Log(L"output: %f\n", output);
}

void InitializePins()
{
	pinMode(FAN_SWITCH, OUTPUT);
	analogWrite(FAN_SWITCH, 0);
	pinMode(HOT_BED_SWITCH, OUTPUT);
	analogWrite(HOT_BED_SWITCH, 0);
	pinMode(HOT_END_SWITCH, OUTPUT);
	analogWrite(HOT_END_SWITCH, 0);

	pinMode(Z_AXIS_DIR, OUTPUT);
	digitalWrite(Z_AXIS_DIR, 0);
	pinMode(Z_AXIS_1_STEP, OUTPUT);
	digitalWrite(Z_AXIS_1_STEP, 0);
	pinMode(Z_AXIS_2_STEP, OUTPUT);
	digitalWrite(Z_AXIS_2_STEP, 0);

	pinMode(Y_AXIS_1_DIR, OUTPUT);
	digitalWrite(Y_AXIS_1_DIR, 0);
	pinMode(Y_AXIS_1_STEP, OUTPUT);
	digitalWrite(Y_AXIS_1_STEP, 0);
	pinMode(Y_AXIS_2_STEP, OUTPUT);
	digitalWrite(Y_AXIS_2_STEP, 0);

	pinMode(X_AXIS_1_DIR, OUTPUT);
	digitalWrite(X_AXIS_1_DIR, 0);
	pinMode(X_AXIS_1_STEP, OUTPUT);
	digitalWrite(X_AXIS_1_STEP, 0);

	pinMode(EXTRUDER_DIR, OUTPUT);
	digitalWrite(EXTRUDER_DIR, 0);
	pinMode(EXTRUDER_STEP, OUTPUT);
	digitalWrite(EXTRUDER_STEP, 0);
}
void print_mm(int mm, char axis)
{ 
	int direction2Pin = -1;
	int direction1Pin = -1;
	int step1Pin = -1;
	int step2Pin = -1;
	switch (axis)
	{
	case 'E':
		step1Pin = EXTRUDER_STEP;
		direction1Pin = EXTRUDER_DIR;
		break;
	case 'X':
		step1Pin = X_AXIS_1_STEP;
		direction1Pin = X_AXIS_1_DIR;
		break;
	case 'Y':
		step1Pin = Y_AXIS_1_STEP;
		step2Pin = Y_AXIS_2_STEP;
		direction1Pin = Y_AXIS_1_DIR;
		break;
	case 'Z':
		step1Pin = Z_AXIS_1_STEP;
		step2Pin = Z_AXIS_2_STEP;
		direction1Pin = Z_AXIS_DIR;
		break;
	}
	digitalWrite(direction1Pin, HIGH);

	for (int miliMeters = 0; miliMeters < mm; miliMeters++)
	{



		for (int i = 0; i < stepper_steps_mm; i++)
		{
			for (int j = 0; j < floor(e_steps_mm / stepper_steps_mm); j++)
			{
				digitalWrite(step1Pin, HIGH);
				if (step2Pin != -1) digitalWrite(step2Pin, HIGH);
				delayMicroseconds(10000);

				digitalWrite(step1Pin, LOW);
				if (step2Pin != -1) digitalWrite(step2Pin, LOW);
				delayMicroseconds(10000);
			}
		}
	}
}