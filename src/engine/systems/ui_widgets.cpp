#include <engine/systems/ui_widgets.hpp>
#include <engine/core/text_renderer.hpp>
#include <algorithm>
#include <cmath>

// --- Button ---

void Button::Update(Vector2 mouse, bool pressed) {
    m_clicked = false;
    m_hovered = false;
    if (!m_enabled) return;
    m_hovered = CheckCollisionPointRec(mouse, m_rect);
    if (m_hovered && pressed)
        m_clicked = true;
}

void Button::Draw(bool selected, const WidgetStyle& style) const {
    const WidgetStyle& s = m_enabled ? style : kDisabledStyle;
    Color bg = m_hovered ? s.m_bgHovered : s.m_bgNormal;
    DrawRectangleRec(m_rect, bg);
    Color border = selected ? s.m_borderSel : s.m_border;
    DrawRectangleLinesEx(m_rect, selected ? s.m_borderWidthActive : s.m_borderWidth, border);

    if (m_fontSize != 0 && !m_label.empty()) {
        int lw = Text::Measure(m_label.c_str(), m_fontSize, Text::Kind::Button);
        Color lc = m_enabled ? m_labelColor : kDisabledStyle.m_text;
        Text::Draw(m_label.c_str(),
            static_cast<int>(m_rect.x + (m_rect.width - lw) / 2.0f),
            static_cast<int>(m_rect.y + (m_rect.height - m_fontSize) / 2.0f),
            m_fontSize, lc, Text::Kind::Button);
    }
}

void Button::DrawLabel(int fontSize, Color color) const {
    if (m_label.empty()) return;
    int lw = Text::Measure(m_label.c_str(), fontSize, Text::Kind::Button);
    Text::Draw(m_label.c_str(),
        static_cast<int>(m_rect.x + (m_rect.width - lw) / 2.0f),
        static_cast<int>(m_rect.y + (m_rect.height - fontSize) / 2.0f),
        fontSize, color, Text::Kind::Button);
}

// --- Slider ---

void Slider::Update(Vector2 mouse, bool held) {
    bool over = CheckCollisionPointRec(mouse, m_rect);
    m_dragging = held && (over || m_dragging);

    if (m_dragging && m_max > m_min) {
        float t = std::clamp((mouse.x - m_rect.x) / m_rect.width, 0.0f, 1.0f);
        float value = m_min + t * (m_max - m_min);

        // Snap to the nearest step boundary measured from m_min.
        if (m_step > 0.0f)
            value = m_min + std::round((value - m_min) / m_step) * m_step;

        m_value = std::clamp(value, m_min, m_max);
    }
}

void Slider::Draw(const WidgetStyle& style) const {
    DrawRectangleRec(m_rect, style.m_bgNormal);
    DrawRectangleLinesEx(m_rect, style.m_borderWidth, style.m_border);

    if (m_max <= m_min) return;

    float t = (m_value - m_min) / (m_max - m_min);
    Rectangle filled = { m_rect.x, m_rect.y, m_rect.width * t, m_rect.height };
    DrawRectangleRec(filled, style.m_accent);

    // Knob
    float kx = m_rect.x + m_rect.width * t;
    DrawRectangle(static_cast<int>(kx - 3), static_cast<int>(m_rect.y) - 2,
        6, static_cast<int>(m_rect.height) + 4, style.m_text);
}

// --- Toggle ---

void Toggle::Update(Vector2 mouse, bool pressed) {
    m_clicked = false;
    if (CheckCollisionPointRec(mouse, m_rect) && pressed) {
        m_value = !m_value;
        m_clicked = true;
    }
}

void Toggle::Draw(const WidgetStyle& style) const {
    Color bg = m_value ? style.m_bgActive : style.m_bgNormal;
    DrawRectangleRec(m_rect, bg);
    DrawRectangleLinesEx(m_rect, style.m_borderWidth, style.m_border);

    if (!m_label.empty()) {
        int fontSize = static_cast<int>(m_rect.height * 0.65f);
        Text::Draw(m_label.c_str(),
            static_cast<int>(m_rect.x + m_rect.width + 6.0f),
            static_cast<int>(m_rect.y + (m_rect.height - fontSize) / 2.0f),
            fontSize, style.m_text);
    }
}

// --- TextInput ---

void TextInput::Update(Vector2 mouse, bool pressed) {
    if (pressed)
        m_focused = CheckCollisionPointRec(mouse, m_rect);

    if (!m_focused) return;

    int ch = GetCharPressed();
    while (ch > 0) {
        if (ch >= 32 && static_cast<int>(m_text.size()) < m_maxLength)
            m_text += static_cast<char>(ch);
        ch = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && !m_text.empty())
        m_text.pop_back();

    if (IsKeyPressed(KEY_ENTER))
        m_focused = false;
}

