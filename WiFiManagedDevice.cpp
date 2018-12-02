#include "WiFiManagedDevice.h"
#include <PubSubClient.h>;
#include <ArduinoJson.h>;
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
// #include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
// #include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
// #include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
const int mqttPort = 1883;
long mqttLastReconnectAttempt = 0;

const char* topicGarageOpener = "home/garage/opener";
const char* topicGarageOpenerLight = "home/garage/opener/light";
const char* topicGarageOpenerDoor = "home/garage/opener/door";
const char* topicGarageOpenerStatus = "home/garage/opener/status";

WiFiManagedDevice::WiFiManagedDevice()
{
	
}

void WiFiManagedDevice::_log(char* message) {
	if (this->Logging) {
		Serial.println(message);
	}
}

void WiFiManagedDevice::_log(String message)
{
	if (this->Logging) {
		Serial.println(message);
	}
}

void WiFiManagedDevice::Setup(char* apName, char* mqttServer) {
	WiFiManager wifiManager;
	// wifiManager.setSaveConfigCallback(this->saveConfigCallback);
	wifiManager.resetSettings();
	WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqttServer, 40);
	wifiManager.addParameter(&custom_mqtt_server);
	wifiManager.autoConnect(apName);
	this->_log("Connected to ");
	this->_log(WiFi.SSID());
	this->_log("IP address:\t");
	this->_log(WiFi.localIP().toString());
}

void WiFiManagedDevice::saveConfigCallback() {
	this->shouldSaveConfig = true;
}

bool WiFiManagedDevice::mqttReconnect() {
	if (mqttClient.connect("GarageDoorOpener")) {
		StaticJsonBuffer<200> jsonBuffer;

		JsonObject& root = jsonBuffer.createObject();
		root["connected"] = true;
		this->publish(root, topicGarageOpener);
		mqttClient.subscribe(topicGarageOpenerLight);
		mqttClient.subscribe(topicGarageOpenerDoor);
	}
	return mqttClient.connected();
}

void WiFiManagedDevice::publish(JsonObject &message, const char* topic) {
	if (mqttClient.connected()) {
		message["time"] = millis();
		message["wifi-ssid"] = WiFi.SSID();
		message["ip-address"] = WiFi.localIP().toString();
		String payloadJson;
		message.printTo(payloadJson);
		this->_log(payloadJson);
		mqttClient.publish(topic, payloadJson.c_str());
	}
	else {
		this->_log("Unable to publish due to connection loss");
	}	
}

void WiFiManagedDevice::subscribe(const char* topic) {
	if (mqttClient.connected()) {
		mqttClient.subscribe(topic);
	}	
}

void WiFiManagedDevice::mqttLoop()
{
	if (!mqttClient.connected()) {
		long now = millis();
		if (now - mqttLastReconnectAttempt > 5000) {
			mqttLastReconnectAttempt = now;
			// Attempt to reconnect
			if (mqttReconnect()) {
				mqttLastReconnectAttempt = 0;
			}
		}
	}
	else {
		mqttClient.loop();
	}
}

WiFiManagedDevice::~WiFiManagedDevice()
{
}
