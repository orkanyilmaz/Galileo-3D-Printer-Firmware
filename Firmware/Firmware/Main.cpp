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

int _tmain(int argc, _TCHAR* argv[])
{
    return RunArduinoSketch();
}
byte ATuneModeRemember = 2;

http_client web_client(U("http://archos.azurewebsites.net"));
TemperatureReader temp_reader(A1);

double input = 0, kp = 1, ki = 0, kd = 0, setpoint = 80;
double output = 0;
unsigned int aTuneLookBack = 20;

boolean tuning = true;
unsigned long  modelTime, serialTime;

double aTuneStep = 10, aTuneNoise = 1, aTuneStartValue = 20;

PID heaterPid(&input, &output, &setpoint, kp, ki, kd, DIRECT);
PID_ATune aTune(&input, &output);

void AutoTuneHelper(boolean start);
void SerialSend();
void changeAutoTune();

void setup()
{
	pinMode(10, OUTPUT);
	pinMode(11, OUTPUT);
	analogWrite(11, 180);
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


	analogWrite(10, output);
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
	if (tuning){
		Log(L"tuning mode");
	}
	else {
		Log(L"kp: $f\n", heaterPid.GetKp());
		Log(L"ki: $f\n", heaterPid.GetKi());
		Log(L"kd: %f\n", heaterPid.GetKd());
	}
}