// --- ProgressBar ---

void ProgressBar::Draw(const WidgetStyle& style) const {
    DrawRectangleRec(m_rect, style.m_bgNormal);
    DrawRectangleLinesEx(m_rect, style.m_borderWidth, style.m_border);

    if (m_max > 0.0f && m_value > 0.0f) {
        float t = std::clamp(m_value / m_max, 0.0f, 1.0f);
        Rectangle filled = { m_rect.x, m_rect.y, m_rect.width * t, m_rect.height };
        DrawRectangleRec(filled, style.m_accent);
    }
}

// --- TextInput ---

void TextInput::Draw(const WidgetStyle& style) const {
    DrawRectangleRec(m_rect, style.m_bgInput);
    Color border = m_focused ? style.m_accent : style.m_border;
    DrawRectangleLinesEx(m_rect, m_focused ? style.m_borderWidthActive : style.m_borderWidth, border);

    int fontSize = static_cast<int>(m_rect.height * 0.6f);
    float padding = m_rect.height * 0.2f;

    if (!m_text.empty() || m_focused) {
        std::string display = m_text + (m_focused ? "_" : "");
        Text::Draw(display.c_str(),
            static_cast<int>(m_rect.x + padding),
            static_cast<int>(m_rect.y + (m_rect.height - fontSize) / 2.0f),
            fontSize, style.m_text);
    } else if (!m_placeholder.empty()) {
        // Placeholder in muted colour when the input is empty and unfocused.
        Color phColor = style.m_text;
        phColor.a = static_cast<unsigned char>(phColor.a * 0.45f);
        Text::Draw(m_placeholder.c_str(),
            static_cast<int>(m_rect.x + padding),
            static_cast<int>(m_rect.y + (m_rect.height - fontSize) / 2.0f),
            fontSize, phColor);
    }
}

// --- ScrollableList ---

float ScrollableList::MaxScroll(int count, float screenH) const {
    float contentH = count * (m_cfg.m_cardH + m_cfg.m_cardGap);
    float bandH = ListBottom(screenH) - ListTop();
    return std::max(0.0f, contentH - bandH);
}

Rectangle ScrollableList::CardRect(int index, float screenW, float screenH) const {
    (void)screenH;
    float cardW = screenW - 2.0f * m_cfg.m_margin;
    float y = ListTop() - m_scroll + index * (m_cfg.m_cardH + m_cfg.m_cardGap);
    return {m_cfg.m_margin, y, cardW, m_cfg.m_cardH};
}

void ScrollableList::ProcessScroll(float wheel, int count, float screenH) {
    if (wheel != 0.0f)
        m_scroll = std::clamp(m_scroll - wheel * m_cfg.m_scrollSpeed, 0.0f, MaxScroll(count, screenH));
}

int ScrollableList::ProcessHover(Vector2 mouse, bool clicked, int count, float screenW, float screenH) {
    m_hovered = -1;
    bool inBand = mouse.y >= ListTop() && mouse.y <= ListBottom(screenH);
    for (int i = 0; i < count; i++) {
        if (inBand && CheckCollisionPointRec(mouse, CardRect(i, screenW, screenH))) {
            m_hovered = i;
            if (clicked) return i;
        }
    }
    return -1;
}

bool ScrollableList::IsCardVisible(int index, float screenH) const {
    Rectangle r = CardRect(index, 0.0f, screenH);
    float bottom = ListBottom(screenH);
    return r.y + r.height >= ListTop() && r.y <= bottom;
}

Rectangle ScrollableList::FooterButtonRect(float screenW, float screenH,
                                           float btnW, float btnH) const {
    return {screenW / 2.0f - btnW / 2.0f, ListBottom(screenH) + 18.0f, btnW, btnH};
}

void ScrollableList::DrawScrollbar(int count, float screenW, float screenH,
                                   Color trackColor, Color thumbColor) const {
    float maxScroll = MaxScroll(count, screenH);
    if (maxScroll <= 0.0f) return;
    float bandH = ListBottom(screenH) - ListTop();
    float contentH = bandH + maxScroll;
    float trackX = screenW - m_cfg.m_margin + 8.0f;
    float thumbH = bandH * (bandH / contentH);
    float thumbY = ListTop() + (m_scroll / maxScroll) * (bandH - thumbH);
    DrawRectangle(static_cast<int>(trackX), static_cast<int>(ListTop()), 6, static_cast<int>(bandH), trackColor);
    DrawRectangle(static_cast<int>(trackX), static_cast<int>(thumbY), 6, static_cast<int>(thumbH), thumbColor);
}

