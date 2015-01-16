#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include "tinythread.h"
#include "fast_mutex.h"
#include "ThermistorLookupTable.h"
using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams
using namespace tthread;
class TemperatureReader
{
#define NUMSAMPLES 10
#define ROOMTEMP 25
	int currentTestId;
	float temperaturePin;
	int readingSendCount;
	bool stable;
	double tempHistory[5];
	fast_mutex mutex;
	ThermistorLookupTable sensorTable = ThermistorLookupTable();
public:

	bool IsStable();
	int GetReadingSendCount();
	TemperatureReader(int tempPin);
	void BeginNewRecording(http_client web_client, uri_builder resourceUrl, float newKp, float ki, float kd);
	void AddTemperatureReading(http_client web_client, uri_builder resourceUrl, double temperature);
	float GetEndTemp(http_client web_client, uri_builder resourceUrl);
};