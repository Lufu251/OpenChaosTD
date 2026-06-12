#include <hud/tower_info_hud.hpp>
#include <hud/hud_draw.hpp>
#include <hud/hud_theme.hpp>
#include <engine/core/text.hpp>
#include <engine/core/input.hpp>
#include <raylib.h>
#include <algorithm>

void TowerInfoHUD::Build(float scale) {
    HUD::Build(scale);
    m_metrics = Hud::PanelMetrics::Make(scale);
    m_metrics.panelW = m_metrics.Scaled(160.0f);
    m_descLineH = m_metrics.Scaled(13.0f);
    m_sellH = m_metrics.Scaled(22.0f);
    m_sellGap = m_metrics.Scaled(6.0f);
    m_anchorGap = m_metrics.Scaled(20.0f);
    Hide(); // shown only while a tower is selected or hovered
}

void TowerInfoHUD::SetTarget(const TowerInfoView& view) {
    SetContent(view);
    Layout(view);
    Show();
}

void TowerInfoHUD::SetContent(const TowerInfoView& view) {
    // Snapshot the content the panel will render (no Tower/Enemy references kept).
    m_hasTarget     = true;
    m_name          = view.m_name;
    m_hasAttack     = view.m_hasAttack;
    m_level         = view.m_level;
    m_upgradeCount  = view.m_upgradeCount;
    m_statLines     = view.m_statLines;
    m_targetingName = view.m_targetingName;
    m_screenH       = view.m_screenH;

    m_showSell      = view.m_interactive;                              // always shown for a selected tower
    m_sellEnabled   = !view.m_waveActive;                             // but only sellable between waves
    m_showTargeting = view.m_interactive && view.m_hasAttack;         // retarget any time
    m_showUpgrade   = view.m_interactive && view.m_hasAttack && view.m_upgradeCount > 0;

    m_descLines = Text::Wrap(view.m_description, m_metrics.panelW - m_metrics.margin * 2.0f, m_metrics.fontBody);
}

void TowerInfoHUD::Layout(const TowerInfoView& view) {
    const float margin = m_metrics.margin;
    const float panelW = m_metrics.panelW;

    int statRows = static_cast<int>(m_statLines.size());
    float panelH = margin + m_metrics.headerH
        + static_cast<float>(m_descLines.size()) * m_descLineH
        + statRows * m_metrics.lineH
        + (m_showUpgrade ? m_sellGap + m_sellH : 0.0f)
        + (m_showTargeting ? m_sellGap + m_sellH : 0.0f)
        + (m_showSell ? m_sellGap + m_sellH : 0.0f) + margin;

    // Anchor above the screen point, then clamp so the panel stays on-screen
    m_panelRect = { view.m_screenPos.x - panelW / 2.0f, view.m_screenPos.y - panelH - m_anchorGap,
                    panelW, panelH };
    ClampPanelToScreen(view.m_screenW, view.m_screenH);

    // Lay config buttons bottom-up: Sell at the bottom, then Upgrade, then Targeting on top
    float btnW = panelW - margin * 2.0f;
    float btnY = m_panelRect.y + panelH - margin - m_sellH;
    if (m_showSell) {
        m_sellBtn.m_label = TextFormat("Sell: $%d", view.m_sellRefund);
        m_sellBtn.m_rect = { m_panelRect.x + margin, btnY, btnW, m_sellH };
        btnY -= m_sellGap + m_sellH;
    }
    m_upgradeReady = false;
    m_hasNextUpgrade = false;
    m_upgradePreview.clear();
    if (m_showUpgrade) {
        if (view.m_upgradeAtMax) {
            m_upgradeBtn.m_label = "Max Level";
        } else {
            m_upgradeBtn.m_label = TextFormat("Upgrade $%d", view.m_upgradeCost);
            m_upgradeReady = view.m_upgradeReady;
            m_hasNextUpgrade = true;
            m_upgradePreview = view.m_upgradePreview;
        }
        m_upgradeBtn.m_rect = { m_panelRect.x + margin, btnY, btnW, m_sellH };
        btnY -= m_sellGap + m_sellH;
    }
    if (m_showTargeting) {
        m_targetBtn.m_label = TextFormat("Target: %s", m_targetingName.c_str());
        m_targetBtn.m_rect = { m_panelRect.x + margin, btnY, btnW, m_sellH };
    }
}

