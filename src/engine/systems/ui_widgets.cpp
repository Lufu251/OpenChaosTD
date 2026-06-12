#include <engine/systems/ui_widgets.hpp>
#include <engine/core/text_renderer.hpp>
#include <algorithm>
#include <cmath>

// --- Button ---

void Button::Update(Vector2 mouse, bool pressed) {
    m_clicked = false;
    m_hovered = CheckCollisionPointRec(mouse, m_rect);
    if (m_hovered && pressed)
        m_clicked = true;
}

void Button::Draw(bool selected, const WidgetStyle& style) const {
    Color bg = m_hovered ? style.m_bgHovered : style.m_bgNormal;
    DrawRectangleRec(m_rect, bg);
    Color border = selected ? style.m_borderSel : style.m_border;
    DrawRectangleLinesEx(m_rect, selected ? style.m_borderWidthActive : style.m_borderWidth, border);
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
    std::string display = m_text + (m_focused ? "_" : "");
    Text::Draw(display.c_str(),
        static_cast<int>(m_rect.x + padding),
        static_cast<int>(m_rect.y + (m_rect.height - fontSize) / 2.0f),
        fontSize, style.m_text);
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
