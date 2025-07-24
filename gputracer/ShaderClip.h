#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <map>

#include "ComputeShader.h"

// Einfacher Keyframe mit Zeit + Wert
struct Keyframe {
    float time = 0.0f;
    float value = 0.0f;
};

// Eine Timeline fÅr einen einzelnen Uniform-Parameter
struct ParamTimeline {
    std::string name; // z.ˇB. "glow", "speed", "fogAmount"
    std::vector<Keyframe> keyframes;

    // Interpolierter Wert zur gegebenen Zeit (linear)
    float evaluate(float t) const {
        if (keyframes.empty()) return 0.0f;
        if (keyframes.size() == 1 || t <= keyframes.front().time) return keyframes.front().value;
        if (t >= keyframes.back().time) return keyframes.back().value;

        for (size_t i = 0; i < keyframes.size() - 1; ++i) {
            const Keyframe& a = keyframes[i];
            const Keyframe& b = keyframes[i + 1];
            if (t >= a.time && t <= b.time) {
                float f = (t - a.time) / (b.time - a.time);
                return a.value * (1.0f - f) + b.value * f;
            }
        }
        return 0.0f;
    }
};

// Ein ShaderClip beschreibt einen ComputeShader + Parameter Åber Zeit
struct ShaderClip {
    std::string name;
    float startTime = 0.0f;
    float duration = 10.0f;

    ComputeShader* shader = nullptr;

    std::vector<ParamTimeline> parameters;

    // Gibt true zurÅck, wenn dieser Clip zu Zeit t aktiv ist
    bool isActive(float t) const {
        return (t >= startTime && t <= startTime + duration);
    }

    // Setzt alle Parameter fÅr diesen Clip auf den Shader
    void apply(float globalTime) {
        if (!shader) return;
        float localTime = globalTime - startTime;

        for (const auto& param : parameters) {
            float value = param.evaluate(localTime);
            shader->setFloat(param.name.c_str(), value);
        }
    }
};
