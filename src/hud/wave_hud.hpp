#pragma once

#include <hud/hud.hpp>
#include <hud/hud_theme.hpp>
#include <raylib.h>
#include <string>
#include <vector>

class Input;
class Resources;

// Scaled layout metrics for an enemy card, supplied by WaveHUD from its own constants.
struct CardMetrics {
    float pad = 6.0f;       // inner padding of the card
    float iconSize = 44.0f; // square sprite tile on the left
    float lineH = 14.0f;    // row height for the header and each stat line
    int   fontSm = 11;
};

// One enemy entry in the WaveHUD "Next Wave" panel: a sprite tile on the left, a header row
// (count + name on the left, level badge right-aligned), then one stat row per enemy module.
// Content comes from a read-only WaveEnemyEntry, so the card never touches an Enemy directly.
struct WaveEnemyCard {
    // Fill content + layout metrics from a precomputed wave entry.
    void SetContent(const WaveEnemyEntry& entry, const CardMetrics& metrics);

    // Height the card needs: pad + max(icon tile, header row + stat rows) + pad.
    float Measure() const;

    // Draw the frame, icon backing + sprite, header row, and stat rows within `bounds`.
    void Draw(Rectangle bounds, Resources& assets) const;

private:
    CardMetrics m_metrics;

    int m_count = 1;
    std::string m_name;
    int m_level = 1;
    bool m_hasProto = false;
    std::string m_textureKey;
    std::vector<DescLine> m_stats;
};

// Compact right-side panel summarising the upcoming wave: total budget plus a card per enemy type
// showing its sprite, current upgrade level, and fully-upgraded stats. Content comes from a
// read-only WaveView built by PlayingState. Hidden by default; toggled by the StatusHUD "Waves"
// button or the hotkey.
class WaveHUD : public HUD {
public:
    void Build(float scale, int screenW);

    // Flip visibility — driven by the Waves button signal and the WaveInfo hotkey.
    void Toggle() { if (m_visible) Hide(); else Show(); }

    void ProcessInput(Input& input);
    void Draw(const WaveView& view, Resources& assets);

private:
    // Shared panel metrics plus the card-specific extras.
    Hud::PanelMetrics m_metrics;
    float m_cardGap   = 6.0f; // vertical space between cards
    float m_cardPad   = 6.0f; // inner padding of a card
    float m_iconSize  = 44.0f; // square sprite area inside a card
    float m_topOffset = 42.0f; // pushes the panel below the top status bar
    int   m_screenW = 0;

    // Card list + panel rect, rebuilt from the view before drawing.
    std::vector<WaveEnemyCard> m_cards;

    // Rebuild the card list and size m_panelRect to fit; separates layout from rendering.
    void Rebuild(const WaveView& view);
};
