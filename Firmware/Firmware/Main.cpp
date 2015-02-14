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
#include "spi.h"

#include "TemperatureReader.h"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

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

#define FAN_SWITCH 9
#define HOT_BED_SWITCH 5
#define HOT_END_SWITCH 6

#define SPI_LATCH 3

#define Z_AXIS_DIR 16
#define Z_AXIS_STEP 32

#define Y_AXIS_DIR 4
#define Y_AXIS_STEP 8

#define X_AXIS_DIR 1
#define X_AXIS_STEP 2

#define EXTRUDER_DIR 64
#define EXTRUDER_STEP 128

#define GPIO_FAST_IO3 27
#define GPIO_FAST_IO2 45

int _tmain(int argc, _TCHAR* argv[])
{
	return RunArduinoSketch();
}

struct PARAMS
{
	double temperature;
	http_client* client;
	uri_builder resource;
	int currentTestId;
};
byte ATuneModeRemember = 2;

http_client web_client(U("http://archos.azurewebsites.net"));
TemperatureReader temp_reader(HOT_END_TEMP);

//double input = 0, kp = 22.350830, ki = 0.556857, kd = 175, setpoint = 245; // 145
//double input = 0, kp = .9675, ki = 0.049923, kd = 0, setpoint = 190; // 190
//double input = 0, kp = .9675, ki = 0.01248075, kd = 0, setpoint = 190; // 190 0.049923
//double input = 0, kp = 18.676056338028169014084507042254, ki = 2.25, kd = 1.25, setpoint = 220; // 75
double input = 0, kp = 30, ki = 0, kd = 0, setpoint = 100;
double output = 0;
unsigned int aTuneLookBack = 30;

boolean tuning = false;
unsigned long  modelTime, serialTime;

double aTuneStep = 120, aTuneNoise = 1, aTuneStartValue = 120;

PID heaterPid(&input, &output, &setpoint, kp, ki, kd, DIRECT);
PID_ATune aTune(&input, &output);

//
double e_steps_mm = (STEPS_REV * 1) * (GREGS_GEAR_RATIO) / (7 * 3.14159);
double layerHeight = .6;
const int stepper_steps_mm = (STEPS_REV * 1) / (2 * 20);

int readingCount = 0;
bool stable = false;

bool xDirection = true; //False = Left, True = Right
bool yDirection = false; //False = Up, True = Down
bool zDirection = true; //False = Forwards, True = Backwards
bool eDirection = false; //False = Extrude, True = Retract

int stepsPerMM = 7 * 16;

int microSecondsPerStep = 200;
int startTime = 0;
int endTime = 0;

float xPosition = 0.0;
float yPosition = 0.0;
float zPosition = 0.0;

int tuningCount = 0;

WSADATA wsa;
SOCKET s;
char message[1024], server_reply[2000];
int recv_size;
struct sockaddr_in server;



void AutoTuneHelper(boolean start);
void SerialSend();
void changeAutoTune();
void InitializePins();
void print_mm(int mm, char axis);
void extrude_mm(int mm, bool shouldStep);
void controlTemp();
void step(char axis);
void controlTemp(void * aArg);
std::string DownloadCommand();

