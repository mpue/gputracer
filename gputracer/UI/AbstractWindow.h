#pragma once

#include <string>
#include "../imgui/imgui.h"
#include "glm/glm.hpp"

class AbstractWindow {

public:

	AbstractWindow() : open(false),size(glm::vec2(100,100)){}

	AbstractWindow(std::string name, glm::vec2 size) {
		this->name = name;
		this->size = size;
	}

	virtual ~AbstractWindow() {};

	virtual void render() = 0;

	bool isOpen() {
		return open;
	}


	void close() {
		open = false;
	}

	bool open = false;
	glm::vec2 size;
	std::string name;

protected:
	void renderInternal() {
		if (open) {
			ImGui::SetNextWindowSize(ImVec2(this->size.x, this->size.y));
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