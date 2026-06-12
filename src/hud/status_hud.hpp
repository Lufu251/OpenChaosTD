#pragma once

#include <hud/hud.hpp>
#include <hud/hud_views.hpp>
#include <hud/hud_theme.hpp>
#include <engine/features/ui_widgets.hpp>
#include <raylib.h>

class Input;

class StatusHUD : public HUD {
public:
    void Build(float scale, int screenW);

    void ProcessInput(Input& input, const StatusView& view);
    void Draw(const StatusView& view);

    bool WasWaveRequested()    { return m_waveSignal.Consume(); }
    bool WasAutoToggled()      { return m_autoSignal.Consume(); }
    bool WasSpeedToggled()     { return m_speedSignal.Consume(); }
    bool WasWaveInfoToggled()  { return m_waveInfoSignal.Consume(); }

private:
    // Center readout: wave progress + win target, drawn centered on centerX. Endless mode
    // shows the infinity glyph in place of a target wave.
    void DrawWaveReadout(const StatusView& view, int centerX);
    // Hand-drawn infinity glyph (the default font lacks U+221E). Spans [x, x + 1.2*h] horizontally,
    // vertically centered on yMid.
    void DrawInfinity(float x, float yMid, float h, Color color) const;

    Button m_startWaveBtn;
    Button m_autoBtn;
    Button m_speedBtn;
    Button m_waveInfoBtn;
    HudSignal m_waveSignal;
    HudSignal m_autoSignal;
    HudSignal m_speedSignal;
    HudSignal m_waveInfoSignal;

    // Shared scaling + typographic scale; the bar's own button widths stay local in Build().
    Hud::PanelMetrics m_metrics;
    int m_textY = 10;
};
