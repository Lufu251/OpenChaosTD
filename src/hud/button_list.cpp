#include <hud/button_list.hpp>
#include <hud/hud_theme.hpp> // kTextDisabled

namespace Hud {

int ButtonList::Add(const std::string& label) {
    Item item;
    item.button.m_label = label;
    items.push_back(std::move(item));
    return static_cast<int>(items.size()) - 1;
}

void ButtonList::LayoutVertical(float x, float firstY, float w, float h, float spacing) {
    for (size_t i = 0; i < items.size(); i++)
        items[i].button.m_rect = { x, firstY + spacing * static_cast<float>(i), w, h };
}

void ButtonList::Update(Vector2 mouse, bool pressed, bool& clicked) {
    for (auto& item : items)
        if (item.enabled) item.button.Update(mouse, pressed);
    for (auto& item : items) {
        if (item.enabled && item.button.IsClicked()) {
            item.signal.Raise();
            clicked = true;
        }
    }
}

void ButtonList::Draw(int fontSize, Color labelColor) const {
    for (const auto& item : items) {
        // A disabled item draws with the muted style and disabled label color (mirrors how
        // MenuState's "Continue" used to gray itself out by hand).
        item.button.Draw(false, item.enabled ? kDefaultStyle : kDisabledStyle);
        item.button.DrawLabel(fontSize, item.enabled ? labelColor : kTextDisabled);
    }
}

} // namespace Hud
