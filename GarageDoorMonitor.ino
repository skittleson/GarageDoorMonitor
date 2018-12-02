/*
	Name:       GarageDoorMonitor.ino
	Created:	8/25/2018 6:06:34 PM
	Author:     Spencer Kittleson
*/

// TODO move publish garage events in main file
// TODO have callbacks for mqtt messages
// TODO mqtt message on door,light open/close (optional)
// TODO mqtt with user/password
// https://wiki.wemos.cc/products:d1:d1_mini_lite


#include <PubSubClient.h>
#include "GarageDoorMonitorLogic.h"

int garageDoorRelayPin = 5; // D1 - GPIO5
int garageLightRelayPin = 4; // D2 - GPIO4
int garageDoorReedSwitchPin = 13; // D7 - GPIO13
const int garageDoorLedIndicatorPin = 12; // D6 = GPIO12
const int garageDoorOverridePin = 14; // D5 = GPIO14 this is a pull up only
GarageDoorMonitorLogic monitor(garageDoorOverridePin, garageDoorLedIndicatorPin, garageDoorReedSwitchPin);
// WiFiManagedDevice managed;

void setup() {
	Serial.begin(115200);
	monitor.maxTimeOpenInMinutes = 1;
	monitor.callbackTimeoutSecs = 45;
	monitor.Logging = true;

	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(garageDoorRelayPin, OUTPUT);
	pinMode(garageLightRelayPin, OUTPUT);
	pinMode(garageDoorLedIndicatorPin, OUTPUT);
	pinMode(garageDoorOverridePin, INPUT);
	pinMode(garageDoorReedSwitchPin, INPUT);

	// Default states
	digitalWrite(garageDoorRelayPin, 1);
	digitalWrite(garageLightRelayPin, 1);
	
	monitor.callbackGarageDoorTiggerShut([]() {
		RelaySimulateButtonPress(garageDoorRelayPin);
	});
	digitalWrite(LED_BUILTIN, LOW);
	// managed = WiFiManagedDevice();
}

// the loop function runs over and over again forever
void loop() {
	delay(1000);                       // wait for a second
	monitor.loop();
	//managed.mqttLoop();
}


void RelaySimulateButtonPress(int pin) {
	digitalWrite(pin, 0);
	delay(500);
	digitalWrite(pin, 1);
}