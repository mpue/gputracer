#pragma once

#include <string>
#include "../imgui/imgui.h"

class AWindow {

public:
	
	AWindow() : open(false) {}

	AWindow(std::string name) {
		this->name = name;
	}

	virtual ~AWindow() {};

	virtual void render() = 0;

	bool isOpen() {
		return open;
	}


	void close() {
		open = false;
	}

protected:
	bool open = false;
	std::string name;
	void renderInternal() {
		if (open) {
			if (!ImGui::Begin(name.c_str(), &open)) {
				ImGui::End();
			}
			else {
				render();
				ImGui::End();
			}
		}

	}

};