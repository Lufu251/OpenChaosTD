#pragma once

#include <raylib.h>
#include <engine/core/text_renderer.hpp>
#include <string>

// Transient status message ("toast") shown by editor/settings states: set a message,
// tick it down each frame, and draw it centered while it is still alive. Replaces the
// hand-rolled m_status/m_statusTimer pair those states each carried.
class StatusToast {
public:
    void Set(const std::string& msg) { m_text = msg; m_timer = kDuration; }
    void Update(float dt) { if (m_timer > 0.0f) m_timer -= dt; }
    bool Active() const { return m_timer > 0.0f; }

    // Draw the message horizontally centered on centerX with its top at y; no-op once expired.
    void Draw(float centerX, float y, int fontSize, Color color) const {
        if (m_timer > 0.0f)
            DrawCenteredText(m_text.c_str(), centerX, y, fontSize, color);
    }

private:
    static constexpr float kDuration = 2.5f; // seconds a message stays on screen
    std::string m_text;
    float m_timer = 0.0f;
};
