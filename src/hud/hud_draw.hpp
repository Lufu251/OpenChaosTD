#pragma once

#include <raylib.h>
#include <vector>
#include <engine/core/text_renderer.hpp>
#include <engine/systems/ui_widgets.hpp>
#include <world/desc_line.hpp>

// Stateless HUD draw helpers for idioms that recur across panels: a filled+bordered box, a
// right-aligned label, and a column of DescLine rows. These take all geometry as arguments so any
// panel (or free function) can use them without sharing state.
namespace Hud {

// Fill a rectangle and stroke a 1px border in the given colors.
void DrawFramedBox(Rectangle rect, Color fill, Color border);

// Draw `text` so its right edge sits at `rightEdge` (measure + subtract width).
void DrawTextRightAligned(const char* text, float rightEdge, float y, int fontSize, Color color,
                          Text::Kind kind = Text::Kind::Body);

// Draw each line at (x, y), advancing y by lineH per row. Returns the y past the last line.
float DrawDescLines(const std::vector<DescLine>& lines, float x, float y, float lineH, int fontSize);

// Draw a button that may be disabled: default vs. muted style, with an active-colored label when
// enabled and a uniform DARKGRAY label when not.
void DrawToggleableButton(const Button& btn, bool enabled, int fontSize, Color enabledColor);

// Draw a button that toggles between an active highlight (gold selected border + active-colored
// label) and a normal look. Used for stateful toggles like the Status bar Speed/Auto buttons.
void DrawHighlightButton(const Button& btn, bool highlighted, int fontSize,
                         Color activeColor, Color normalColor);

// Draw one ephemeral toast: fill `bg` with the standard overlay panel color and draw `text` at
// (textX, textY), both scaled by `fade` (0..1) so toasts fade out uniformly.
void DrawOverlayToast(const char* text, Rectangle bg, float textX, float textY, int fontSize,
                      float fade);

} // namespace Hud