// --- WidgetGroup ---

void WidgetGroup::SetCount(int count) {
    m_slots.resize(static_cast<size_t>(std::max(0, count)));
}

void WidgetGroup::SetSlotSize(int index, float w, float h) {
    m_slots[index].m_width = w;
    m_slots[index].m_height = h;
}

void WidgetGroup::SetSlotVisible(int index, bool visible) {
    m_slots[index].m_visible = visible;
}

bool WidgetGroup::IsSlotVisible(int index) const {
    return m_slots[index].m_visible;
}

Rectangle WidgetGroup::ContentArea() const {
    return {
        m_config.m_bounds.x + m_config.m_padLeft,
        m_config.m_bounds.y + m_config.m_padTop,
        m_config.m_bounds.width - m_config.m_padLeft - m_config.m_padRight,
        m_config.m_bounds.height - m_config.m_padTop - m_config.m_padBottom
    };
}

int WidgetGroup::VisibleCount() const {
    int n = 0;
    for (const auto& s : m_slots)
        if (s.m_visible) n++;
    return n;
}

float WidgetGroup::TotalContentWidth() const {
    float w = 0.0f;
    int vis = 0;
    for (const auto& s : m_slots) {
        if (!s.m_visible) continue;
        float iw = s.m_width != 0.0f ? s.m_width : m_config.m_defaultItemW;
        w += iw;
        vis++;
    }
    if (vis > 1) w += m_config.m_gapX * static_cast<float>(vis - 1);
    return w;
}

float WidgetGroup::TotalContentHeight() const {
    float h = 0.0f;
    int vis = 0;
    for (const auto& s : m_slots) {
        if (!s.m_visible) continue;
        float ih = s.m_height != 0.0f ? s.m_height : m_config.m_defaultItemH;
        h += ih;
        vis++;
    }
    if (vis > 1) h += m_config.m_gapY * static_cast<float>(vis - 1);
    return h;
}

int WidgetGroup::HitTest(Vector2 point) const {
    for (int i = 0; i < Count(); i++) {
        const Slot& slot = m_slots[i];
        if (slot.m_visible && CheckCollisionPointRec(point, slot.m_rect))
            return i;
    }
    return -1;
}

void WidgetGroup::Layout() {
    switch (m_config.m_mode) {
    case WidgetGroupConfig::Mode::Vertical:   layoutVertical();   break;
    case WidgetGroupConfig::Mode::Horizontal: layoutHorizontal(); break;
    case WidgetGroupConfig::Mode::Grid:       layoutGrid();       break;
    }
}

// --- Private layout helpers ---

float WidgetGroup::resolveWidth(const Slot& slot, float contentW) const {
    float w = slot.m_width != 0.0f ? slot.m_width : m_config.m_defaultItemW;
    return w != 0.0f ? w : contentW;
}

float WidgetGroup::resolveHeight(const Slot& slot, float contentH) const {
    float h = slot.m_height != 0.0f ? slot.m_height : m_config.m_defaultItemH;
    return h != 0.0f ? h : contentH;
}

void WidgetGroup::layoutVertical() {
    Rectangle ca = ContentArea();
    bool stretchW = m_config.m_align == WidgetGroupConfig::Align::Stretch;

    // Collect heights for visible slots.
    float totalH = 0.0f;
    for (auto& s : m_slots) {
        if (!s.m_visible) continue;
        s.m_rect.width = stretchW ? ca.width : resolveWidth(s, ca.width);
        s.m_rect.height = resolveHeight(s, ca.height);
        totalH += s.m_rect.height;
    }
    int vis = VisibleCount();
    if (vis > 1) totalH += m_config.m_gapY * static_cast<float>(vis - 1);

    float y;
    switch (m_config.m_pack) {
    case WidgetGroupConfig::Pack::Start:  y = ca.y;                              break;
    case WidgetGroupConfig::Pack::Center: y = ca.y + (ca.height - totalH) / 2.0f; break;
    case WidgetGroupConfig::Pack::End:    y = ca.y + ca.height - totalH;          break;
    default: y = ca.y; break;
    }

    for (auto& s : m_slots) {
        if (!s.m_visible) continue;
        float w = s.m_rect.width;
        switch (m_config.m_align) {
        case WidgetGroupConfig::Align::Start:   s.m_rect.x = ca.x;                             break;
        case WidgetGroupConfig::Align::Center:  s.m_rect.x = ca.x + (ca.width - w) / 2.0f;      break;
        case WidgetGroupConfig::Align::End:     s.m_rect.x = ca.x + ca.width - w;               break;
        case WidgetGroupConfig::Align::Stretch: s.m_rect.x = ca.x;                              break;
        }
        s.m_rect.y = y;
        y += s.m_rect.height + m_config.m_gapY;
    }
}

