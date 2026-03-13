#include <core/input_manager.hpp>
#include <core/renderer.hpp>

// Update — call once at the start of every frame
void InputManager::Update(const Renderer& renderer) {
    m_mouseConsumed = false;
    m_virtualMouse  = renderer.GetVirtualMouse();
}

// Keyboard — Action
bool InputManager::IsPressed(std::string action) const {
    KeyboardKey key = m_bindings.at(action);
    return (key != KEY_NULL) && IsKeyPressed(key);
}

bool InputManager::IsDown(std::string action) const {
    KeyboardKey key = m_bindings.at(action);
    return (key != KEY_NULL) && IsKeyDown(key);
}

bool InputManager::IsReleased(std::string action) const {
    KeyboardKey key = m_bindings.at(action);
    return (key != KEY_NULL) && IsKeyReleased(key);
}

// Mouse buttons
bool InputManager::IsMouseLeftPressed()  const { return IsMouseButtonPressed(MOUSE_LEFT_BUTTON);  }
bool InputManager::IsMouseRightPressed() const { return IsMouseButtonPressed(MOUSE_RIGHT_BUTTON); }
bool InputManager::IsMouseLeftDown()     const { return IsMouseButtonDown(MOUSE_LEFT_BUTTON);     }
bool InputManager::IsMouseRightDown()    const { return IsMouseButtonDown(MOUSE_RIGHT_BUTTON);    }

// Mouse consumption
void InputManager::ConsumeMouseClick() {
    m_mouseConsumed = true;
}

void InputManager::AddAction(std::string action, KeyboardKey key){
    m_bindings[action] = key;
}

void InputManager::Shutdown() {
    m_bindings.clear();
}