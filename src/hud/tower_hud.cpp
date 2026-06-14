#include <hud/tower_hud.hpp>
#include <hud/hud_theme.hpp>
#include <engine/core/text_renderer.hpp>
#include <engine/core/input.hpp>
#include <engine/core/resources.hpp>
#include <raylib.h>
#include <algorithm>

// ============================================================================
// TowerBuildHUD — docked bottom build bar
// ============================================================================

void TowerBuildHUD::Build(float scale, int screenW, int screenH, const std::vector<TowerBuildOption>& options) {
    HUD::Build(scale);
    m_metrics = Hud::PanelMetrics::Make(scale);
    const float btnSize = m_metrics.Scaled(Hud::g_buildCfg.btnSize);
    const float panelH  = m_metrics.Scaled(Hud::g_buildCfg.panelH);
    const float margin  = m_metrics.margin;
    const float gap     = m_metrics.Scaled(Hud::g_buildCfg.gap);
    // Glyph and label offsets scale with the button size they sit inside.
    m_iconYOffset = m_metrics.Scaled(Hud::g_buildCfg.btnSize * Hud::kBuildIconYToBtn);
    m_nameYOffset = m_metrics.Scaled(Hud::g_buildCfg.btnSize * Hud::kBuildNameYToBtn);
    m_costYOffset = m_metrics.Scaled(Hud::g_buildCfg.btnSize * Hud::kBuildCostYToBtn);

    float y = screenH - btnSize - margin;
    m_panelRect = { 0.0f, screenH - panelH, static_cast<float>(screenW), panelH };

    m_buttons.clear();
    for (size_t i = 0; i < options.size(); i++) {
        BuildButton entry;
        entry.m_button.m_label = options[i].m_name;
        entry.m_button.m_rect = { margin + i * (btnSize + gap), y, btnSize, btnSize };
        entry.m_textureKey = options[i].m_textureKey;
        entry.m_cost = options[i].m_cost;
        m_buttons.push_back(std::move(entry));
    }

    m_selectedTower = "";
}

void TowerBuildHUD::ProcessInput(Input& input) {
    Vector2 mousePos{};
    bool pressed = false;
    if (!BeginInput(input, mousePos, pressed)) return;

    for (BuildButton& entry : m_buttons) {
        entry.m_button.Update(mousePos, pressed);
        if (entry.m_button.IsClicked()) {
            PlayClickSound();
            // Toggle: clicking the active type again clears the selection
            m_selectedTower = (m_selectedTower == entry.m_button.m_label) ? "" : entry.m_button.m_label;
            break;
        }
    }
}

const std::string& TowerBuildHUD::GetHoveredTower(Vector2 mousePos) const {
    static const std::string empty;
    for (const auto& entry : m_buttons)
        if (CheckCollisionPointRec(mousePos, entry.m_button.m_rect))
            return entry.m_button.m_label;
    return empty;
}

Vector2 TowerBuildHUD::GetHoveredButtonTopCenter(Vector2 mousePos) const {
    for (const auto& entry : m_buttons)
        if (CheckCollisionPointRec(mousePos, entry.m_button.m_rect))
            return { entry.m_button.m_rect.x + entry.m_button.m_rect.width / 2.0f, entry.m_button.m_rect.y };
    return {};
}

