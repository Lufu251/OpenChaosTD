#include <hud/wave_hud.hpp>
#include <hud/hud_draw.hpp>
#include <hud/hud_theme.hpp>
#include <engine/core/text.hpp>
#include <engine/core/input.hpp>
#include <engine/core/resources.hpp>
#include <raylib.h>

void WaveHUD::Build(float scale, int screenW) {
    HUD::Build(scale);
    m_metrics = Hud::PanelMetrics::Make(scale);
    m_metrics.panelW = m_metrics.Scaled(200.0f);
    m_cardGap = m_metrics.Scaled(6.0f);
    m_cardPad = m_metrics.Scaled(6.0f);
    m_iconSize = m_metrics.Scaled(44.0f);
    // Sit below the top status bar (its base height) plus a small gap.
    m_topOffset = m_metrics.Scaled(Hud::kStatusBarBaseHeight + Hud::kWavePanelGap);
    m_screenW = screenW;
    Hide(); // hidden by default; shown via the Waves button or the WaveInfo hotkey
}

void WaveHUD::ProcessInput(Input& input) {
    // Swallow clicks that land on the panel so they don't place/deselect towers underneath.
    BeginInput(input);
}

void WaveHUD::Rebuild(const WaveView& view) {
    // One card per enemy entry, so the panel can size itself to their total height (cards grow with
    // their module count, exactly like the tower info panel). Reuse the member vector's capacity.
    CardMetrics cardMetrics{ m_cardPad, m_iconSize, m_metrics.lineH, m_metrics.fontBody };
    m_cards.clear();
    m_cards.reserve(view.m_entries.size());
    for (const auto& entry : view.m_entries) {
        WaveEnemyCard card;
        card.SetContent(entry, cardMetrics);
        m_cards.push_back(std::move(card));
    }

    // An empty wave collapses to a single note line; otherwise sum the cards plus the gaps.
    float bodyH = m_metrics.lineH;
    if (!m_cards.empty()) {
        bodyH = static_cast<float>(m_cards.size() - 1) * m_cardGap;
        for (const auto& card : m_cards)
            bodyH += card.Measure();
    }
    float panelH = m_metrics.margin + m_metrics.headerH + bodyH + m_metrics.margin;

    float screenW = static_cast<float>(m_screenW);
    m_panelRect = { screenW - m_metrics.panelW - m_metrics.margin, m_topOffset, m_metrics.panelW, panelH };
}

void WaveHUD::Draw(const WaveView& view, Resources& assets) {
    if (!m_visible) return;

    Rebuild(view);
    DrawWindowBackground();

    const float margin = m_metrics.margin;
    float x = m_panelRect.x + margin;
    float y = m_panelRect.y + margin;
    float innerW = m_metrics.panelW - 2.0f * margin;

    // Header: title on the left, upcoming threat budget right-aligned.
    Text::Draw("Next Wave", static_cast<int>(x), static_cast<int>(y), m_metrics.fontHeader, GOLD, Text::Kind::Heading);
    const char* budgetText = TextFormat("Budget: %d", static_cast<int>(view.m_budget));
    Hud::DrawTextRightAligned(budgetText, m_panelRect.x + m_metrics.panelW - margin,
                              y + (m_metrics.fontHeader - m_metrics.fontBody), m_metrics.fontBody,
                              Hud::kTextMuted);
    y += m_metrics.headerH;

    if (m_cards.empty()) {
        Text::Draw("No enemies", static_cast<int>(x), static_cast<int>(y), m_metrics.fontBody, Hud::kTextMuted);
        return;
    }

    for (const auto& card : m_cards) {
        float h = card.Measure();
        card.Draw({ x, y, innerW, h }, assets);
        y += h + m_cardGap;
    }
}
