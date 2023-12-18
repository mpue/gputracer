#pragma once

#include "AWindow.h"

class SettingsWindow : public AWindow {
	
public:
	SettingsWindow() : name("Settings") {};

	void render() override {

	}


private:
	float specStrength = 1.0f;
	float exponent = 64.0;
	float speed = 1.0;

};
