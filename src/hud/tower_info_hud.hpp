#pragma once

#include <hud/hud.hpp>
#include <hud/hud_views.hpp>
#include <hud/hud_theme.hpp>
#include <engine/features/ui_widgets.hpp>
#include <raylib.h>
#include <string>
#include <vector>

class Input;

class TowerInfoHUD : public HUD {
public:
    void Build(float scale);

    // Point the panel at a tower (or hover preview) described by a read-only view, position it
    // near the view's screen anchor, and show it.
    void SetTarget(const TowerInfoView& view);

    void ProcessInput(Input& input);
    void Draw();

    bool WasSellRequested() { return m_sellSignal.Consume(); }
    bool WasTargetingCycleRequested() { return m_targetSignal.Consume(); }
    bool WasUpgradeRequested() { return m_upgradeSignal.Consume(); }

private:
    // Shared panel metrics plus the extras unique to this panel.
    Hud::PanelMetrics m_metrics;
    float m_descLineH = 0.0f;
    float m_sellH     = 0.0f;
    float m_sellGap   = 0.0f;
    float m_anchorGap = 0.0f;

    // Content snapshot taken in SetTarget (no Tower/Enemy references kept).
    bool m_hasTarget = false;
    std::string m_name;
    bool m_hasAttack = false;
    int  m_level = 0;
    int  m_upgradeCount = 0;
    std::vector<std::string> m_descLines;
    std::vector<DescLine> m_statLines;
    std::string m_targetingName;
    int m_screenH = 0;

    Button m_sellBtn;
    Button m_targetBtn;
    Button m_upgradeBtn;
    HudSignal m_sellSignal;
    HudSignal m_targetSignal;
    HudSignal m_upgradeSignal;
    bool m_showSell = true;
    bool m_sellEnabled = true;       // sellable only between waves; greyed out during a wave
    bool m_showTargeting = false;
    bool m_showUpgrade = false;
    bool m_upgradeReady = false;    // affordable and not yet max level
    bool m_hasNextUpgrade = false;  // an unpurchased upgrade level exists
    std::vector<DescLine> m_upgradePreview; // delta lines for the next upgrade

    // SetTarget splits into a content snapshot (+ word-wrap) and a geometry pass.
    void SetContent(const TowerInfoView& view);
    void Layout(const TowerInfoView& view);

    void DrawUpgradeTooltip();
};
