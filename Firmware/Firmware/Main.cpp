// Main.cpp : Defines the entry point for the console application.
//
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include "tinythread.h"
#include "fast_mutex.h"

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
using namespace tthread;

#define STEPS_REV 400
#define GREGS_GEAR_RATIO 39/11

#define HOT_END_TEMP A0
#define HOT_BED_TEMP A1

#define FAN_SWITCH 3
#define HOT_BED_SWITCH 5
#define HOT_END_SWITCH 6

#define Z_AXIS_DIR 7
#define Z_AXIS_1_STEP 10
#define Z_AXIS_2_STEP 11

#define Y_AXIS_1_DIR 0
#define Y_AXIS_1_STEP 1
#define Y_AXIS_2_STEP 4

#define X_AXIS_1_DIR 8
#define X_AXIS_1_STEP 9

#define EXTRUDER_DIR 12
#define EXTRUDER_STEP 13

int _tmain(int argc, _TCHAR* argv[])
{
	return RunArduinoSketch();
}
byte ATuneModeRemember = 2;

http_client web_client(U("http://archos.azurewebsites.net"));
TemperatureReader temp_reader(HOT_END_TEMP);

//double input = 0, kp = 22.350830, ki = 0.556857, kd = 175, setpoint = 245; // 145
//double input = 0, kp = .9675, ki = 0.049923, kd = 0, setpoint = 190; // 190
//double input = 0, kp = .9675, ki = 0.01248075, kd = 0, setpoint = 190; // 190 0.049923
//double input = 0, kp = 18.676056338028169014084507042254, ki = 2.25, kd = 1.25, setpoint = 220; // 75
double input = 0, kp = 30, ki = 0, kd = 0, setpoint = 90;
double output = 0;
unsigned int aTuneLookBack = 30;

boolean tuning = false;
unsigned long  modelTime, serialTime;

double aTuneStep = 120, aTuneNoise = 1, aTuneStartValue = 120;

PID heaterPid(&input, &output, &setpoint, kp, ki, kd, DIRECT);
PID_ATune aTune(&input, &output);
//
double e_steps_mm = (STEPS_REV * 1) * (GREGS_GEAR_RATIO) / (7 * 3.14159);
double layerHeight = .20;
const int stepper_steps_mm = (STEPS_REV * 1) / (2 * 20);

int readingCount = 0;
bool stable = false;

bool xDirection = false; //False = Left, True = Right
bool yDirection = true; //False = Up, True = Down
bool zDirection = false; //False = Forwards, True = Backwards
bool eDirection = false; //False = Extrude, True = Retract

int microSecondsPerStep = 100;

