#pragma once

#include <hud/hud.hpp>
#include <hud/button_list.hpp>
#include <raylib.h>

class Input;

// Centered pause overlay: dims the screen so the world stays visible behind it and offers
// Resume / Settings / Save / Load / Restart / Main Menu. Hidden by default; PlayingState shows it
// while the simulation is paused. Each button raises a one-shot signal that PlayingState consumes.
class PauseHUD : public HUD {
public:
    void Build(float scale, int screenW, int screenH);

    void ProcessInput(Input& input);
    void Draw();

    bool WasResumeRequested()   { return m_buttons.Consume(kResume); }
    bool WasSettingsRequested() { return m_buttons.Consume(kSettings); }
    bool WasSaveRequested()     { return m_buttons.Consume(kSave); }
    bool WasLoadRequested()     { return m_buttons.Consume(kLoad); }
    bool WasRestartRequested()  { return m_buttons.Consume(kRestart); }
    bool WasMainMenuRequested() { return m_buttons.Consume(kMainMenu); }

private:
    // Stable indices into m_buttons, in stack order.
    enum : int { kResume, kSettings, kSave, kLoad, kRestart, kMainMenu };

    Hud::ButtonList m_buttons;
    int m_screenW = 0;
    int m_screenH = 0;
};
