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
		//Log(L"%s", (utility::conversions::to_utf8string(resourceUrl.to_string())));
	}
}

int TemperatureReader::GetTestId()
{
	return currentTestId;
}
void TemperatureReader::AddTemperatureReading(http_client web_client, uri_builder resourceUrl, double temperature)
{
	try
	{
		delay(200);
		resourceUrl.append_query(U("temp"), utility::conversions::to_string_t(std::to_string(temperature)));
		delay(100);
		resourceUrl.append_query(U("testId"), utility::conversions::to_string_t(std::to_string(currentTestId)));
		delay(100);
		web_client.request(web::http::methods::GET, resourceUrl.to_string());
		delay(800);
	}
	catch (std::exception& e)
	{

	}
}
int TemperatureReader::GetReadingSendCount()
{
	return readingSendCount;
}
bool TemperatureReader::IsStable()
{
	return stable;
}
float TemperatureReader::analog2tempi(int raw, const float& beta, const float& rs, const float& r_inf)
{
	float rawf = (float)(AD_RANGE - raw);
	return ABS_ZERO + beta / log((rawf*rs / (AD_RANGE - rawf)) / r_inf);
}
float TemperatureReader::GetEndTemp(http_client web_client, uri_builder resourceUrl)
{



	//int temp = analogRead(temperaturePin);
	//for (int i = 1; i<18; i++)
	//{
	//	if (sensorTable.temptable[i][0] > temp)
	//	{
	//		int realtemp = sensorTable.temptable[i - 1][1] + (temp - sensorTable.temptable[i - 1][0]) * (sensorTable.temptable[i][1] - sensorTable.temptable[i - 1][1]) / (sensorTable.temptable[i][0] - sensorTable.temptable[i - 1][0]);
	//		realtemp /= 4;
	//		if (realtemp > 255)
	//			realtemp = 255;

	//		temp = realtemp;

	//		break;
	//	}
	//}
	//Calculate real temperature based on lookup table
	//for (int j = 1; j < 18; j++) {
	//	if (pgm_read_word(&(sensorTable.temptable[j][0])) > temp) {
	//		temp = (
	//			//     ((x - x₀)y₁
	//			((uint32_t)temp - pgm_read_word(&(sensorTable.temptable[j - 1][0]))) * pgm_read_word(&(sensorTable.temptable[j][1]))
	//			//                 +
	//			+
	//			//                   (x₁-x)
	//			(pgm_read_word(&(sensorTable.temptable[j][0])) - (uint32_t)temp)
	//			//                         y₀ )
	//			* pgm_read_word(&(sensorTable.temptable[j - 1][1])))
	//			//                              /
	//			/
	//			//                                (x₁ - x₀)
	//			(pgm_read_word(&(sensorTable.temptable[j][0])) - pgm_read_word(&(sensorTable.temptable[j - 1][0])));
	//		break;
	//	}
	//}
	//temp /= 8;
	//double raw = 0;
	//for (int i = 0; i < NUMSAMPLES; i++)
	//{
	//	raw += analogRead(temperaturePin);
	//	delay(10);
	//}
	//raw /= NUMSAMPLES;
	//raw *= 1.75;
	//double hotEndTempAverage = (5 * raw * 100.0) / 1024.0;

	//double hotEndTempAverage = 0;
	//	int hotEndAverage = 0;
	//	int i;
		//for (i = 0; i < NUMSAMPLES; i++) {
			//hotEndAverage += ;
		//	delay(5);
		//}
		//hotEndAverage /= NUMSAMPLES;
		//float hotEndResistance = 0;
	//int value = analogRead(temperaturePin);
	//float temp = analog2tempi(analogRead(temperaturePin), 4097, E_RS, E_R_INF);
		//hotEndResistance = 4095 - hotEndAverage;
		//hotEndResistance *= 4700 / hotEndAverage;
		double voltage = 5-((analogRead(temperaturePin) * 5.0)/ (4095));
		double hotEndResistance = 50000 * (5-voltage) / voltage;
		//0.107692011853806 = 100000 * exp(-4097/298.15)
		float hotEndTemp = (4097 / log(hotEndResistance / 0.107692011853806)) - 273.15;
		//hotEndTemp = log(hotEndTemp);
		//hotEndTemp /= 3974;
		//hotEndTemp += 1.0 / (ROOMTEMP + 273.15);
		//hotEndTemp = 1.0 / hotEndTemp;
		//hotEndTemp -= 273.15;
		//hotEndTempAverage += hotEndTemp;


		//for (int i = 5 - 1; i >= 0; i--)
		//{
		//	tempHistory[i] = tempHistory[i - 1];
		//	if (i == 0)	tempHistory[0] = hotEndTempAverage;
		//}
		//mutex.lock();
		//params.temperature = hotEndTempAverage;

		//thread upload(AddTemperatureReading, &params);
		//upload.detach();
		//readingSendCount++;
		//if (readingSendCount == 5)
		//{
//			AddTemperatureReading(web_client, resourceUrl, hotEndTempAverage);
		//		readingSendCount = 0;
		//}

	//if ((tempHistory[0] + tempHistory[1] + tempHistory[2] + tempHistory[3] + tempHistory[4]) / 5 < 192 && (tempHistory[0] + tempHistory[1] + tempHistory[2] + tempHistory[3] + tempHistory[4]) / 5 > 188 && stable == false)
	//{
	//	mutex.lock();
	//	stable = true;
	//	mutex.unlock();
	//}
		return hotEndTemp*1.75;
}