int tuningCount = 0;
void AutoTuneHelper(boolean start);
void SerialSend();
void changeAutoTune();
void InitializePins();
void print_mm(int mm, char axis);
void extrude_mm(int mm, bool shouldStep);
void controlTemp(void * aArg);
void step(char axis);
void setup()
{
	InitializePins();
	try
	{


		temp_reader.BeginNewRecording(web_client, uri_builder(U("/Temperature/AddTemperatureTest")), kp, ki, kd);
		aTune.SetControlType(1);
		heaterPid.SetMode(AUTOMATIC);
		heaterPid.SetOutputLimits(0, 255);

		if (tuning)
		{
			tuning = false;
			changeAutoTune();
			tuning = true;
		}
		//print_mm(1, 'Z');
		serialTime = 0;
		thread tempControl(controlTemp, 0);
		tempControl.detach();
	}
	catch (std::exception& e)
	{

	}
	extrude_mm(5, false);
	for (int i = 0; i < 20; i++)
	{
		for (int j = 0; j < 20; j++)
		{
			extrude_mm(1, true);
		}
		print_mm(1, 'Y');
		xDirection = !xDirection;
	}
	print_mm(3, 'Y');
}
void loop()
{
	//InitializePins();
	//print_mm(1, 'X');
	//Log(L"%f\n",temp_reader.GetEndTemp(web_client, uri_builder(U("/Temperature/AddTemperatureTestData"))));
	//delay(1000);
	//if (temp_reader.IsStable())
	//{
		//extrude_mm(5);
	//}


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
	Log(L"Reading Count: %i\n", readingCount);
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
void extrude_mm(int mm, bool shouldStep)
{
	int step1Pin = EXTRUDER_STEP;
	int direction1Pin = EXTRUDER_DIR;

	if (!eDirection)
		digitalWrite(direction1Pin, LOW);
	else
		digitalWrite(direction1Pin, HIGH);
	double stepsPerMM = 8 / floor(e_steps_mm*layerHeight);
	double stepCount = 0;
	int previousStep = 0;
	for (int miliMeters = 0; miliMeters < mm; miliMeters++)
	{
		for (int j = 0; j < floor(e_steps_mm*layerHeight); j++)
		{
			digitalWrite(step1Pin, HIGH);
			delayMicroseconds(microSecondsPerStep);

			digitalWrite(step1Pin, LOW);
			delayMicroseconds(microSecondsPerStep);
			stepCount += stepsPerMM;

			if (floor(stepCount) != previousStep)
			{
				if (shouldStep)
					step('X');
				previousStep = floor(stepCount);
			}
		}
		previousStep = 0;
		stepCount = 0;

	}
}
void step(char axis)
{
	int direction2Pin = -1;
	int direction1Pin = -1;
	int step1Pin = -1;
	int step2Pin = -1;
	switch (axis)
	{
	case 'X':
		step1Pin = X_AXIS_1_STEP;
		direction1Pin = X_AXIS_1_DIR;
		if (!xDirection)
			digitalWrite(direction1Pin, LOW);
		else
			digitalWrite(direction1Pin, HIGH);
		break;
	case 'Y':
		step1Pin = Y_AXIS_1_STEP;
		step2Pin = Y_AXIS_2_STEP;
		direction1Pin = Y_AXIS_1_DIR;
		if (!yDirection)
			digitalWrite(direction1Pin, LOW);
		else
			digitalWrite(direction1Pin, HIGH);
		break;
	case 'Z':
		step1Pin = Z_AXIS_1_STEP;
		step2Pin = Z_AXIS_2_STEP;
		direction1Pin = Z_AXIS_DIR;
		if (!zDirection)
			digitalWrite(direction1Pin, LOW);
		else
			digitalWrite(direction1Pin, HIGH);
		break;
	}
	//digitalWrite(direction1Pin, HIGH);

			digitalWrite(step1Pin, HIGH);
			if (step2Pin != -1) digitalWrite(step2Pin, HIGH);
			delayMicroseconds(microSecondsPerStep);

			digitalWrite(step1Pin, LOW);
			if (step2Pin != -1) digitalWrite(step2Pin, LOW);
			delayMicroseconds(microSecondsPerStep);
}
void print_mm(int mm, char axis)
{
	int direction2Pin = -1;
	int direction1Pin = -1;
	int step1Pin = -1;
	int step2Pin = -1;
	switch (axis)
	{
	case 'X':
		step1Pin = X_AXIS_1_STEP;
		direction1Pin = X_AXIS_1_DIR;
		if (!xDirection)
			digitalWrite(direction1Pin, LOW);
		else
			digitalWrite(direction1Pin, HIGH);
		break;
	case 'Y':
		step1Pin = Y_AXIS_1_STEP;
		step2Pin = Y_AXIS_2_STEP;
		direction1Pin = Y_AXIS_1_DIR;
		if (!yDirection)
			digitalWrite(direction1Pin, LOW);
		else
			digitalWrite(direction1Pin, HIGH);
		break;
	case 'Z':
		step1Pin = Z_AXIS_1_STEP;
		step2Pin = Z_AXIS_2_STEP;
		direction1Pin = Z_AXIS_DIR;
		if (!zDirection)
			digitalWrite(direction1Pin, LOW);
		else
			digitalWrite(direction1Pin, HIGH);
		break;
	}
	for (int miliMeters = 0; miliMeters < mm; miliMeters++)
	{

		for (int i = 0; i < 8; i++)
		{
			digitalWrite(step1Pin, HIGH);
			if (step2Pin != -1) digitalWrite(step2Pin, HIGH);
			delayMicroseconds(microSecondsPerStep);

			digitalWrite(step1Pin, LOW);
			if (step2Pin != -1) digitalWrite(step2Pin, LOW);
			delayMicroseconds(microSecondsPerStep);

		}
	}
}

void controlTemp(void * aArg)
{
	try
	{
		while (true)
		{
			//if (readingCount >= 100)
			//{
			//	readingCount = 0;
			//	if (kp >= 25)
			//	{
			//		InitializePins();
			//		_exit_arduino_loop();
			//	}
			//	double newKp = heaterPid.GetKp() + 1;
			//	heaterPid.SetTunings(newKp, heaterPid.GetKi(), heaterPid.GetKd());
			//	temp_reader.BeginNewRecording(web_client, uri_builder(U("/Temperature/AddTemperatureTest")), newKp, ki, kd);
			//}
			//unsigned long now = millis();
			//if (temp_reader.GetReadingSendCount() == 4) readingCount++;
			//
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
					Log(L"Ki: %f", ki);
					Log(L"Kd: %f\n", kd);
					heaterPid.SetTunings(kp, ki, kd);
					AutoTuneHelper(false);
				}
			}
			else 
			heaterPid.Compute();

			//if (input < 250)
			////{
			analogWrite(HOT_END_SWITCH, output);
			analogWrite(HOT_BED_SWITCH, 35);
			analogWrite(FAN_SWITCH, 120);
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
			if (millis() > serialTime)
			{
				SerialSend();
				serialTime += 500;
			}
		}


	}
	catch (std::exception& e)
	{
		Log(L"%s", e);
	}
}