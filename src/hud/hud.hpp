#pragma once
#include <raylib.h>
#include <engine/core/text_renderer.hpp>

class Input;
class SoundSystem;

// One-shot flag: raised on an event, consumed exactly once.
struct HudSignal {
    void Raise() { m_pending = true; }
    bool Consume() { bool p = m_pending; m_pending = false; return p; }
private:
    bool m_pending = false;
};

void DrawTextCenteredX(const char* text, int centerX, int y, int fontSize, Color color,
                       Text::Kind kind = Text::Kind::Body);

// Base for every HUD component: shared scaling, panel helpers, and visibility state. Concrete
// HUDs expose their own typed ProcessInput/Draw methods fed by read-only views (see hud_views.hpp)
// and an Input& for clicks — they never receive Game or query gameplay state directly.
class HUD {
public:
    virtual ~HUD() = default;

    // Called once on state enter; concrete HUDs call this then do their own layout.
    void Build(float scale) { m_scale = scale; }

    void Show() { m_visible = true; }
    void Hide() { m_visible = false; }
    bool IsVisible() const { return m_visible; }

    void SetSoundSystem(SoundSystem* ss) { m_soundSystem = ss; }

protected:
    float Scaled(float base) const { return base * m_scale; }
    int   ScaledInt(float base) const { return static_cast<int>(base * m_scale); }

    void PlayClickSound() const;

    void DrawPanelBackground(unsigned char alpha, bool border = false) const;
    // Semantic panel-class backgrounds (see hud_theme.hpp): primary windows share one opaque,
    // bordered look; docked bars share a lighter, border-less look. Both build on DrawPanelBackground.
    void DrawWindowBackground() const;
    void DrawDockedBackground() const;
    void ConsumePanelClick(Input& input) const;
    void ClampPanelToScreen(int screenW, int screenH);

    // Shared ProcessInput preamble: returns false (so the caller bails) when hidden;
    // otherwise reads the cursor + left-click state and swallows any panel click.
    bool BeginInput(Input& input, Vector2& mousePos, bool& pressed);
    // Click-only variant for HUDs that just need to swallow clicks on their panel.
    bool BeginInput(Input& input);

    float m_scale = 1.0f;
    bool m_visible = true;
    Rectangle m_panelRect = {};
    SoundSystem* m_soundSystem = nullptr;
};
