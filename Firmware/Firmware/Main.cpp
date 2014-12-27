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

//double input = 0, kp = 2.61, ki = 0.12528, kd = 0, setpoint = 175;
double input = 0, kp = .9675, ki = 0.049923, kd = 0, setpoint = 190;
//double input = 0, kp = 1.1925, ki = 0.05724, kd = 0, setpoint = 190;
double output = 0;
unsigned int aTuneLookBack = 60;

boolean tuning = false;
unsigned long  modelTime, serialTime;

double aTuneStep = 45, aTuneNoise = 1, aTuneStartValue = 45;

PID heaterPid(&input, &output, &setpoint, kp, ki, kd, DIRECT);
PID_ATune aTune(&input, &output);

const double e_steps_mm = (STEPS_REV * 1) * (GREGS_GEAR_RATIO) / (7 * 3.14159);
const int stepper_steps_mm = (STEPS_REV * 1) / (2 * 20);

int readingCount = 0;
bool stable = false;


void AutoTuneHelper(boolean start);
void SerialSend();
void changeAutoTune();
void InitializePins();
void print_mm(int mm, char axis);
void extrude_mm(int mm);
void controlTemp(void * aArg);

void setup()
{
	try
	{
		InitializePins();
		analogWrite(FAN_SWITCH, 180);
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



}
void loop()
{
	if (temp_reader.IsStable())
	{
		extrude_mm(5);
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
void extrude_mm(int mm)
{
	int step1Pin = EXTRUDER_STEP;
	int direction1Pin = EXTRUDER_DIR;

	digitalWrite(direction1Pin, LOW);
	for (int miliMeters = 0; miliMeters < mm; miliMeters++)
	{
		for (int i = 0; i < stepper_steps_mm; i++)
		{
			for (int j = 0; j < floor(e_steps_mm / stepper_steps_mm); j++)
			{
				digitalWrite(step1Pin, HIGH);
				delayMicroseconds(25);

				digitalWrite(step1Pin, LOW);
				delayMicroseconds(25);
			}

		}
		print_mm(1, 'X');
	}
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

		for (int i = 0; i < 10; i++)
		{
			digitalWrite(step1Pin, HIGH);
			if (step2Pin != -1) digitalWrite(step2Pin, HIGH);
			delayMicroseconds(25);

			digitalWrite(step1Pin, LOW);
			if (step2Pin != -1) digitalWrite(step2Pin, LOW);
			delayMicroseconds(25);

		}
	}
}

void controlTemp(void * aArg)
{
	try
	{
		while (true)
		{
			//if (readingCount > 50)
			//{
			//	readingCount = 0;
			//	if (kp >= 3)
			//	{
			//		_exit_arduino_loop();
			//	}
			//	double newKp = heaterPid.GetKp() + .05;
			//	heaterPid.SetTunings(newKp, heaterPid.GetKi(), heaterPid.GetKd());
			//	temp_reader.BeginNewRecording(web_client, uri_builder(U("/Temperature/AddTemperatureTest")), newKp, ki, kd);
			//}
			unsigned long now = millis();
			if (temp_reader.GetReadingSendCount() == 49) readingCount++;
			input = temp_reader.GetEndTemp(web_client, uri_builder(U("/Temperature/AddTemperatureTestData")));


			//if (tuning)
			//{
			//	byte val = (aTune.Runtime());
			//	if (val != 0)
			//	{
			//		tuning = false;
			//	}
			//	if (!tuning)
			//	{ //we're done, set the tuning parameters
			//		kp = aTune.GetKp();
			//		ki = aTune.GetKi();
			//		kd = aTune.GetKd();
			//		Log(L"Kp: %f", kp);
			//		Log(L"Ki: %f", ki);
			//		Log(L"Kd: %f\n", kd);
			//		heaterPid.SetTunings(kp, ki, kd);
			//		AutoTuneHelper(false);
			//	}
			//}
			//else 
			heaterPid.Compute();

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
			if (millis() > serialTime)
			{
				SerialSend();
				serialTime += 500;
			}
		}


	}
	catch (std::exception& e)
	{

	}
}