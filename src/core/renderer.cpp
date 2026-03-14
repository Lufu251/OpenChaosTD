#include <core/renderer.hpp>
#include <stdexcept>

// Init / Shutdown
void Renderer::Init(int virtualWidth, int virtualHeight) {
    m_virtualWidth  = virtualWidth;
    m_virtualHeight = virtualHeight;

    m_target = LoadRenderTexture(m_virtualWidth, m_virtualHeight);
    if (m_target.id == 0)
        throw std::runtime_error("Renderer: failed to create RenderTexture");

    SetTextureFilter(m_target.texture, TEXTURE_FILTER_BILINEAR);

    UpdateScale();
}

void Renderer::Shutdown() {
    if (m_target.id != 0) {
        UnloadRenderTexture(m_target);
        m_target = {};
    }
}

// Resize
void Renderer::OnResize() {
    UpdateScale();
}

// UpdateScale
// Calculates the largest rectangle that fits the virtual resolution inside the current screen
void Renderer::UpdateScale() {
    const float screenW = static_cast<float>(GetScreenWidth());
    const float screenH = static_cast<float>(GetScreenHeight());

    const float scaleX = screenW / static_cast<float>(m_virtualWidth);
    const float scaleY = screenH / static_cast<float>(m_virtualHeight);
    m_scale = (scaleX < scaleY) ? scaleX : scaleY; // fit inside, preserve ratio

    const float scaledW = m_virtualWidth  * m_scale;
    const float scaledH = m_virtualHeight * m_scale;

    // Center the game image
    const float offsetX = (screenW - scaledW) * 0.5f;
    const float offsetY = (screenH - scaledH) * 0.5f;

    m_destRect = { offsetX, offsetY, scaledW, scaledH };

    // Black bar rectangles (one pair: top+bottom or left+right)
    m_barA = { 0.0f,            0.0f,   screenW, offsetY };  // top   or left
    m_barB = { 0.0f, offsetY + scaledH, screenW, offsetY };  // bottom or right
}

// Frame
void Renderer::BeginFrame() {
    // Recalculate every frame to handle live window resizing
    UpdateScale();

    BeginTextureMode(m_target);
        ClearBackground(BLACK);
}

void Renderer::EndFrame() {
    EndTextureMode();

    BeginDrawing();
        ClearBackground(BLACK); // fills letterbox bars

        // RenderTexture is stored upside-down in OpenGL — flip Y with negative height
        const Rectangle src = {
            0.0f, 0.0f,
            static_cast<float>(m_virtualWidth),
            static_cast<float>(-m_virtualHeight)  // negative = flip
        };

        DrawTexturePro(m_target.texture, src, m_destRect, { 0.0f, 0.0f }, 0.0f, WHITE);

    EndDrawing();
}

// Virtual mouse
Vector2 Renderer::GetVirtualMouse() const {
    Vector2 mouse = GetMousePosition();

    // Subtract letterbox offset, then divide by scale
    mouse.x = (mouse.x - m_destRect.x) / m_scale;
    mouse.y = (mouse.y - m_destRect.y) / m_scale;

    // Clamp to virtual bounds
    if (mouse.x < 0.0f) mouse.x = 0.0f;
    if (mouse.y < 0.0f) mouse.y = 0.0f;
    if (mouse.x > static_cast<float>(m_virtualWidth))  mouse.x = static_cast<float>(m_virtualWidth);
    if (mouse.y > static_cast<float>(m_virtualHeight)) mouse.y = static_cast<float>(m_virtualHeight);

    return mouse;
}