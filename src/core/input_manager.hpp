#pragma once

#include <raylib.h>
#include <unordered_map>
#include <string>


class InputManager {
public:
    InputManager()  = default;
    ~InputManager() { Shutdown(); }
    
    // Non-copyable
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    // Call once at the start of every frame
    void Update(const class Renderer& renderer);

    // Keyboard actions
    bool IsPressed(std::string action) const;
    bool IsDown(std::string action)    const;
    bool IsReleased(std::string action) const;

    // Mouse buttons
    bool IsMouseLeftPressed()  const;
    bool IsMouseRightPressed() const;
    bool IsMouseLeftDown()     const;
    bool IsMouseRightDown()    const;

    // Mouse position (virtual / letterbox-corrected)
    Vector2 GetMousePosition() const { return m_virtualMouse; }

    // Mouse consumption
    void ConsumeMouseClick();
    bool IsMouseConsumed() const { return m_mouseConsumed; }

    //Add action
    void AddAction(std::string action, KeyboardKey key);

    // Shutdown
    void Shutdown();

private:
    std::unordered_map<std::string, KeyboardKey> m_bindings;

    Vector2            m_virtualMouse  = {};
    bool               m_mouseConsumed = false;
};