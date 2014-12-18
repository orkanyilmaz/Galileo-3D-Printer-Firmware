#include "TemperatureReader.h"
#include "stdafx.h"
#include "arduino.h"

TemperatureReader::TemperatureReader(int tempPin)
{
	temperaturePin = tempPin;
}
void TemperatureReader::BeginNewRecording(http_client web_client, uri_builder resourceUrl)
{
	try
	{
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

float TemperatureReader::GetEndTemp(http_client web_client, uri_builder resourceUrl)
{
	float hotEndTempAverage = 0;
	for (int j = 0; j < NUMSAMPLES; j++)
	{
		float hotEndAverage = 0;
		int i;
		for (i = 0; i < NUMSAMPLES; i++) {
			hotEndAverage += analogRead(temperaturePin);
			delay(10);
		}
		hotEndAverage /= NUMSAMPLES;
		hotEndAverage *= 1;
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

	AddTemperatureReading(web_client, resourceUrl, hotEndTempAverage);

	return hotEndTempAverage;
}