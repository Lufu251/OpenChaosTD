#include <hud/event_hud.hpp>
#include <hud/hud_theme.hpp>
#include <hud/hud.hpp>
#include <engine/core/text_renderer.hpp>
#include <raylib.h>
#include <algorithm>

void EventHUD::Add(const std::string& message, float duration) {
    // Reset timer if this message is already the newest — prevents visual spam
    if (!m_entries.empty() && m_entries.back().m_message == message) {
        m_entries.back().m_timeLeft = duration;
        return;
    }

    // Drop oldest entry when at capacity
    if (static_cast<int>(m_entries.size()) >= Hud::g_eventCfg.maxEntries)
        m_entries.erase(m_entries.begin());

    m_entries.push_back({message, duration});
}

void EventHUD::Update(float dt) {
    for (auto& entry : m_entries)
        entry.m_timeLeft -= dt;

    std::erase_if(m_entries, [](const Entry& e) { return e.m_timeLeft <= 0.0f; });
}

void EventHUD::Draw() {
    if (!m_visible || m_entries.empty()) return;

    // Route the shared margin and toast font through the central metrics/typographic scale.
    const Hud::PanelMetrics m = Hud::PanelMetrics::Make(m_scale);
    const float margin    = m.margin;
    const float lineH     = Scaled(Hud::kFontBodyBase * Hud::kToastRowToBody); // row stride follows the toast font
    const float scoreHudH = Scaled(Hud::kStatusBarBaseHeight); // keep messages below the top panel
    const int   fontSize  = m.fontBody;

    // The background box hugs the measured text with small insets expressed as fractions of the
    // shared margin (formerly the standalone 2/1/10/3/2 px toast paddings at the default margin 8).
    const float padX    = margin * 0.25f;  // bg left extension and row-height shrink
    const float padTop  = margin * 0.125f; // bg top extension above the row
    const float padW    = margin * 1.25f;  // bg width beyond the text width
    const float textInX = margin * 0.375f; // text inset from the bar margin
    const float textInY = margin * 0.25f;  // text inset from the row top

    int n = static_cast<int>(m_entries.size());
    float baseY = scoreHudH + margin;

    for (int i = 0; i < n; i++) {
        const Entry& entry = m_entries[i];

        // Fade both background and text uniformly over the last fadeTime seconds.
        float fade = std::min(entry.m_timeLeft / Hud::g_eventCfg.fadeTime, 1.0f);

        // Stack downward: oldest (index 0) at baseY, newer entries below it
        float y = baseY + static_cast<float>(i) * lineH;

        int textW = Text::Measure(entry.m_message.c_str(), fontSize);
        Rectangle bg = { margin - padX, y - padTop, textW + padW, lineH - padX };
        Hud::DrawOverlayToast(entry.m_message.c_str(), bg,
                              margin + textInX, y + textInY,
                              fontSize, fade);
    }
}
