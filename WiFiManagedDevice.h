#pragma once
#include <ArduinoJson.h>;
class WiFiManagedDevice
{
public:
	WiFiManagedDevice();
	void Setup(char* apName, char* mqttServer);
	~WiFiManagedDevice();
	bool Logging;
	void mqttLoop();
	void publish(JsonObject &message, const char* topic);
	void subscribe(const char * topic);
private:
	bool shouldSaveConfig;
	void _log(char * message);
	void _log(String message);
	void saveConfigCallback();
	bool mqttReconnect();
};

