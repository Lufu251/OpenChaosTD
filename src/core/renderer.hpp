#pragma once

#include <raylib.h>

class Renderer {
public:
    Renderer() = default;
    ~Renderer() { Shutdown(); }

    // Non-copyable — owns a RenderTexture2D handle
    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Call after InitWindow()
    void Init(int virtualWidth, int virtualHeight);

    // Call when the window is resized
    void OnResize();

    // Wraps all game drawing — call BeginFrame() / EndFrame() each loop
    void BeginFrame();
    void EndFrame();

    // Converts real screen mouse coords to virtual game coords
    Vector2 GetVirtualMouse() const;

    int GetGameWidth()  const { return m_virtualWidth;  }
    int GetGameHeight() const { return m_virtualHeight; }

    void Shutdown();

private:
    // Recalculates the destination rectangle and black bar fills
    void UpdateScale();

    int m_virtualWidth  = 1280;
    int m_virtualHeight = 720;

    RenderTexture2D m_target = {};  // Game renders into this

    // Destination rect on the real screen (letterboxed)
    Rectangle m_destRect  = {};

    // Bars to fill with black (either top/bottom or left/right)
    Rectangle m_barA      = {};
    Rectangle m_barB      = {};

    float m_scale     = 1.0f;
};