#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../imgui/imgui.h"
#include <uuid.h>
#include "../Color.h" // Assuming Color is defined in this header
#include "Port.h"  // Assuming Port is defined in this header
#include <type_traits>
#include <cmath>
#include <sstream>
#include <locale>
#include <codecvt>
#include <typeindex>

class Node {
private:
    uuid guid;

public:
    std::vector<std::shared_ptr<Port>> Inputs;
    std::vector<std::shared_ptr<Port>> Outputs;
    std::string Name;
    ImVec2 Position;
    ImVec2 Size;
    bool IsSelected = false;

    ImVec2 delta;
    ImVec2 dragStartPos;
    ImVec2 min;
    ImVec2 max;

    bool IsDragging = false;

    Node() {
        uuid_generate(guid);
    }

    Node(const std::string& name, ImVec2 size, ImVec2 position) : Name(name), Size(size), Position(position) {
        uuid_generate(guid);
    }

    Node(const std::string& name, ImVec2 position) : Name(name), Size(ImVec2(160, 200)), Position(position) {
        uuid_generate(guid);
    }

    virtual void Process(float deltaTime) = 0;
    virtual void RenderContent() = 0;

    void Render() {
        ImGuiWindowFlags flags = ImGuiWindowFlags_None;

        CheckForDrag();
        if (IsSelected) {
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.5f, 1.0f, 0.5f));
                
                Color::FromArgb(255, 100, 255, 100).ToUIntArgb());
        }
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Color::FromArgb(255, 100, 100, 100).ToUIntArgb());
        ImGui::GetStyle().ChildRounding = 10.0f;
        ImGui::BeginChild(GetGUIDString().c_str(), Size, true, flags);
        ImGui::NewLine();
        ImGui::NewLine();
        RenderContent();
        ImGui::EndChild();
        ImGui::PopStyleColor();
        if (IsSelected)
            ImGui::PopStyleColor();
        RenderPorts();

        ImDrawList* imDrawListPtr = ImGui::GetWindowDrawList();
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 vPos(windowPos.x + Position.x, windowPos.y + Position.y);
        imDrawListPtr->AddLine(vPos + ImVec2(0, 30) + delta, vPos + ImVec2(Size.x, 30) + delta, Color::FromArgb(255, 100, 100, 100).ToUIntArgb());
        imDrawListPtr->AddText(vPos + ImVec2(10, 10) + delta, Color::White.ToUIntArgb(), Name.c_str());
    }

    ImVec2 getMinRect() const {
        return min;
    }

    ImVec2 getMaxRect() const {
        return max;
    }

