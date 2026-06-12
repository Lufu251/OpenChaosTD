#pragma once

#include <raylib.h>

// Pure DPI/letterbox geometry used internally by Screen.
struct Letterbox {
    Rectangle destRect; // virtual rect placed on the framebuffer, centered
    float     scale;    // framebuffer-pixels per virtual unit
};

// Letterboxed virtual-resolution presenter: maps the fixed virtual coordinate
// space onto the window via the projection matrix, so everything rasterizes at
// native resolution (no intermediate render texture — vector text stays crisp).
// DPI-aware: all letterbox math runs in framebuffer pixels (GetRenderWidth),
// and mouse input is bridged from logical points via the window DPI scale, so
// hit-testing stays aligned on fractional-scale (HiDPI) monitors.
class Screen {
public:
    // Call after InitWindow()
    void Init(int virtualWidth, int virtualHeight);

    // Call when the window is resized
    void OnResize();

    // Wraps all game drawing — call BeginFrame() / EndFrame() each loop
    void BeginFrame();
    void EndFrame();

    // Clips drawing to a rect in virtual coordinates (use instead of raylib's
    // BeginScissorMode inside a frame); EndScissor restores the letterbox clip.
    void BeginScissor(Rectangle virtualRect) const;
    void EndScissor() const;

    // Converts real screen mouse coords to virtual game coords
    Vector2 GetVirtualMouse() const;

    int GetGameWidth() const { return m_virtualWidth;  }
    int GetGameHeight() const { return m_virtualHeight; }

    float GetScale() const { return m_scale; }
    Rectangle GetDestRect() const { return m_destRect; }

private:
    // Recalculates the letterboxed destination rectangle and scale
    void UpdateScale();

    int m_virtualWidth = 1280;
    int m_virtualHeight = 720;

    Rectangle m_destRect = {};  // destination rect in framebuffer pixels (letterboxed)
    float m_scale = 1.0f;       // framebuffer-pixels per virtual unit
    float m_dpiScale = 1.0f;    // logical points -> framebuffer pixels (measured: render/screen)
};
