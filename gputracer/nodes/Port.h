#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../imgui/imgui.h" // Assuming ImVec2 is defined here

class Port {
public:
    std::string Name;
    ImVec2 Position;
    std::vector<std::shared_ptr<Port>> OutputConnections;
    std::shared_ptr<Port> InputConnection;
    void* Value;

    Port() : Value(nullptr) {
        OutputConnections = std::vector<std::shared_ptr<Port>>();
    }

    Port(const std::string& name) : Name(name), Value(nullptr) {
        OutputConnections = std::vector<std::shared_ptr<Port>>();
    }

    virtual bool AcceptsConnection(const std::shared_ptr<Port>& port) const = 0;

    virtual ~Port() = default;
};