private:
    void RenderPorts() {
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImDrawList* imDrawListPtr = ImGui::GetWindowDrawList();

        int outputIndex = 0;
        int inputIndex = 0;
        float portSize = 8.0f;
        ImVec2 vPos(windowPos.x + Position.x, windowPos.y + Position.y);
        for (const auto& outPort : Outputs) {
            float y = (Size.y / Outputs.size()) * (outputIndex + 1) - (Size.y / Outputs.size()) / 2;
            ImVec2 outportPos = vPos + ImVec2(Size.x - 2 + portSize, y) + delta;

            outPort->Position = ImVec2(outportPos.x, outportPos.y);
            imDrawListPtr->AddText(ImVec2(outportPos.x - 30, outportPos.y - 10), Color::White.ToUIntArgb(), outPort->Name.c_str());
            if (ImGui::IsMouseHoveringRect(outportPos - ImVec2(portSize / 2, portSize / 2), outportPos + ImVec2(portSize / 2, portSize / 2))) {
                imDrawListPtr->AddCircleFilled(outportPos, portSize, (Color::Yellow.ToUIntArgb());
            }
            else {
                uint32_t portCol = Color::Yellow.ToUIntArgb();

                if (dynamic_cast<FloatPort*>(outPort.get())) {
                    portCol = Color::Violet.ToUIntArgb();
                }
                else if (dynamic_cast<EntityPort*>(outPort.get())) {
                    portCol = Color::Blue.ToUIntArgb();
                }
                else if (dynamic_cast<Vector3Port*>(outPort.get())) {
                    portCol = Color::Orange.ToUIntArgb();
                }
                else {
                    portCol = Color::Green.ToUIntArgb();
                }

                imDrawListPtr->AddCircleFilled(outportPos, portSize, portCol);
                imDrawListPtr->AddCircle(outportPos, portSize, Color::Black.ToUIntArgb());
            }
            outputIndex++;
        }

        for (const auto& inPort : Inputs) {
            float y = (Size.y / Inputs.size()) * (inputIndex + 1) - (Size.y / Inputs.size()) / 2;
            vPos = ImVec2(windowPos.x + Position.x, windowPos.y + Position.y);
            ImVec2 inportPos = vPos + ImVec2(-portSize + 2, y) + delta;

            inPort->Position = ImVec2(inportPos.x, inportPos.y);
            imDrawListPtr->AddText(ImVec2(inportPos.x + 20, inportPos.y - 10), Color::White.ToUIntArgb(), inPort->Name.c_str());
            if (ImGui::IsMouseHoveringRect(inportPos - ImVec2(portSize / 2, portSize / 2), inportPos + ImVec2(portSize / 2, portSize / 2))) {
                imDrawListPtr->AddCircleFilled(inportPos, 10, Color::Yellow.ToUIntArgb());
            }
            else {
                uint32_t portCol = Color::Yellow.ToUIntArgb();

                if (dynamic_cast<FloatPort*>(inPort.get())) {
                    portCol = Color::Violet.ToUIntArgb();
                }
                else if (dynamic_cast<EntityPort*>(inPort.get())) {
                    portCol = Color::Blue.ToUIntArgb();
                }
                else if (dynamic_cast<Vector3Port*>(inPort.get())) {
                    portCol = Color::Orange.ToUIntArgb();
                }
                imDrawListPtr->AddCircleFilled(inportPos, portSize, portCol);
                imDrawListPtr->AddCircle(inportPos, portSize, Color::Black.ToUIntArgb());
            }
            inputIndex++;
        }
    }

    void CheckForDrag() {
        if (IsDragging && IsSelected) {
            delta = ImGui::GetMouseDragDelta();
            ImGui::SetCursorPos(dragStartPos + delta);
        }
        else {
            ImGui::SetCursorPos(ImVec2(Position.x, Position.y));
        }
        min = ImVec2(ImGui::GetWindowPos().x + Position.x, ImGui::GetWindowPos().y + Position.y);
        max = ImVec2(ImGui::GetWindowPos().x + Position.x + Size.x, ImGui::GetWindowPos().y + Position.y + Size.y);

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            if (ImGui::IsMouseHoveringRect(min, max)) {
                dragStartPos = Position;
                IsDragging = true;
            }
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            if (IsDragging) {
                Position = ImVec2(dragStartPos.x + delta.x, dragStartPos.y + delta.y);
                delta = ImVec2(0, 0);
                IsDragging = false;
            }
        }
    }

    std::string GetGUIDString() const {
        char buffer[37];
        uuid_unparse_lower(guid, buffer);
        return std::string(buffer);
    }

public:
    static std::vector<std::type_index> GetNodeTypes(const std::vector<std::type_index>& types) {
        std::vector<std::type_index> nodeTypes;
        for (const auto& type : types) {
            if (std::is_base_of<Node, std::type_index::type)) {
                nodeTypes.push_back(type);
            }
        }
        return nodeTypes;
    }

    static bool IsNumeric(const std::string& str) {
        std::istringstream iss(str);
        double number;
        iss.imbue(std::locale("C"));
        iss >> std::noskipws >> number;
        return iss.eof() && !iss.fail();
    }
};
