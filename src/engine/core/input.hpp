#pragma once

#include <raylib.h>
#include <unordered_map>
#include <string>
#include <variant>

class Screen;
class FileStore;

class Input {
public:
    Input() = default;

    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    void Update(const Screen& renderer);
    void Load(FileStore& fileStore);

    bool IsPressed(const std::string& action) const;
    bool IsDown(const std::string& action) const;

    float GetMouseWheelDelta() const;
    Vector2 GetMousePosition() const { return m_virtualMouse; }
    Vector2 GetWorldMousePosition(const Camera2D& camera) const { return GetScreenToWorld2D(m_virtualMouse, camera); }

    // Raw mouse button queries — bypass the action binding system.
    bool IsMousePressed(MouseButton btn) const;
    bool IsMouseDown(MouseButton btn) const;

    void ConsumeMouseInput();
    bool IsMouseInputConsumed() const { return m_mouseConsumed; }

    void AddAction(const std::string& action, KeyboardKey key);
    void AddAction(const std::string& action, MouseButton button);

    // Key-name <-> KeyboardKey conversion, sharing one lookup table. Used by the
    // settings menu to display, capture, and persist rebinds. KeyName returns ""
    // for keys outside the supported set; ParseKey returns KEY_NULL for unknown names.
    static KeyboardKey ParseKey(const std::string& name);
    static std::string KeyName(KeyboardKey key);

private:
    using Binding = std::variant<KeyboardKey, MouseButton>;
    std::unordered_map<std::string, Binding> m_bindings;
    Vector2 m_virtualMouse = {};
    bool m_mouseConsumed = false;
};
