#include <hud/wave_hud.hpp>
#include <hud/hud.hpp>
#include <hud/hud_theme.hpp>
#include <engine/core/text_renderer.hpp>
#include <engine/core/input.hpp>
#include <engine/core/resources.hpp>
#include <raylib.h>
#include <algorithm>

void WaveHUD::Build(float scale, int screenW) {
    HUD::Build(scale);
    m_metrics = Hud::PanelMetrics::Make(scale);
    m_metrics.panelW = m_metrics.Scaled(Hud::g_waveCfg.width);
    m_cardGap  = m_metrics.Scaled(Hud::g_waveCfg.cardGap);
    m_cardPad  = m_metrics.Scaled(Hud::g_waveCfg.cardPadding);
    m_iconSize = m_metrics.Scaled(Hud::g_waveCfg.iconSize);
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
    Text::Draw("Next Wave", static_cast<int>(x), static_cast<int>(y), m_metrics.fontHeader, Hud::kTextHeader, Text::Kind::Heading);
    const char* budgetText = TextFormat("Budget: %d", static_cast<int>(view.m_budget));
    Hud::DrawTextRightAligned(budgetText, m_panelRect.x + m_metrics.panelW - margin,
                              y + (m_metrics.fontHeader - m_metrics.fontBody), m_metrics.fontBody,
                              Hud::kTextSecondary);
    y += m_metrics.headerH;

    if (m_cards.empty()) {
        Text::Draw("No enemies", static_cast<int>(x), static_cast<int>(y), m_metrics.fontBody, Hud::kTextSecondary);
        return;
    }

    for (const auto& card : m_cards) {
        float h = card.Measure();
        card.Draw({ x, y, innerW, h }, assets);
        y += h + m_cardGap;
    }
}

// ============================================================================
// WaveEnemyCard
// ============================================================================

// Resolve a texture by key and aspect-fit it into `dest`. No-op if the key is missing.
static void DrawSpriteFit(const std::string& textureKey, Resources& assets, Rectangle dest) {
    if (!assets.HasTexture(textureKey)) return; // GetTexture throws on a missing key
    DrawTextureFitted(assets.GetTexture(textureKey), dest);
}

void WaveEnemyCard::SetContent(const WaveEnemyEntry& entry, const CardMetrics& metrics) {
    m_metrics    = metrics;
    m_count      = entry.m_count;
    m_name       = entry.m_name;
    m_level      = entry.m_level;
    m_hasProto   = entry.m_hasProto;
    m_textureKey = entry.m_textureKey;
    m_stats      = entry.m_stats;
}

float WaveEnemyCard::Measure() const {
    // Header row plus one row per module stat; never shorter than the sprite tile.
    float textH = static_cast<float>(1 + m_stats.size()) * m_metrics.lineH;
    float contentH = std::max(m_metrics.iconSize, textH);
    return contentH + 2.0f * m_metrics.pad;
}

void WaveEnemyCard::Draw(Rectangle bounds, Resources& assets) const {
    const float pad = m_metrics.pad;

    // Card frame: subtle fill plus a border so each enemy type reads as a distinct tile.
    Hud::DrawFramedBox(bounds, Hud::kCardFill, Hud::kCardBorder);

    // Icon tile on the left; the sprite draws directly over the card with a transparent backing.
    Rectangle icon = { bounds.x + pad, bounds.y + pad, m_metrics.iconSize, m_metrics.iconSize };
    if (m_hasProto)
        DrawSpriteFit(m_textureKey, assets, icon);

    // Text column to the right of the icon.
    float tx = icon.x + m_metrics.iconSize + pad;
    float ty = bounds.y + pad;
    float textRight = bounds.x + bounds.width - pad;

    // Header row: "Nx Name" on the left, "Lv N" badge right-aligned.
    Text::Draw(TextFormat("%dx %s", m_count, m_name.c_str()),
             static_cast<int>(tx), static_cast<int>(ty), m_metrics.fontSm, Hud::kTextPrimary);
    if (m_hasProto)
        Hud::DrawTextRightAligned(TextFormat("Lv %d", m_level), textRight, ty, m_metrics.fontSm, Hud::kHighlight);
    ty += m_metrics.lineH;

    // One stat row per module, colored by the module's DescLine (core stats, Shield/Immune accents).
    Hud::DrawDescLines(m_stats, tx, ty, m_metrics.lineH, m_metrics.fontSm);
}
