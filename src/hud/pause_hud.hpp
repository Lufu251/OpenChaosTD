#pragma once

#include <hud/hud.hpp>
#include <hud/hud_theme.hpp>
#include <engine/systems/ui_widgets.hpp>
#include <raylib.h>
#include <vector>

class Input;

// Centered pause overlay: dims the screen so the world stays visible behind it and offers
// Resume / Settings / Save / Load / Restart / Main Menu. Hidden by default; PlayingState shows it
// while the simulation is paused. Each button raises a one-shot signal that PlayingState consumes.
class PauseHUD : public HUD {
public:
    void Build(float scale, int screenW, int screenH);

    void ProcessInput(Input& input);
    void Draw();

    bool WasResumeRequested()   { return Consume(kResume); }
    bool WasSettingsRequested() { return Consume(kSettings); }
    bool WasSaveRequested()     { return Consume(kSave); }
    bool WasLoadRequested()     { return Consume(kLoad); }
    bool WasRestartRequested()  { return Consume(kRestart); }
    bool WasMainMenuRequested() { return Consume(kMainMenu); }

private:
    bool Consume(int i) { bool v = m_raised[i]; m_raised[i] = false; return v; }

    // Stable indices into m_pauseButtons, in stack order.
    enum : int { kResume, kSettings, kSave, kLoad, kRestart, kMainMenu, kCount };

    std::vector<Button> m_pauseButtons;
    WidgetGroup m_pauseGroup;
    std::vector<bool> m_raised; // one-shot click signals, one per button
    Hud::PanelMetrics m_metrics;
    float m_titleOffset = 0.0f; // title baseline below the panel top
    int m_screenW = 0;
    int m_screenH = 0;
};
