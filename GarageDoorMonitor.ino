/*
    Name:       GarageDoorMonitor.ino
    Created:	8/25/2018 6:06:34 PM
    Author:     Spencer Kittleson
*/

// TODO close garage door if opened too long. buzzer.  pin is opened.
// TODO led indicator whether door is opened or not
// TODO open/close door and turn on/off light using relay
// TODO non-blocking mqtt connection, reconnection, and loop
// TODO proper mqtt messages
// TODO 

#include <FS.h>
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <PubSubClient.h>         //https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino
#include <ArduinoJson.h>

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
char mqttServer[40];
const int mqttPort = 1883;
const int garageDoorRelayPin = 2;
const int garageDoorReedSwitchPin = 4;
const int garageLightRelayPin = 3;
const int garageDoorLedIndicatorPin = 5;
const int garageDoorOverridePin = 6;
const char* relayStatus = "status";
const char* relayOn = "true";
const char* relayOff = "false";
const char* topic = "home/garage-door";
const char* topicEvent = "home/garage-door/event";
bool shouldSaveConfig = false;

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
	//if (strcmp(incomingTopic, topic) != 0) { return; }
	int pinToFire = 0;
	if (strcmp(incomingTopic, "light") == 0) {
		pinToFire = garageLightRelayPin;
	} else if (strcmp(incomingTopic, "garage-door") == 0) {
		pinToFire = garageDoorRelayPin;
	} else if (strcmp(incomingTopic, "alarm") == 0) {
	 pinToFire = garageDoorRelayPin;
 }

	char* payloadValue = byteArrayIntoCharArray(payload, length);

	//Allocate for json response
	DynamicJsonBuffer jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();
	root["event"] = "relay";
	root["topic"] = topic;
	root["msg"] = "";
	root["uptime"] = millis();

	
	


	if (strcmp(payloadValue, relayStatus) == 0) {
		root[relayStatus] = !((bool)digitalRead(garageDoorRelayPin));
	}
	else if (strcmp(payloadValue, relayOn) == 0) {
		digitalWrite(garageDoorRelayPin, 0);
		root[relayStatus] = true;
	}
	else if (strcmp(payloadValue, relayOff) == 0) {
		digitalWrite(garageDoorRelayPin, 1);
		root[relayStatus] = false;
	}
	else {
		root["msg"] = "unknown payload";
	}
	String payloadJson;
	root.printTo(payloadJson);
	Serial.println(payloadJson);
	mqttClient.publish(topicEvent, payloadJson.c_str());
}

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

/**
* Reconnect mqtt client
*/
void reconnect() {
	while (!mqttClient.connected()) {
		Serial.print("Attempting MQTT connection...");
		if (mqttClient.connect("IoT Client")) {
			Serial.println("connected");
			mqttClient.subscribe(topic);
		}
		else {
			Serial.print("failed, rc=");
			Serial.print(mqttClient.state());
			Serial.println(" try again in 5 seconds");
			delay(5 * 1000);
		}
	}
}

void isGarageDoorOpened() {
	// is it opened?
	// 

}

void setup() {
	Serial.begin(115200);
	SPIFFS.begin();

	//Default the primary relay off
	pinMode(garageDoorRelayPin, OUTPUT);
	pinMode(garageDoorReedSwitchPin, INPUT);
	digitalWrite(garageDoorRelayPin, 1);
	delay(500);
	WiFiManager wifiManager;
	wifiManager.setSaveConfigCallback( []() { shouldSaveConfig = true;  });
	//wifiManager.resetSettings();
	WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqttServer, 40);
	wifiManager.addParameter(&custom_mqtt_server);
	wifiManager.autoConnect("GarageDoorMonitorAp");

	//save the custom parameters to FS
	if (shouldSaveConfig) {
		strcpy(mqttServer, custom_mqtt_server.getValue());
		saveConfig();
	}
	loadConfig();
	mqttClient.setServer(mqttServer, mqttPort);
	mqttClient.setCallback(callbackMessage);
	Serial.println("Connected");
	SPIFFS.end();
}

void loop() {
	if (!mqttClient.connected()) {
		reconnect();
	}
	mqttClient.loop();

	// check pin
	
}
