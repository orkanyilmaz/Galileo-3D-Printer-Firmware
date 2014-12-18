// Main.cpp : Defines the entry point for the console application.
//
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>

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

http_client web_client(U("http://archos.azurewebsites.net"));
TemperatureReader temp_reader(A1);

void setup()
{
	temp_reader.BeginNewRecording(web_client, uri_builder(U("/Temperature/AddTemperatureTest")));
}
void loop()
{
	temp_reader.GetEndTemp(web_client, uri_builder(U("/Temperature/AddTemperatureTestData")));
	delay(2000);
}
