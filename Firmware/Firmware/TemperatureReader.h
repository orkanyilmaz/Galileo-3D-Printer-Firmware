#include <cpprest/http_client.h>
#include <cpprest/filestream.h>

using namespace utility;                    // Common utilities like string conversions
using namespace web;                        // Common features like URIs.
using namespace web::http;                  // Common HTTP functionality
using namespace web::http::client;          // HTTP client features
using namespace concurrency::streams;       // Asynchronous streams

class TemperatureReader
{
	#define NUMSAMPLES 10
	#define ROOMTEMP 25

	int currentTestId;
	float temperaturePin;
	int readingSendCount;
	public:
		int GetReadingSendCount();
		TemperatureReader(int tempPin);
		void BeginNewRecording(http_client web_client, uri_builder resourceUrl, float newKp, float ki, float kd);
		void AddTemperatureReading(http_client web_client, uri_builder resourceUrl, double temperature);
		float GetEndTemp(http_client web_client, uri_builder resourceUrl);
};