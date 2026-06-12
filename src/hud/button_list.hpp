#pragma once

#include <hud/hud.hpp> // HudSignal
#include <engine/systems/ui_widgets.hpp>
#include <raylib.h>
#include <string>
#include <vector>

namespace Hud {

// A vertical stack of uniformly-styled buttons, each paired with a one-shot signal. Collapses the
// per-button declare / lay-out / update / draw boilerplate into list operations, so adding an entry
// is a single Add() call rather than edits across four methods.
struct ButtonList {
    struct Item {
        Button button;
        HudSignal signal;
    };
    std::vector<Item> items;

    // Append a labelled button; returns its index for later signal polling.
    int Add(const std::string& label);

    // Lay the buttons out as a centered vertical stack: button i at (x, firstY + spacing*i).
    void LayoutVertical(float x, float firstY, float w, float h, float spacing);

    // Update every button and raise the matching signal on a click. Sets `clicked` if any button
    // was clicked this frame, so the caller can play the click sound once.
    void Update(Vector2 mouse, bool pressed, bool& clicked);

    // Draw every button and its label in one shared style.
    void Draw(int fontSize, Color labelColor) const;

    // Consume the one-shot signal for item i.
    bool Consume(int i) { return items[i].signal.Consume(); }
};

} // namespace Hud
