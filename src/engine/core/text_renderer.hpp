#pragma once

#include <raylib.h>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class FileStore;

// ============================================================================
// TextRenderer — GPU-accelerated vector text renderer
// ============================================================================
//
// HarfBuzz shaping + Slug coverage rendering via harfbuzz-gpu. Glyph outlines
// are encoded once into a GPU atlas and rasterized per-fragment, so text stays
// sharp at any scale and under any camera zoom — no pre-baked bitmap atlas,
// no FreeType.
//
// Fully independent of raylib's DrawText; respects the active raylib
// matrices (BeginMode2D camera, render texture viewport) and blend state
// is restored after each draw.
class TextRenderer {
public:
    // Opaque font handle; valid for the lifetime of the owning TextRenderer.
    using FontId = std::uint32_t;

    // Creates shader and GPU glyph atlas. Requires an active raylib window (GL 3.3).
    static std::expected<std::unique_ptr<TextRenderer>, std::string> Create();

    ~TextRenderer();

    // Non-copyable / non-movable — owns GL and HarfBuzz handles
    TextRenderer(const TextRenderer&)            = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    // Loads a TTF/OTF face directly through HarfBuzz (hb_face_create).
    // path must point to a valid font file; glyphs are encoded lazily on first use.
    std::expected<FontId, std::string> LoadFont(std::string_view path);

    // Loads a TTF/OTF face from a memory blob (e.g. font data embedded in the binary).
    // data must stay valid and unmodified for the renderer's lifetime; size > 0.
    std::expected<FontId, std::string> LoadFontFromMemory(const void* data, std::size_t size);

    // Shapes UTF-8 text (kerning, ligatures, multi-line via '\n') and draws it
    // with the top-left corner at position; each line's cap height is centered
    // within its fontSize-tall box. font must come from LoadFont.
    void DrawText(FontId font, std::string_view text, Vector2 position, float fontSize, Color color);

    // Logical extents of the shaped text (width of widest line, total height).
    Vector2 MeasureText(FontId font, std::string_view text, float fontSize);

private:
    TextRenderer();

    std::expected<FontId, std::string> LoadFontBlob(struct hb_blob_t* blob);

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// ============================================================================
// Text — global text drawing facade
// ============================================================================
//
// Global text drawing used by all game code. Wraps a TextRenderer with embedded
// fonts (EB Garamond, Victor Mono) baked into the binary at build time.
// Signatures are drop-in for raylib's DrawText/MeasureText; if the GPU path is
// unavailable (web builds, shader failure) it falls back to raylib's bitmap font
// with a warning.
//
// Game code must not call raylib's DrawText/MeasureText directly — always go
// through Text:: (or the draw helpers below).

namespace Text {

// Semantic text roles. Each maps to a font name in the [fonts] table of config/hud.toml; several
// kinds may resolve to the same font. Body is the default for general UI copy.
enum class Kind { Title, Heading, Body, Label, Number, Button, Tooltip };

// Number of Kind values — indexes the per-kind font table.
inline constexpr int KindCount = 7;

// Creates the renderer, reads the [fonts] table of config/hud.toml via fileStore, and resolves each
// Kind to a font (file override in resources/fonts/ or an embedded default); warns
// and arms the raylib font fallback on failure. Requires an initialized window.
void Init(FileStore& fileStore);

// Releases GPU resources. Must run before CloseWindow(); Draw/Measure
// fall back to raylib afterwards.
void Shutdown();

// Draws UTF-8 text (multi-line via '\n') with its top-left corner at (x, y).
void Draw(const char* text, int x, int y, int fontSize, Color color, Kind kind = Kind::Body);

// Width in pixels of the widest line of text at fontSize.
int Measure(const char* text, int fontSize, Kind kind = Kind::Body);

// Greedy word-wrap: split UTF-8 text into lines no wider than maxWidth pixels at fontSize.
std::vector<std::string> Wrap(const std::string& text, float maxWidth, int fontSize,
                              Kind kind = Kind::Body);

} // namespace Text

// ============================================================================
// Draw helpers — primitive drawing conveniences
// ============================================================================
//
// Shared by states and HUDs. Operate on raw virtual coordinates (no HUD scaling)
// and depend only on raylib + the Text facade.

#include <algorithm>

// Draw text horizontally centered on centerX, with its top at y.
inline void DrawCenteredText(const char* text, float centerX, float y, int fontSize, Color color,
                             Text::Kind kind = Text::Kind::Body) {
    int w = Text::Measure(text, fontSize, kind);
    Text::Draw(text, static_cast<int>(centerX - w / 2.0f), static_cast<int>(y), fontSize, color, kind);
}

// Vertically center a label inside a row of the given height, left-aligned at x.
inline void DrawLabelInRow(const char* text, float x, float rowY, float rowH, int fontSize, Color color,
                           Text::Kind kind = Text::Kind::Body) {
    Text::Draw(text, static_cast<int>(x), static_cast<int>(rowY + (rowH - fontSize) / 2.0f), fontSize, color, kind);
}

// Trim text with a trailing ellipsis so it fits within maxWidth pixels.
inline std::string TruncateToWidth(const std::string& text, int fontSize, float maxWidth,
                                   Text::Kind kind = Text::Kind::Body) {
    if (Text::Measure(text.c_str(), fontSize, kind) <= maxWidth) return text;
    std::string out = text;
    while (!out.empty() && Text::Measure((out + "...").c_str(), fontSize, kind) > maxWidth)
        out.pop_back();
    return out + "...";
}

// Draw a texture aspect-fitted (letterboxed) inside region, centered. No-op on an empty texture.
inline void DrawTextureFitted(const Texture2D& tex, Rectangle region) {
    if (tex.id == 0 || tex.width <= 0 || tex.height <= 0) return;
    float scale = std::min(region.width / tex.width, region.height / tex.height);
    float w = tex.width * scale;
    float h = tex.height * scale;
    Rectangle dst = {region.x + (region.width - w) / 2.0f,
                     region.y + (region.height - h) / 2.0f, w, h};
    Rectangle src = {0.0f, 0.0f, static_cast<float>(tex.width), static_cast<float>(tex.height)};
    DrawTexturePro(tex, src, dst, {0.0f, 0.0f}, 0.0f, WHITE);
}

// Exact component-wise equality for raylib Color (which has no built-in operator==).
inline bool ColorEquals(Color a, Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
