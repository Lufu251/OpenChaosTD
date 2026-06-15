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
        entry.m_textureKey = options[i].m_textureKey;
        entry.m_cost = options[i].m_cost;
        m_buttons.push_back(std::move(entry));
    }

    // Lay out buttons with WidgetGroup.
    m_buildGroup.SetCount(static_cast<int>(m_buttons.size()));
    m_buildGroup.m_config.m_mode = WidgetGroupConfig::Mode::Horizontal;
    m_buildGroup.m_config.m_pack = WidgetGroupConfig::Pack::Start;
    m_buildGroup.m_config.m_align = WidgetGroupConfig::Align::Start;
    m_buildGroup.m_config.m_bounds = {margin, y, static_cast<float>(screenW) - 2.0f * margin, btnSize};
    m_buildGroup.m_config.m_defaultItemW = btnSize;
    m_buildGroup.m_config.m_defaultItemH = btnSize;
    m_buildGroup.m_config.m_gapX = gap;
    m_buildGroup.Layout();
    for (int i = 0; i < m_buildGroup.Count(); i++)
        m_buttons[i].m_button.m_rect = m_buildGroup[i].m_rect;

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
    int idx = m_buildGroup.HitTest(mousePos);
    if (idx >= 0 && idx < static_cast<int>(m_buttons.size()))
        return m_buttons[idx].m_button.m_label;
    return empty;
}

Vector2 TowerBuildHUD::GetHoveredButtonTopCenter(Vector2 mousePos) const {
    int idx = m_buildGroup.HitTest(mousePos);
    if (idx >= 0 && idx < static_cast<int>(m_buttons.size())) {
        Rectangle r = m_buildGroup[idx].m_rect;
        return { r.x + r.width / 2.0f, r.y };
    }
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
        float pad = m_metrics.margin;
        Rectangle iconRect = { btn.m_rect.x + pad,
                               btn.m_rect.y + pad,
                               btn.m_rect.width - 2.0f * pad,
                               btn.m_rect.height - m_nameYOffset - 2.0f * pad };
        DrawTextureFitted(tex, iconRect);

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
    // Auto-draw labels on the action buttons.
    m_sellBtn.m_fontSize = m_metrics.fontBody;
    m_targetBtn.m_fontSize = m_metrics.fontBody;
    m_targetBtn.m_labelColor = Hud::kAccent;
    m_upgradeBtn.m_fontSize = m_metrics.fontBody;
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

    // Lay config buttons bottom-up: Targeting at top, then Upgrade, then Sell at the bottom.
    float btnW = panelW - margin * 2.0f;
    m_upgradeReady = false;
    m_hasNextUpgrade = false;
    m_upgradePreview.clear();

    // Update labels first, then let WidgetGroup position the buttons.
    if (m_showSell)
        m_sellBtn.m_label = TextFormat("Sell: $%d", view.m_sellRefund);
    if (m_showUpgrade) {
        if (view.m_upgradeAtMax) {
            m_upgradeBtn.m_label = "Max Level";
        } else {
            m_upgradeBtn.m_label = TextFormat("Upgrade $%d", view.m_upgradeCost);
            m_upgradeReady = view.m_upgradeReady;
            m_hasNextUpgrade = true;
            m_upgradePreview = view.m_upgradePreview;
        }
    }
    if (m_showTargeting)
        m_targetBtn.m_label = TextFormat("Target: %s", m_targetingName.c_str());

    // Slot order: 0=Targeting (top), 1=Upgrade, 2=Sell (bottom).
    m_actionGroup.SetCount(3);
    m_actionGroup.SetSlotVisible(0, m_showTargeting);
    m_actionGroup.SetSlotVisible(1, m_showUpgrade);
    m_actionGroup.SetSlotVisible(2, m_showSell);
    m_actionGroup.m_config.m_mode = WidgetGroupConfig::Mode::Vertical;
    m_actionGroup.m_config.m_pack = WidgetGroupConfig::Pack::End;
    m_actionGroup.m_config.m_align = WidgetGroupConfig::Align::Stretch;
    m_actionGroup.m_config.m_bounds = {m_panelRect.x + margin, m_panelRect.y + margin, btnW, panelH - 2.0f * margin};
    m_actionGroup.m_config.m_defaultItemH = m_sellH;
    m_actionGroup.m_config.m_gapY = m_sellGap;
    m_actionGroup.Layout();

    if (m_showTargeting)
        m_targetBtn.m_rect = m_actionGroup[0].m_rect;
    if (m_showUpgrade)
        m_upgradeBtn.m_rect = m_actionGroup[1].m_rect;
    if (m_showSell)
        m_sellBtn.m_rect = m_actionGroup[2].m_rect;
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
        m_upgradeBtn.m_enabled = m_upgradeReady;
        m_upgradeBtn.m_labelColor = Hud::kStatusPositive;
        m_upgradeBtn.Draw();
        if (m_hasNextUpgrade && m_upgradeBtn.IsHovered())
            DrawUpgradeTooltip();
    }

    if (m_showTargeting)
        m_targetBtn.Draw();

    if (m_showSell) {
        m_sellBtn.m_enabled = m_sellEnabled;
        m_sellBtn.m_labelColor = Hud::kStatusPositive;
        m_sellBtn.Draw();
    }
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
