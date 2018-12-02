#pragma once

class GarageDoorMonitorLogic
{
public:
	~GarageDoorMonitorLogic();
	GarageDoorMonitorLogic(int overrideButtonPin, int ledIndicatorPin, int reedSwitchPin);
	bool isButtonOverrideEnabled();
	bool isGarageDoorClosed();
	void setStatusIndicator(bool enable);
	bool Logging;
	int maxTimeOpenInMinutes;
	int callbackTimeoutSecs;
	void callbackGarageDoorTiggerShut(void(*garageDoorToggleCallback)());
	void loop();
private:
	void _log(char * message);
	int _overrideButtonPin;
	int _ledStatusPin;
	int _garageDoorReedSwitchPin;
	long _lastOpened;
	void(*_garageDoorToggleCallback)();
};