void WidgetGroup::layoutHorizontal() {
    Rectangle ca = ContentArea();
    bool stretchH = m_config.m_align == WidgetGroupConfig::Align::Stretch;

    float totalW = 0.0f;
    for (auto& s : m_slots) {
        if (!s.m_visible) continue;
        s.m_rect.width = resolveWidth(s, ca.width);
        s.m_rect.height = stretchH ? ca.height : resolveHeight(s, ca.height);
        totalW += s.m_rect.width;
    }
    int vis = VisibleCount();
    if (vis > 1) totalW += m_config.m_gapX * static_cast<float>(vis - 1);

    float x;
    switch (m_config.m_pack) {
    case WidgetGroupConfig::Pack::Start:  x = ca.x;                              break;
    case WidgetGroupConfig::Pack::Center: x = ca.x + (ca.width - totalW) / 2.0f; break;
    case WidgetGroupConfig::Pack::End:    x = ca.x + ca.width - totalW;          break;
    default: x = ca.x; break;
    }

    // In Horizontal mode, m_align controls vertical placement.
    for (auto& s : m_slots) {
        if (!s.m_visible) continue;
        float h = s.m_rect.height;
        switch (m_config.m_align) {
        case WidgetGroupConfig::Align::Start:   s.m_rect.y = ca.y;                             break;
        case WidgetGroupConfig::Align::Center:  s.m_rect.y = ca.y + (ca.height - h) / 2.0f;     break;
        case WidgetGroupConfig::Align::End:     s.m_rect.y = ca.y + ca.height - h;              break;
        case WidgetGroupConfig::Align::Stretch: s.m_rect.y = ca.y;                              break;
        }
        s.m_rect.x = x;
        x += s.m_rect.width + m_config.m_gapX;
    }
}

void WidgetGroup::layoutGrid() {
    Rectangle ca = ContentArea();
    int cols = std::max(1, m_config.m_columns);
    int vis = VisibleCount();
    if (vis == 0) return;
    int rows = (vis + cols - 1) / cols;

    float cellW = (ca.width - (cols - 1) * m_config.m_gapX) / static_cast<float>(cols);
    float cellH = (ca.height - (rows - 1) * m_config.m_gapY) / static_cast<float>(rows);
    bool stretchW = m_config.m_align == WidgetGroupConfig::Align::Stretch;
    bool stretchH = m_config.m_alignV == WidgetGroupConfig::Align::Stretch;

    // Walk slots in storage order, placing visible ones row-major.
    int placed = 0;
    for (auto& s : m_slots) {
        if (!s.m_visible) continue;
        int col = placed % cols;
        int row = placed / cols;

        float cellX = ca.x + col * (cellW + m_config.m_gapX);
        float cellY = ca.y + row * (cellH + m_config.m_gapY);

        float iw = stretchW ? cellW : std::min(resolveWidth(s, cellW), cellW);
        float ih = stretchH ? cellH : std::min(resolveHeight(s, cellH), cellH);

        // X within cell.
        switch (m_config.m_align) {
        case WidgetGroupConfig::Align::Start:   s.m_rect.x = cellX;                       break;
        case WidgetGroupConfig::Align::Center:  s.m_rect.x = cellX + (cellW - iw) / 2.0f; break;
        case WidgetGroupConfig::Align::End:     s.m_rect.x = cellX + cellW - iw;          break;
        case WidgetGroupConfig::Align::Stretch: s.m_rect.x = cellX;                       break;
        }
        // Y within cell.
        switch (m_config.m_alignV) {
        case WidgetGroupConfig::Align::Start:   s.m_rect.y = cellY;                       break;
        case WidgetGroupConfig::Align::Center:  s.m_rect.y = cellY + (cellH - ih) / 2.0f; break;
        case WidgetGroupConfig::Align::End:     s.m_rect.y = cellY + cellH - ih;          break;
        case WidgetGroupConfig::Align::Stretch: s.m_rect.y = cellY;                       break;
        }

        s.m_rect.width = iw;
        s.m_rect.height = ih;
        placed++;
    }
}
