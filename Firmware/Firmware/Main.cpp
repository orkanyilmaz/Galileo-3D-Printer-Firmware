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

#define FAN_SWITCH 3
#define HOT_BED_SWTICH 5
#define HOT_END_SWITCH 6

#define Z_AXIS_DIR 8
#define Z_AXIS_1_STEP 9
#define Z_AXIS_2_STEP 10

#define Y_AXIS_DIR 0
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
TemperatureReader temp_reader(A1);

double input = 0, kp = 19, ki = .6, kd = 150,setpoint = 80;
double output = 0;
unsigned int aTuneLookBack = 20;

boolean tuning = false;
unsigned long  modelTime, serialTime;

double aTuneStep = 10, aTuneNoise = 1, aTuneStartValue = 0;

PID heaterPid(&input, &output, &setpoint, kp, ki, kd, DIRECT);
PID_ATune aTune(&input, &output);

void AutoTuneHelper(boolean start);
void SerialSend();
void changeAutoTune();

void setup()
{
	pinMode(11, OUTPUT);
	pinMode(10, OUTPUT);
	pinMode(9, OUTPUT);
	analogWrite(9, 0);
	analogWrite(11,150);
	
	temp_reader.BeginNewRecording(web_client, uri_builder(U("/Temperature/AddTemperatureTest")));

	heaterPid.SetMode(AUTOMATIC);
	heaterPid.SetOutputLimits(0, 255);
	if (tuning)
	{
		tuning = false;
		changeAutoTune();
		tuning = true;
	}

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
			heaterPid.SetTunings(kp, ki, kd);
			AutoTuneHelper(false);
		}
	}
	else heaterPid.Compute();


	if (input < 300)
	{
		analogWrite(10, output);
	}
	else
	{
		analogWrite(10, 0);
	}
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
	Log(L"setpoint: %f\n", setpoint);
	Log(L"input: %f\n", input);
	Log(L"output: %f\n", output);
}