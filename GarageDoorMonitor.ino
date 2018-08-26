/*
	Name:       GarageDoorMonitor.ino
	Created:	8/25/2018 6:06:34 PM
	Author:     Spencer Kittleson
*/

// DONE run without mqtt as well
// TODO mqtt message on door,light open/close (optional)
// TODO ArduinoJson is complaining about the latest version
// TODO mqtt with user/password (optional)

#include <FS.h>
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <PubSubClient.h>         //https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino
#include <ArduinoJson.h>

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
const int mqttPort = 1883;
const int garageDoorRelayPin = 1;
const int garageDoorReedSwitchPin = 2;
const int garageLightRelayPin = 3;
const int garageDoorLedIndicatorPin = 4;
const int garageDoorOverridePin = 5;
const int garageDoorMaxOpenMins = 15;
const char* relayOn = "true";
const char* relayOff = "false";
const char* topicGarageOpener = "home/garage/opener";
const char* topicGarageOpenerLight = "home/garage/opener/light";
const char* topicGarageOpenerDoor = "home/garage/opener/door";
const char* topicGarageOpenerStatus = "home/garage/opener/status";

long garageOpenedAt = 0;
bool shouldSaveConfig = false;
char mqttServer[40];
bool isMqtt = false;
long mqttLastReconnectAttempt = 0;

void setup() {
	Serial.begin(115200);

	//Default the primary relay off
	pinMode(garageDoorRelayPin, OUTPUT);
	pinMode(garageDoorReedSwitchPin, INPUT);
	pinMode(garageDoorLedIndicatorPin, OUTPUT);
	digitalWrite(garageDoorRelayPin, 1);
	delay(500);
	setupWifi();
	if (isMqtt) {
		mqttClient.setServer(mqttServer, mqttPort);
		mqttClient.setCallback(callbackMessage);
	}
}

void loop() {
	if (isMqtt) {
		mqttLoop();
	}
	isGarageDoorOpened();
	delay(1);
}

void isGarageDoorOpened() {
	if (digitalRead(garageDoorReedSwitchPin) == HIGH) {
		digitalWrite(garageDoorLedIndicatorPin, LOW);
		garageOpenedAt = 0;
	}
	else {
		digitalWrite(garageDoorLedIndicatorPin, HIGH);

		// Physical override if the door is to remain open for longer
		if (digitalRead(garageDoorOverridePin) == HIGH) {
			// Blink a bunch of times to let someone know to flip the switch back
			for (int i = 0; i <= 5; i++) {
				digitalWrite(garageDoorLedIndicatorPin, LOW);
				delay(2000);
				digitalWrite(garageDoorLedIndicatorPin, HIGH);
			}
		}
		else {
			// If the garage door has been opened for X amount minutes then close the door and reset the clock
			long now = millis();
			long maxOpenMillis = (garageDoorMaxOpenMins * 60 * 1000);
			if (now - garageOpenedAt > maxOpenMillis) {
				garageOpenedAt = 0;
				RelaySimulateButtonPress(garageDoorRelayPin);
				if (isMqtt) {

				}
			}
		}
	}
}

/*
Setup access point and special configurations
*/
void setupWifi() {
	SPIFFS.begin();
	WiFiManager wifiManager;
	wifiManager.setSaveConfigCallback([]() { shouldSaveConfig = true; });
	//wifiManager.resetSettings();
	WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqttServer, 40);
	wifiManager.addParameter(&custom_mqtt_server);
	wifiManager.autoConnect("GarageDoorMonitorAp");

	//save the custom parameters to FS
	if (shouldSaveConfig) {
		strcpy(mqttServer, custom_mqtt_server.getValue());
		String mqttServerStr(mqttServer);
		isMqtt = (mqttServerStr.length() > 0);
		saveConfig();
	}
	loadConfig();
	Serial.print("Connected to ");
	Serial.println(WiFi.SSID());
	Serial.print("IP address:\t");
	Serial.println(WiFi.localIP());
	Serial.print("Is Mqtt?:\t");
	Serial.println(isMqtt);
	SPIFFS.end();
}

