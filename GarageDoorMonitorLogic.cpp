#include "GarageDoorMonitorLogic.h"
#include "Arduino.h"

GarageDoorMonitorLogic::GarageDoorMonitorLogic(int overrideButtonPin, int ledIndicatorPin, int reedSwitchPin)
{
	this->_overrideButtonPin = overrideButtonPin;
	this->_ledStatusPin = ledIndicatorPin;
	this->_garageDoorReedSwitchPin = reedSwitchPin;
	this->_lastOpened = 0;
	this->maxTimeOpenInMinutes = 15;
	this->callbackTimeoutSecs = 45;
}

GarageDoorMonitorLogic::~GarageDoorMonitorLogic() {
}

bool GarageDoorMonitorLogic::isButtonOverrideEnabled() {
	return !digitalRead(this->_overrideButtonPin) == HIGH;
}

bool GarageDoorMonitorLogic::isGarageDoorClosed() {
	return digitalRead(this->_garageDoorReedSwitchPin) == HIGH;
}

void GarageDoorMonitorLogic::setStatusIndicator(bool enable) {
	if (enable) {
		this->_log("led status on");
		digitalWrite(this->_ledStatusPin, HIGH);
	}
	else {
		this->_log("led status off");
		digitalWrite(this->_ledStatusPin, LOW);
	}
}

/// <summary>
/// Sets a callback notification to trigger a garage door shut.
/// </summary>
void GarageDoorMonitorLogic::callbackGarageDoorTiggerShut(void(*garageDoorToggleCallback)()) {
	this->_garageDoorToggleCallback = garageDoorToggleCallback;
}

void GarageDoorMonitorLogic::_log(char* message) {
	if (this->Logging) {
		Serial.println(message);
	}
}

/// <summary>
/// A garage door and override button check for controller loop.  Triggers callback if garage door has been opened for too long.
/// </summary>
void GarageDoorMonitorLogic::loop() {
	if (this->isButtonOverrideEnabled()) {
		this->_log("Garage button override on");
		// Blink on loop
		this->setStatusIndicator(true);
		delay(1000);
		this->setStatusIndicator(false);
		delay(500);
	}
	else {
		if (this->isGarageDoorClosed()) {
			this->_log("Garage Closed");
			this->setStatusIndicator(false);
			this->_lastOpened = 0;
		}
		else {
			this->_log("Garage open");
			this->setStatusIndicator(true);
			// If the garage door has been opened for X amount minutes then close the door and reset the clock
			long now = millis();
			long maxOpenMillis = (this->maxTimeOpenInMinutes * 60 * 1000);
			if (now - this->_lastOpened > maxOpenMillis) {
				this->_log("Garage door trigger callback");
				this->_garageDoorToggleCallback();
				delay(this->callbackTimeoutSecs * 1000);
				this->_lastOpened = 0;
				this->_log("Garage door should be shut by now.");
			}
		}
	}
}