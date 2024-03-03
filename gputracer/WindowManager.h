#pragma once

#include <map>
#include "UI/AbstractWindow.h"
#include "glm/glm.hpp" 

class WindowManager {

public:

	void add(AbstractWindow* window) {
		windows.insert({ window->name, window });
	}

	void remove(AbstractWindow* window) {
		windows.erase(window->name);
	}

	void render() {
		for (const auto& pair : windows) {
			pair.second->render();
		}
	}

private:
	std::map<std::string, AbstractWindow*> windows;



};