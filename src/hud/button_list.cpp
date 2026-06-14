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
    int n = static_cast<int>(items.size());
    m_group.SetCount(n);
    m_group.m_config.m_mode = WidgetGroupConfig::Mode::Vertical;
    m_group.m_config.m_pack = WidgetGroupConfig::Pack::Start;
    m_group.m_config.m_align = WidgetGroupConfig::Align::Start;
    m_group.m_config.m_bounds = {x, firstY, w, spacing * static_cast<float>(n - 1) + h};
    m_group.m_config.m_defaultItemW = w;
    m_group.m_config.m_defaultItemH = h;
    m_group.m_config.m_gapY = spacing - h;
    m_group.Layout();
    for (int i = 0; i < n; i++)
        items[i].button.m_rect = m_group[i].m_rect;
}

void ButtonList::Update(Vector2 mouse, bool pressed, bool& clicked) {
    for (auto& item : items)
        item.button.Update(mouse, pressed);
    for (auto& item : items) {
        if (item.enabled && item.button.IsClicked()) {
            item.signal.Raise();
            clicked = true;
        }
    }
}

void ButtonList::Draw(int fontSize, Color labelColor) {
    for (auto& item : items) {
        item.button.m_fontSize = fontSize;
        item.button.m_enabled = item.enabled;
        item.button.m_labelColor = item.enabled ? labelColor : kTextDisabled;
        item.button.Draw();
    }
}

} // namespace Hud