#pragma region Config Methods

/**
* Save values that need to persist a reboot
*/
bool loadConfig() {
	File configFile = SPIFFS.open("/config.json", "r");
	if (!configFile) {
		Serial.println("Failed to open config file");
		return false;
	}
	size_t size = configFile.size();
	if (size > 1024) {
		Serial.println("Config file size is too large");
		return false;
	}

	// Allocate a buffer to store contents of the file.
	std::unique_ptr<char[]> buf(new char[size]);

	// We don't use String here because ArduinoJson library requires the input
	// buffer to be mutable. If you don't use ArduinoJson, you may as well
	// use configFile.readString instead.
	configFile.readBytes(buf.get(), size);
	StaticJsonBuffer<200> jsonBuffer;
	JsonObject& json = jsonBuffer.parseObject(buf.get());
	if (!json.success()) {
		Serial.println("Failed to parse config file");
		return false;
	}
	String mqtt = json["mqtt"];
	mqtt.toCharArray(mqttServer, 40);
	Serial.println(mqttServer);
	return true;
}

/**
* Save config settings that can be used later.
*/
bool saveConfig() {
	StaticJsonBuffer<200> jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();
	json["mqtt"] = mqttServer;
	File configFile = SPIFFS.open("/config.json", "w");
	if (!configFile) {
		Serial.println("Failed to open config file for writing");
		return false;
	}
	json.printTo(configFile);
	return true;
}
#pragma endregion

#pragma region Mqtt

/**
* Convert byte array into char array.
*/
char* byteArrayIntoCharArray(byte* bytes, unsigned int length) {
	char* data = (char*)bytes;
	data[length] = NULL;
	return data;
}

/**
* Callback handler for MQTT
*/
void callbackMessage(char* incomingTopic, byte* payload, unsigned int length) {
	//Only topic defined should be processed
	int pinMapping = 0;
	if (strcmp(incomingTopic, topicGarageOpenerLight) == 0) {
		pinMapping = garageLightRelayPin;
	}
	else if (strcmp(incomingTopic, topicGarageOpenerDoor) == 0) {
		pinMapping = garageDoorRelayPin;
	}
	else {
		return;
	}

	char* payloadValue = byteArrayIntoCharArray(payload, length);

	StaticJsonBuffer<400> jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();
	root["topic"] = incomingTopic;
	root["msg"] = "";
	root["uptime"] = millis();

	// Simulate a button press using a relay
	if (strcmp(payloadValue, relayOn) == 0) {
		RelaySimulateButtonPress(pinMapping);
	}
	else if (strcmp(payloadValue, relayOff) == 0) {
		digitalWrite(pinMapping, 1);
	}
	else {
		root["msg"] = "unknown payload";
	}
	String payloadJson;
	root.printTo(payloadJson);
	Serial.println(payloadJson);
	mqttClient.publish(topicGarageOpenerStatus, payloadJson.c_str());
}

boolean mqttReconnect() {
	if (mqttClient.connect("GarageDoorOpener")) {
		StaticJsonBuffer<400> jsonBuffer;
		JsonObject& root = jsonBuffer.createObject();
		root["connected"] = true;
		root["wifi-ssid"] = WiFi.SSID();
		root["ip-address"] = WiFi.localIP();
		String payloadJson;
		root.printTo(payloadJson);
		mqttClient.publish(topicGarageOpener, payloadJson.c_str());
		mqttClient.subscribe(topicGarageOpenerLight);
		mqttClient.subscribe(topicGarageOpenerDoor);
	}
	return mqttClient.connected();
}

void mqttLoop()
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
#pragma endregion

void RelaySimulateButtonPress(int pin) {
	digitalWrite(pin, 0);
	delay(500);
	digitalWrite(pin, 1);
}
