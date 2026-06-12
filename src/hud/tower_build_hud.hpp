#pragma once

#include <hud/hud.hpp>
#include <hud/hud_views.hpp>
#include <hud/hud_theme.hpp>
#include <engine/systems/ui_widgets.hpp>
#include <raylib.h>
#include <vector>
#include <string>

class Input;
class Resources;

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

    // Shared scaling + typographic scale; button grid geometry stays local to Build().
    Hud::PanelMetrics m_metrics;
    float m_iconYOffset = 0.0f; // sprite nudge above the button center
    float m_nameYOffset = 0.0f; // name baseline above the button bottom
    float m_costYOffset = 0.0f; // cost baseline above the button bottom
};