void TowerBuildHUD::Draw(const BuildBarView& view, Resources& assets) {
    if (!m_visible) return;

    DrawDockedBackground();

    int fontSize = m_metrics.fontSmall;
    for (const BuildButton& entry : m_buttons) {
        const Button& btn = entry.m_button;
        const std::string& name = btn.m_label;
        bool selected = (name == m_selectedTower);

        btn.Draw(selected);

        Texture2D& tex = assets.GetTexture(entry.m_textureKey);
        float tw = static_cast<float>(tex.width);
        float th = static_cast<float>(tex.height);
        DrawTextureV(tex, { btn.m_rect.x + (btn.m_rect.width  - tw) / 2.0f,
                            btn.m_rect.y + (btn.m_rect.height - th) / 2.0f - m_iconYOffset }, Hud::kIconTint);

        int centerX = static_cast<int>(btn.m_rect.x + btn.m_rect.width / 2.0f);
        DrawCenteredText(name.c_str(), static_cast<float>(centerX),
            btn.m_rect.y + btn.m_rect.height - m_nameYOffset,
            fontSize, Hud::kTextSecondary);

        const char* costStr = TextFormat("$%d", entry.m_cost);
        Color costColor = (view.m_gold >= entry.m_cost) ? Hud::kStatusPositive : Hud::kStatusNegative;
        DrawCenteredText(costStr, static_cast<float>(centerX),
            btn.m_rect.y + btn.m_rect.height - m_costYOffset,
            fontSize, costColor, Text::Kind::Number);
    }
}

// ============================================================================
// TowerInfoHUD — inspect / upgrade panel
// ============================================================================

void TowerInfoHUD::Build(float scale) {
    HUD::Build(scale);
    m_metrics = Hud::PanelMetrics::Make(scale);
    m_metrics.panelW = m_metrics.Scaled(Hud::g_infoCfg.width);
    // Line height and the action-button height/gap follow the body font and shared margin.
    m_descLineH  = m_metrics.Scaled(Hud::kFontBodyBase * Hud::kInfoDescLineToBody);
    m_sellH      = m_metrics.Scaled(Hud::kFontBodyBase * Hud::kInfoSellHToBody);
    m_sellGap    = m_metrics.Scaled(Hud::kMarginBase * Hud::kInfoSellGapToMargin);
    m_anchorGap  = m_metrics.Scaled(Hud::g_infoCfg.anchorGap);
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
    Text::Draw(m_name.c_str(), static_cast<int>(x), static_cast<int>(y), m_metrics.fontHeader, Hud::kTextHeader, Text::Kind::Heading);
    if (m_hasAttack && m_upgradeCount > 0) {
        bool isMax = m_level >= m_upgradeCount;
        const char* lvlText = isMax ? "MAX" : TextFormat("Lv %d", m_level + 1);
        Color lvlColor = isMax ? Hud::kHighlight : Hud::kTextSecondary;
        Hud::DrawTextRightAligned(lvlText, m_panelRect.x + m_metrics.panelW - margin, y,
                                  m_metrics.fontHeader, lvlColor);
    }
    y += m_metrics.headerH;

    // Description (word-wrapped, computed in SetContent)
    for (const auto& line : m_descLines) {
        Text::Draw(line.c_str(), static_cast<int>(x), static_cast<int>(y), m_metrics.fontBody, Hud::kTextSecondary, Text::Kind::Tooltip);
        y += m_descLineH;
    }

    // Stat rows from every module (AttackModule core stats + effect lines). Walls add nothing.
    y = Hud::DrawDescLines(m_statLines, x, y, m_metrics.lineH, m_metrics.fontBody);

    if (m_showUpgrade) {
        Hud::DrawToggleableButton(m_upgradeBtn, m_upgradeReady, m_metrics.fontBody, Hud::kStatusPositive);
        if (m_hasNextUpgrade && m_upgradeBtn.IsHovered())
            DrawUpgradeTooltip();
    }

    if (m_showTargeting) {
        m_targetBtn.Draw();
        m_targetBtn.DrawLabel(m_metrics.fontBody, Hud::kAccent);
    }

    if (m_showSell)
        Hud::DrawToggleableButton(m_sellBtn, m_sellEnabled, m_metrics.fontBody, Hud::kStatusPositive);
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
    Hud::DrawFramedBox(box, Hud::PanelBg(Hud::kTooltipBgAlpha), Hud::kHighlight);

    float tx = boxX + margin;
    float ty = boxY + margin;
    Text::Draw("Next Level", static_cast<int>(tx), static_cast<int>(ty), fontSm, Hud::kTextHeader);
    ty += lineH;
    Hud::DrawDescLines(m_upgradePreview, tx, ty, lineH, fontSm);
}
