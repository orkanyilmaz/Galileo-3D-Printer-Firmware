#include "TemperatureReader.h"
#include "stdafx.h"
#include "arduino.h"

TemperatureReader::TemperatureReader(int tempPin)
{
	temperaturePin = tempPin;
}
void TemperatureReader::BeginNewRecording(http_client web_client, uri_builder resourceUrl, float newKp, float ki, float kd)
{
	try
	{

		resourceUrl.append_query(U("kp"), utility::conversions::to_string_t(std::to_string(newKp)));
		resourceUrl.append_query(U("ki"), utility::conversions::to_string_t(std::to_string(ki)));
		resourceUrl.append_query(U("kd"), utility::conversions::to_string_t(std::to_string(kd)));
		pplx::task<string_t> task = web_client.request(web::http::methods::GET, resourceUrl.to_string()).then([=](web::http::http_response response)
		{
			return response.extract_string();
		});
		task.wait();
		currentTestId = atoi(utility::conversions::to_utf8string(task.get()).c_str());
	}
	catch (std::exception& e)
	{

	}
}
void TemperatureReader::AddTemperatureReading(http_client web_client, uri_builder resourceUrl, double temperature)
{
	resourceUrl.append_query(U("temp"), utility::conversions::to_string_t(std::to_string(temperature)));
	resourceUrl.append_query(U("testId"), utility::conversions::to_string_t(std::to_string(currentTestId)));
	pplx::task<string_t> task = web_client.request(web::http::methods::GET, resourceUrl.to_string()).then([=](web::http::http_response response)
	{
		return response.extract_string();
	});
	task.wait();
}
int TemperatureReader::GetReadingSendCount()
{
	return readingSendCount;
}
bool TemperatureReader::IsStable()
{
	return stable;
}
float TemperatureReader::GetEndTemp(http_client web_client, uri_builder resourceUrl)
{
	float hotEndTempAverage = 0;
	for (int j = 0; j < NUMSAMPLES; j++)
	{
		int hotEndAverage = 0;
		int i;
		for (i = 0; i < NUMSAMPLES; i++) {
			hotEndAverage += analogRead(temperaturePin);
			delay(5);
		}
		hotEndAverage /= NUMSAMPLES;
		hotEndAverage *= 6;
		if (hotEndAverage > 1023)
		{

			//Log(L"Analog Raeding: %i\n", hotEndAverage);
			hotEndAverage = 1022;
		}
		float hotEndResistance = 0;

		hotEndResistance = 1023 - hotEndAverage;
		hotEndResistance *= 100000 / hotEndAverage;

		float hotEndTemp = hotEndResistance / 100000;
		hotEndTemp = log(hotEndTemp);
		hotEndTemp /= 3974;
		hotEndTemp += 1.0 / (ROOMTEMP + 273.15);
		hotEndTemp = 1.0 / hotEndTemp;
		hotEndTemp -= 273.15;
		hotEndTempAverage += hotEndTemp;
	}
	hotEndTempAverage /= NUMSAMPLES;

	readingSendCount++;
	if (readingSendCount == 5)
	{
		for (int i = 5 - 1; i >= 0; i--)
		{
			tempHistory[i] = tempHistory[i - 1];
			if (i == 0)	tempHistory[0] = hotEndTempAverage;
		}


		AddTemperatureReading(web_client, resourceUrl, hotEndTempAverage);
		readingSendCount = 0;
	}


	if ((tempHistory[0] + tempHistory[1] + tempHistory[2] + tempHistory[3] + tempHistory[4]) / 5 < 192 && (tempHistory[0] + tempHistory[1] + tempHistory[2] + tempHistory[3] + tempHistory[4]) / 5 > 188 && stable == false)
	{
		mutex.lock();
		stable = true;
		mutex.unlock();
	}
	return hotEndTempAverage;
}