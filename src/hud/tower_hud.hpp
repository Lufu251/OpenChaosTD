#pragma once

#include <hud/hud.hpp>
#include <hud/hud_theme.hpp>
#include <engine/systems/ui_widgets.hpp>
#include <raylib.h>
#include <vector>
#include <string>

class Input;
class Resources;

// The two tower-related panels: the docked bottom build bar (TowerBuildHUD) and the floating
// inspect/upgrade panel shown for a selected or hovered tower (TowerInfoHUD). Both are driven by
// read-only views (see hud.hpp) and never touch a Tower/Enemy/GameData type directly.

// --- Build bar ---------------------------------------------------------------

// Static per-tower config for a build-bar button, captured once at Build time from the factory.
struct TowerBuildOption {
    std::string m_name;
    std::string m_textureKey;
    int m_cost = 0;
};

class TowerBuildHUD : public HUD {
public:
    void Build(float scale, int screenW, int screenH, const std::vector<TowerBuildOption>& options);

    const std::string& GetSelectedTower() const { return m_selectedTower; }
    void ClearSelection() { m_selectedTower = ""; }

    // Returns the name of the button under mousePos, or "" if none
    const std::string& GetHoveredTower(Vector2 mousePos) const;
    // Returns the top-center screen position of the button under mousePos
    Vector2 GetHoveredButtonTopCenter(Vector2 mousePos) const;

    void ProcessInput(Input& input);
    void Draw(const BuildBarView& view, Resources& assets);

private:
    struct BuildButton {
        Button m_button;
        std::string m_textureKey;
        int m_cost = 0;
    };
    std::vector<BuildButton> m_buttons;
    std::string m_selectedTower;
    WidgetGroup m_buildGroup;

    // Shared scaling + typographic scale; button grid geometry stays local to Build().
    Hud::PanelMetrics m_metrics;
    float m_iconYOffset = 0.0f; // sprite nudge above the button center
    float m_nameYOffset = 0.0f; // name baseline above the button bottom
    float m_costYOffset = 0.0f; // cost baseline above the button bottom
};

// --- Inspect / upgrade panel -------------------------------------------------

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
    WidgetGroup m_actionGroup;
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
