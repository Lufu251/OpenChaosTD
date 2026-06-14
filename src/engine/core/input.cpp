#include <engine/core/input.hpp>
#include <engine/core/screen.hpp>
#include <engine/util/file_store.hpp>
#include <toml++/toml.hpp>

void Input::Update(const Screen& renderer) {
    m_mouseConsumed = false;
    m_virtualMouse = renderer.GetVirtualMouse();
}

void Input::AddAction(const std::string& action, KeyboardKey key) {
    m_bindings[action] = key;
}

void Input::AddAction(const std::string& action, MouseButton button) {
    m_bindings[action] = button;
}

bool Input::IsPressed(const std::string& action) const {
    auto it = m_bindings.find(action);
    if (it == m_bindings.end()) return false;
    return std::visit([](auto&& b) -> bool {
        using T = std::decay_t<decltype(b)>;
        if constexpr (std::is_same_v<T, KeyboardKey>)
            return b != KEY_NULL && IsKeyPressed(b);
        else
            return IsMouseButtonPressed(b);
    }, it->second);
}

bool Input::IsDown(const std::string& action) const {
    auto it = m_bindings.find(action);
    if (it == m_bindings.end()) return false;
    return std::visit([](auto&& b) -> bool {
        using T = std::decay_t<decltype(b)>;
        if constexpr (std::is_same_v<T, KeyboardKey>)
            return b != KEY_NULL && IsKeyDown(b);
        else
            return IsMouseButtonDown(b);
    }, it->second);
}

float Input::GetMouseWheelDelta() const {
    return GetMouseWheelMove();
}

bool Input::IsMousePressed(MouseButton btn) const  { return IsMouseButtonPressed(btn); }
bool Input::IsMouseDown(MouseButton btn) const     { return IsMouseButtonDown(btn); }

void Input::ConsumeMouseInput() {
    m_mouseConsumed = true;
}

// Single source of truth for name <-> key mapping, shared by ParseKey and KeyName.
static const std::unordered_map<std::string, KeyboardKey>& KeyTable() {
    static const std::unordered_map<std::string, KeyboardKey> kMap = {
        {"A", KEY_A}, {"B", KEY_B}, {"C", KEY_C}, {"D", KEY_D}, {"E", KEY_E},
        {"F", KEY_F}, {"G", KEY_G}, {"H", KEY_H}, {"I", KEY_I}, {"J", KEY_J},
        {"K", KEY_K}, {"L", KEY_L}, {"M", KEY_M}, {"N", KEY_N}, {"O", KEY_O},
        {"P", KEY_P}, {"Q", KEY_Q}, {"R", KEY_R}, {"S", KEY_S}, {"T", KEY_T},
        {"U", KEY_U}, {"V", KEY_V}, {"W", KEY_W}, {"X", KEY_X}, {"Y", KEY_Y},
        {"Z", KEY_Z},
        {"0", KEY_ZERO}, {"1", KEY_ONE}, {"2", KEY_TWO}, {"3", KEY_THREE},
        {"4", KEY_FOUR}, {"5", KEY_FIVE}, {"6", KEY_SIX}, {"7", KEY_SEVEN},
        {"8", KEY_EIGHT}, {"9", KEY_NINE},
        {"ENTER", KEY_ENTER}, {"ESCAPE", KEY_ESCAPE}, {"SPACE", KEY_SPACE},
        {"GRAVE", KEY_GRAVE}, {"TAB", KEY_TAB},
        {"ARROW_LEFT", KEY_LEFT}, {"ARROW_RIGHT", KEY_RIGHT},
        {"ARROW_UP", KEY_UP}, {"ARROW_DOWN", KEY_DOWN},
        {"LEFT_SHIFT", KEY_LEFT_SHIFT}, {"LEFT_CTRL", KEY_LEFT_CONTROL},
        {"LEFT_ALT", KEY_LEFT_ALT},
        {"F1", KEY_F1}, {"F2", KEY_F2}, {"F3", KEY_F3}, {"F4", KEY_F4},
        {"F5", KEY_F5}, {"F6", KEY_F6}, {"F7", KEY_F7}, {"F8", KEY_F8},
        {"F9", KEY_F9}, {"F10", KEY_F10}, {"F11", KEY_F11}, {"F12", KEY_F12},
    };
    return kMap;
}

KeyboardKey Input::ParseKey(const std::string& name) {
    const auto& map = KeyTable();
    auto it = map.find(name);
    return it != map.end() ? it->second : KEY_NULL;
}

std::string Input::KeyName(KeyboardKey key) {
    if (key == KEY_NULL) return "";
    for (const auto& [name, k] : KeyTable())
        if (k == key) return name;
    return "";
}

void Input::Load(FileStore& fileStore) {
    if (!fileStore.Exists("config/keybindings.toml"))
        return;
    const toml::table tbl = fileStore.LoadToml("config/keybindings.toml");
    // Bindings are grouped by category: [Movement] Up = "W", ... .
    // The category is presentation-only; each action name is globally unique.
    for (auto& [category, actions] : tbl) {
        const toml::table* group = actions.as_table();
        if (!group) continue;
        for (auto& [action, value] : *group) {
            if (auto name = value.value<std::string>())
                AddAction(std::string(action.str()), ParseKey(*name));
        }
    }
}