void setup()
{
	WSAStartup(MAKEWORD(2, 2), &wsa);
	s = socket(AF_INET, SOCK_STREAM, 0);

	Log("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		Log("Failed. Error Code : %d", WSAGetLastError());
	}

	Log("Initialised.\n");
	DownloadCommand();
	analogReadResolution(12);
	InitializePins();

	attachInterrupt
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
		serialTime = 0;

	}
	catch (std::exception& e)
	{

	}
	thread t(controlTemp, 0);
	t.detach();

}
void loop()
{
	//print_mm(50, 'X');
	//xDirection = !xDirection;
	//print_mm(50, 'X');
	//xDirection = !xDirection;
}
void G1(std::map<char, float> distances)
{
	char parameterList[5] {'X', 'Y', 'Z', 'E', 'F'};
	/*for (int i = 1; i < 4; i++)
	{
		float valueToCompare = abs(distances[parameterList[i]]);
		int j = i;
		while (j > 0 && abs(distances[parameterList[j - 1]]) > valueToCompare)
		{
			distances[parameterList[j]] = distances[parameterList[j - 1]];
			j = j - 1;
		}
		distances[parameterList[j]] = valueToCompare;
	}
	distances.*/
	std::sort(distances.begin(), distances.end(), distanceComparison);

	for (float i = 0; i < distances[0]; i++)
	{
		if (abs(distances[0]) - i > 1)
		{
			
		}
		else
		{
			
		}
	}
	//for (int i = 0; i < abs(largestValue); i++)
	//{
	//	
	//}
}
void distanceComparison(float i, float j)
{
	return (abs(i) < abs(j));
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

	pinMode(SPI_LATCH, OUTPUT);
	SPI.begin();
	SPI.setBitOrder(MSBFIRST);
	SPI.setClockDivider(SPI_CLOCK_DIV2);
	SPI.setDataMode(SPI_MODE0);
}
//void extrude_mm(int mm, bool shouldStep)
//{
//	int step1Pin = EXTRUDER_STEP;
//	int direction1Pin = EXTRUDER_DIR;
//
//	if (!eDirection)
//		digitalWrite(direction1Pin, LOW);
//	else
//		digitalWrite(direction1Pin, HIGH);
//	double stepsPerMM = 10 / floor(e_steps_mm*layerHeight);
//	double stepCount = 0;
//	int previousStep = 0;
//
//
//
//	for (int miliMeters = 0; miliMeters < mm; miliMeters++)
//	{
//
//		//switch (miliMeters)
//		//{
//		//case 0:
//		//	
//
//		//	break;
//		//case 1:
//
//		//	break;
//		//case 2:
//
//		//	break;
//		//case 3:
//
//		//	break;
//		//}
//
//		//int modifier = 0;
//		//if (microSecondsPerStep - ((endTime - startTime) / floor(e_steps_mm*layerHeight)) > 0)
//		//	modifier = microSecondsPerStep - ((endTime - startTime) / floor(e_steps_mm*layerHeight));
//		//Log("%i\n", startTime - endTime);
//
//		//thread t(controlTemp, 0);
//		//t.detach();
//		for (int j = 0; j < floor(e_steps_mm*layerHeight); j++)
//		{
//
//			//digitalWrite(step1Pin,HIGH);
//			delayMicroseconds(microSecondsPerStep);
//			//digitalWrite(step1Pin,LOW);
//			delayMicroseconds(microSecondsPerStep);
//
//
//
//			stepCount += stepsPerMM;
//
//			if (floor(stepCount) != previousStep)
//			{
//				if (shouldStep)
//					step('X');
//				previousStep = floor(stepCount);
//			}
//		}
//		previousStep = 0;
//		stepCount = 0;
//	}
//
//}
//void step(char axis)
//{
//	int direction2Pin = -1;
//	int direction1Pin = -1;
//	int step1Pin = -1;
//	int step2Pin = -1;
//	switch (axis)
//	{
//	case 'X':
//		step1Pin = X_AXIS_1_STEP;
//		direction1Pin = X_AXIS_1_DIR;
//		if (!xDirection)
//			digitalWrite(direction1Pin, LOW);
//		else
//			digitalWrite(direction1Pin, HIGH);
//		break;
//	case 'Y':
//		step1Pin = Y_AXIS_1_STEP;
//		step2Pin = Y_AXIS_2_STEP;
//		direction1Pin = Y_AXIS_1_DIR;
//		if (!yDirection)
//			digitalWrite(direction1Pin, LOW);
//		else
//			digitalWrite(direction1Pin, HIGH);
//		break;
//	case 'Z':
//		step1Pin = Z_AXIS_1_STEP;
//		step2Pin = Z_AXIS_2_STEP;
//		direction1Pin = Z_AXIS_DIR;
//		if (!zDirection)
//			digitalWrite(direction1Pin, LOW);
//		else
//			digitalWrite(direction1Pin, HIGH);
//		break;
//	}
//	//digitalWrite(direction1Pin, HIGH);
//
//	digitalWrite(step1Pin, HIGH);
//	if (step2Pin != -1) digitalWrite(step2Pin, HIGH);
//	delayMicroseconds(microSecondsPerStep);
//
//	digitalWrite(step1Pin, LOW);
//	if (step2Pin != -1) digitalWrite(step2Pin, LOW);
//	delayMicroseconds(microSecondsPerStep);
//}
void print_mm(int mm, char axis)
{
	int byteSequenceOn = 0;
	int byteSequenceOff = 0;
	switch (axis)
	{
	case 'X':
		if (xDirection)
		{
			byteSequenceOn += X_AXIS_DIR;
			byteSequenceOff += X_AXIS_DIR;
		}
		byteSequenceOn += X_AXIS_STEP;
		break;
	case 'Y':
		if (yDirection)
		{
			byteSequenceOn += Y_AXIS_DIR;
			byteSequenceOff += Y_AXIS_DIR;
		}
		byteSequenceOn += Y_AXIS_STEP;
		break;
	case 'Z':
		if (zDirection)
		{
			byteSequenceOn += Z_AXIS_DIR;
			byteSequenceOff += Z_AXIS_DIR;
		}
		byteSequenceOn += Z_AXIS_STEP;
		break;
	}
	startTime = micros();
	for (int miliMeters = 0; miliMeters < mm; miliMeters++)
	{

		for (int i = 0; i < stepsPerMM; i++)
		{
			digitalWrite(3, LOW);
			SPI.transfer(byteSequenceOn);
			digitalWrite(3, HIGH);

			digitalWrite(3, LOW);
			SPI.transfer(byteSequenceOff);
			digitalWrite(3, HIGH);
			delayMicroseconds(microSecondsPerStep);
		}

	}
	endTime = micros() - startTime;
	//Log("%i", endTime);
}