void TowerInfoHUD::ProcessInput(Input& input) {
    Vector2 mousePos{};
    bool pressed = false;
    if (!BeginInput(input, mousePos, pressed)) return;

    // Update whenever an upgrade is available so hover registers even when unaffordable,
    // but only raise the buy signal when the player can pay.
    if (m_hasNextUpgrade) {
        m_upgradeBtn.Update(mousePos, pressed);
        if (m_upgradeReady && m_upgradeBtn.IsClicked()) {
            PlayClickSound();
            m_upgradeSignal.Raise();
        }
    }
    if (m_showTargeting) {
        m_targetBtn.Update(mousePos, pressed);
        if (m_targetBtn.IsClicked()) {
            PlayClickSound();
            m_targetSignal.Raise();
        }
    }
    // Sell is shown while disabled during waves, but only a wave-free (enabled) button reacts.
    if (m_showSell && m_sellEnabled) {
        m_sellBtn.Update(mousePos, pressed);
        if (m_sellBtn.IsClicked()) {
            PlayClickSound();
            m_sellSignal.Raise();
        }
    }
}

void TowerInfoHUD::Draw() {
    if (!m_visible || !m_hasTarget) return;

    DrawWindowBackground();

    const float margin = m_metrics.margin;
    float x = m_panelRect.x + margin;
    float y = m_panelRect.y + margin;

    // Tower name as header, with level indicator right-aligned
    Text::Draw(m_name.c_str(), static_cast<int>(x), static_cast<int>(y), m_metrics.fontHeader, GOLD, Text::Kind::Heading);
    if (m_hasAttack && m_upgradeCount > 0) {
        bool isMax = m_level >= m_upgradeCount;
        const char* lvlText = isMax ? "MAX" : TextFormat("Lv %d", m_level + 1);
        Color lvlColor = isMax ? GOLD : Hud::kTextMuted;
        Hud::DrawTextRightAligned(lvlText, m_panelRect.x + m_metrics.panelW - margin, y,
                                  m_metrics.fontHeader, lvlColor);
    }
    y += m_metrics.headerH;

    // Description (word-wrapped, computed in SetContent)
    for (const auto& line : m_descLines) {
        Text::Draw(line.c_str(), static_cast<int>(x), static_cast<int>(y), m_metrics.fontBody, Hud::kTextMuted, Text::Kind::Tooltip);
        y += m_descLineH;
    }

    // Stat rows from every module (AttackModule core stats + effect lines). Walls add nothing.
    y = Hud::DrawDescLines(m_statLines, x, y, m_metrics.lineH, m_metrics.fontBody);

    if (m_showUpgrade) {
        Hud::DrawToggleableButton(m_upgradeBtn, m_upgradeReady, m_metrics.fontBody, Hud::kUpgradeReady);
        if (m_hasNextUpgrade && m_upgradeBtn.IsHovered())
            DrawUpgradeTooltip();
    }

    if (m_showTargeting) {
        m_targetBtn.Draw();
        m_targetBtn.DrawLabel(m_metrics.fontBody, Hud::kTargetLabel);
    }

    if (m_showSell)
        Hud::DrawToggleableButton(m_sellBtn, m_sellEnabled, m_metrics.fontBody, Hud::kSellLabel);
}

void TowerInfoHUD::DrawUpgradeTooltip() {
    const float margin = m_metrics.margin;
    const int   fontSm = m_metrics.fontBody;
    const float lineH  = m_metrics.lineH;

    // Box sized to the widest of the header and the preview lines
    int maxW = Text::Measure("Next Level", fontSm);
    for (const auto& line : m_upgradePreview)
        maxW = std::max(maxW, Text::Measure(line.m_text.c_str(), fontSm));

    float boxW = maxW + margin * 2.0f;
    float boxH = margin * 2.0f + static_cast<float>(m_upgradePreview.size() + 1) * lineH;

    // Prefer the left of the panel; flip to the right if it would clip off-screen
    float boxX = m_panelRect.x - boxW - m_sellGap;
    if (boxX < 0.0f)
        boxX = m_panelRect.x + m_metrics.panelW + m_sellGap;

    // Align with the upgrade button, clamped to stay on-screen vertically
    float boxY = m_upgradeBtn.m_rect.y;
    float screenH = static_cast<float>(m_screenH);
    if (boxY + boxH > screenH) boxY = screenH - boxH;
    if (boxY < 0.0f) boxY = 0.0f;

    Rectangle box = { boxX, boxY, boxW, boxH };
    Hud::DrawFramedBox(box, Hud::PanelBg(Hud::kTooltipBgAlpha), Hud::kTooltipBorder);

    float tx = boxX + margin;
    float ty = boxY + margin;
    Text::Draw("Next Level", static_cast<int>(tx), static_cast<int>(ty), fontSm, GOLD);
    ty += lineH;
    Hud::DrawDescLines(m_upgradePreview, tx, ty, lineH, fontSm);
}
