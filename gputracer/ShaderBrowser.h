#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include "imgui/imgui.h"
class ShaderBrowser {
public:
	std::string directory;
	std::vector<std::filesystem::path> shaderFiles;
	std::function<void(const std::filesystem::path&)> onShaderSelected;

	ShaderBrowser(const std::string& dir) : directory(dir) {
		scanDirectory();
	}

	void scanDirectory() {
		shaderFiles.clear();
		for (const auto& entry : std::filesystem::directory_iterator(directory)) {
			if (entry.path().extension() == ".comp") {
				shaderFiles.push_back(entry.path());
			}
		}
	}

	void renderUI() {
		if (ImGui::Begin("Shader Browser")) {
			for (const auto& file : shaderFiles) {
				std::string filename = file.filename().string();
				ImGui::Selectable(filename.c_str());

				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					if (onShaderSelected) {
						onShaderSelected(file);
					}
				}
			}

			ImGui::End();

		}

	}

};