//void controlTemp(void * aArg)
//{
//	try
//	{
//		while (true)
//		{
//			//if (readingCount >= 100)
//			//{
//			//	readingCount = 0;
//			//	if (kp >= 25)
//			//	{
//			//		InitializePins();
//			//		_exit_arduino_loop();
//			//	}
//			//	double newKp = heaterPid.GetKp() + 1;
//			//	heaterPid.SetTunings(newKp, heaterPid.GetKi(), heaterPid.GetKd());
//			//	temp_reader.BeginNewRecording(web_client, uri_builder(U("/Temperature/AddTemperatureTest")), newKp, ki, kd);
//			//}
//			//unsigned long now = millis();
//			//if (temp_reader.GetReadingSendCount() == 4) readingCount++;
//			//
//			input = temp_reader.GetEndTemp(web_client, uri_builder(U("/Temperature/AddTemperatureTestData")));
//
//
//			if (tuning)
//			{
//				byte val = (aTune.Runtime());
//				if (val != 0)
//				{
//					tuning = false;
//				}
//				if (!tuning)
//				{ //we're done, set the tuning parameters
//					kp = aTune.GetKp();
//					ki = aTune.GetKi();
//					kd = aTune.GetKd();
//					Log(L"Kp: %f", kp);
//					Log(L"Ki: %f", ki);
//					Log(L"Kd: %f\n", kd);
//					heaterPid.SetTunings(kp, ki, kd);
//					AutoTuneHelper(false);
//				}
//			}
//			else
//				heaterPid.Compute();
//
//			//if (input < 250)
//			////{
//			analogWrite(HOT_END_SWITCH, output);
//			analogWrite(HOT_BED_SWITCH, 30);
//			analogWrite(FAN_SWITCH, 0);
//			//}
//			//else
//			//{
//			//	while (input > 50)
//			//	{
//			//		analogWrite(HOT_END_SWITCH, 0);
//			//		analogWrite(FAN_SWITCH, 200);
//			//		input = temp_reader.GetEndTemp(web_client, uri_builder(U("/Temperature/AddTemperatureTestData")));
//			//		Log(L"%f\n", input);
//			//	}
//			//	//_exit_arduino_loop();
//			//}
//			//send-receive with processing if it's time
//			if (millis() > serialTime)
//			{
//				SerialSend();
//				serialTime += 500;
//			}
//		}
//
//
//	}
//	catch (std::exception& e)
//	{
//		Log(L"%s", e);
//	}
//}
void controlTemp(void * aArg)
{
	try
	{
		//uri_builder resource = uri_builder(U("/Temperature/AddTemperatureTestData"));
		//resource.append_query(U("temp"), utility::conversions::to_string_t(std::to_string(input)));
		//resource.append_query(U("testId"), utility::conversions::to_string_t(std::to_string(temp_reader.GetTestId())));



		while (1)
		{
			input = temp_reader.GetEndTemp(web_client, uri_builder(U("/Temperature/AddTemperatureTestData")));
			if (readingCount == 5)
			{
				strcpy(message, "GET /Temperature/AddTemperatureTestData?temp=");
				strcat(message, std::to_string(input).c_str());
				strcat(message, "&testId=");
				strcat(message, std::to_string(temp_reader.GetTestId()).c_str());
				strcat(message, " HTTP/1.1\r\nHost: studypush.me\r\nConnection: close\r\n\r\n");
				//Create a socket
				if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
				{
					Log("Could not create socket : %d", WSAGetLastError());
				}
				Log("Socket created.\n");


				server.sin_addr.s_addr = inet_addr("191.236.192.121");
				server.sin_family = AF_INET;
				server.sin_port = htons(80);

				//Connect to remote server
				if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0)
				{
					Log("connect error");
				}
				Log("Connected");

				//Send some data
				if (send(s, message, strlen(message), 0) < 0)
				{
					Log("Send failed");
				}
				Log("Data Send\n");
				closesocket(s);
				readingCount = 0;
			}
			readingCount++;
			//Receive a reply from the server
			//if ((recv_size = recv(s, server_reply, 2000, 0)) == SOCKET_ERROR)
			//{
			//	Log("recv failed");
			//}

			//Log("Reply received\n");

			//Add a NULL terminating character to make it a proper string before printing
			//server_reply[recv_size -1] = '\0';
			//Log("%s", server_reply);
			//web_client.request(web::http::methods::GET, resource.to_string());
			heaterPid.Compute();
			//Log(L"%i, Temp: %f", output, input);
			analogWrite(HOT_END_SWITCH, output);
			analogWrite(HOT_BED_SWITCH, 0);
			analogWrite(FAN_SWITCH, 0);
			delay(1000);
		}
	}
	catch (std::exception& e)
	{
		Log("Hi");
	}

}
std::string DownloadCommand()
{
	strcpy(message, "GET /GCode/GetNextCommand");
	strcat(message, " HTTP/1.1\r\nHost: studypush.me\r\nConnection: close\r\n\r\n");
	//Create a socket
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		Log("Could not create socket : %d", WSAGetLastError());
	}
	Log("Socket created.\n");

	server.sin_addr.s_addr = inet_addr("191.236.192.121");
	server.sin_family = AF_INET;
	server.sin_port = htons(80);


	//Connect to remote server
	if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		Log("connect error");
	}
	Log("Connected");
	if (send(s, message, strlen(message), 0) < 0)
	{
		Log("Send failed");
	}
	Log("Data Send\n");

	if (recv(s, server_reply, 2000, 0) < 0)
	{
		Log("Download failed");
	}
	std::string finalCommand = std::string(server_reply);

	std::size_t found = finalCommand.find("\r\n\r\n");
	finalCommand.erase(0, found + strlen("\r\n\r\n"));
	if (finalCommand != "The Queue is Empty")
	{
		std::istringstream stream(finalCommand);
		std::string command;
		std::getline(stream, command, ' ');
		if (command == "G1")
		{
			float x= 0, y = 0, z = 0, e = 0, f = 0;
			std::map<char, float> distances;
			while (std::getline(stream, command, ' '))
			{
				distances[command[0]] = atof(command.substr(1).c_str());
				
			}
			G1(distances);
		}
	}
	closesocket(s);

	return std::string(message);
}
