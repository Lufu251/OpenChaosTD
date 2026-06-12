#include "text_renderer.hpp"

#if defined(PLATFORM_WEB)

// harfbuzz-gpu's GLSL 3.30 isamplerBuffer atlas path needs desktop GL 3.3.
struct TextRenderer::Impl {};
TextRenderer::TextRenderer() = default;
TextRenderer::~TextRenderer() = default;

std::expected<std::unique_ptr<TextRenderer>, std::string> TextRenderer::Create() {
    return std::unexpected("TextRenderer is not supported on web builds");
}
std::expected<TextRenderer::FontId, std::string> TextRenderer::LoadFont(std::string_view) {
    return std::unexpected("TextRenderer is not supported on web builds");
}
std::expected<TextRenderer::FontId, std::string> TextRenderer::LoadFontFromMemory(const void*, std::size_t) {
    return std::unexpected("TextRenderer is not supported on web builds");
}
void TextRenderer::DrawText(FontId, std::string_view, Vector2, float, Color) {}
Vector2 TextRenderer::MeasureText(FontId, std::string_view, float) { return {0, 0}; }

#else

#include <raymath.h>
#include <rlgl.h>

// raylib's bundled GL loader; the function pointers are loaded by raylib at
// InitWindow() and shared with this translation unit. Needed only for the
// glyph atlas (integer texel buffer), which rlgl has no abstraction for.
#include <external/glad.h>

#include <hb-gpu.h>
#include <hb-ot.h>
#include <hb.h>

#include <algorithm>
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace {

// ---- GLSL entry points wrapped around the bundled harfbuzz-gpu sources ----

constexpr const char* kGlslVersion = "#version 330\n";

constexpr const char* kVertexMain = R"glsl(
in vec2 a_position;
in vec2 a_texcoord;
in vec2 a_normal;
in float a_emPerPos;
in float a_glyphLoc;

uniform mat4 u_matViewProjection;
uniform vec2 u_viewport;

out vec2 v_texcoord;
flat out uint v_glyphLoc;

void main()
{
    vec2 pos = a_position;
    vec2 tex = a_texcoord;

    // em-to-object is uniform scaling with y-flip; jac is its inverse.
    vec4 jac = vec4(a_emPerPos, 0.0, 0.0, -a_emPerPos);

    hb_gpu_dilate(pos, tex, a_normal, jac, u_matViewProjection, u_viewport);

    gl_Position = u_matViewProjection*vec4(pos, 0.0, 1.0);
    v_texcoord = tex;
    v_glyphLoc = uint(a_glyphLoc + 0.5);
}
)glsl";

constexpr const char* kFragmentMain = R"glsl(
uniform vec4 u_foreground;

in vec2 v_texcoord;
flat in uint v_glyphLoc;

out vec4 fragColor;

void main()
{
    float cov = hb_gpu_draw(v_texcoord, v_glyphLoc);
    fragColor = vec4(u_foreground.rgb*u_foreground.a, u_foreground.a)*cov;

    // Stem darkening on the edge coverage only keeps small text legible.
    if (cov > 0.0 && cov < 1.0)
    {
        float brightness = dot(u_foreground.rgb, vec3(1.0/3.0));
        float ppem = 1.0/max(fwidth(v_texcoord).x, fwidth(v_texcoord).y);
        fragColor *= hb_gpu_stem_darken(cov, brightness, ppem)/cov;
    }
}
)glsl";

// Matches the attribute layout consumed by kVertexMain.
struct GlyphVertex {
    float x, y;        // object-space position (screen units, y-down)
    float tx, ty;      // em-space texture coordinates (font units, y-up)
    float nx, ny;      // object-space outward normal
    float emPerPos;    // font units per object-space unit
    float glyphLoc;    // atlas texel offset of the encoded glyph blob
};

// Per-glyph encode result, cached per font.
struct GlyphEntry {
    int minX, minY, maxX, maxY;  // ink extents in font units, y-up
    int advance;                 // horizontal advance in font units
    unsigned atlasOffset;        // texel offset into the atlas
    bool empty;                  // no outline (space) or atlas overflow
};

struct FontEntry {
    hb_face_t* face = nullptr;
    hb_font_t* font = nullptr;
    int upem     = 0;
    int baseline = 0;  // font units from line top to baseline
    std::unordered_map<hb_codepoint_t, GlyphEntry> glyphs;
};

constexpr unsigned kAtlasTexelSize = 8;            // sizeof RGBA16I texel
constexpr unsigned kAtlasCapacityTexels = 1u << 20;  // 8 MiB of glyph data
constexpr int kAtlasTextureSlot = 7;               // clear of raylib's batch slots

} // namespace

struct TextRenderer::Impl {
    Shader shader = {};
    int locMvp = -1, locViewport = -1, locForeground = -1, locAtlas = -1;
    int attrPosition = -1, attrTexcoord = -1, attrNormal = -1, attrEmPerPos = -1, attrGlyphLoc = -1;

    unsigned vao = 0, vbo = 0;
    int vboCapacity = 0;  // bytes

    unsigned atlasTex = 0, atlasBuf = 0;
    unsigned atlasCursor = 0;  // texels
    bool atlasOverflowWarned = false;

    hb_gpu_draw_t* encoder = nullptr;
    hb_buffer_t* buffer = nullptr;
    std::vector<FontEntry> fonts;
    std::vector<GlyphVertex> scratch;

    ~Impl() {
        for (FontEntry& f : fonts) {
            hb_font_destroy(f.font);
            hb_face_destroy(f.face);
        }
        hb_buffer_destroy(buffer);
        hb_gpu_draw_destroy(encoder);
        if (vao) rlUnloadVertexArray(vao);
        if (vbo) rlUnloadVertexBuffer(vbo);
        if (atlasTex) glDeleteTextures(1, &atlasTex);
        if (atlasBuf) glDeleteBuffers(1, &atlasBuf);
        if (shader.id) UnloadShader(shader);
    }

    // Uploads an encoded glyph blob; returns its texel offset or fails on overflow.
    std::expected<unsigned, std::string> AtlasAlloc(const char* data, unsigned lenBytes) {
        unsigned lenTexels = (lenBytes + kAtlasTexelSize - 1)/kAtlasTexelSize;
        if (atlasCursor + lenTexels > kAtlasCapacityTexels)
            return std::unexpected("glyph atlas full");

        unsigned offset = atlasCursor;
        atlasCursor += lenTexels;
        glBindBuffer(GL_TEXTURE_BUFFER, atlasBuf);
        glBufferSubData(GL_TEXTURE_BUFFER, offset*kAtlasTexelSize, lenBytes, data);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
        return offset;
    }

    // Encodes and uploads a glyph on first use; cached thereafter.
    const GlyphEntry& LookupGlyph(FontEntry& font, hb_codepoint_t gid) {
        auto it = font.glyphs.find(gid);
        if (it != font.glyphs.end()) return it->second;

        GlyphEntry entry = {};
        entry.advance = hb_font_get_glyph_h_advance(font.font, gid);
        entry.empty = true;

        hb_gpu_draw_clear(encoder);
        hb_gpu_draw_glyph(encoder, font.font, gid);
        hb_glyph_extents_t ext = {};
        hb_blob_t* blob = hb_gpu_draw_encode(encoder, &ext);
        unsigned len = blob ? hb_blob_get_length(blob) : 0;
        if (len > 0) {
            auto offset = AtlasAlloc(hb_blob_get_data(blob, nullptr), len);
            if (offset) {
                entry.minX = ext.x_bearing;
                entry.maxX = ext.x_bearing + ext.width;
                entry.maxY = ext.y_bearing;
                entry.minY = ext.y_bearing + ext.height;  // height is negative (y-up)
                entry.atlasOffset = *offset;
                entry.empty = false;
            } else if (!atlasOverflowWarned) {
                atlasOverflowWarned = true;
                TraceLog(LOG_WARNING, "TEXT: %s, further glyphs are dropped", offset.error().c_str());
            }
        }
        if (blob) hb_gpu_draw_recycle_blob(encoder, blob);

        return font.glyphs.emplace(gid, entry).first->second;
    }

    // Appends the two dilated triangles covering one glyph's ink box.
    void AppendGlyphQuad(const GlyphEntry& g, float penX, float penY, float scale) {
        GlyphVertex v[4];
        for (int ci = 0; ci < 4; ci++) {
            const int cx = (ci >> 1) & 1;
            const int cy = ci & 1;
            const float ex = static_cast<float>(cx ? g.maxX : g.minX);
            const float ey = static_cast<float>(cy ? g.maxY : g.minY);

            v[ci].x = penX + scale*ex;
            v[ci].y = penY - scale*ey;  // font units are y-up, screen is y-down
            v[ci].tx = ex;
            v[ci].ty = ey;
            v[ci].nx = cx ? 1.f : -1.f;
            v[ci].ny = cy ? -1.f : 1.f;
            v[ci].emPerPos = 1.0f/scale;
            v[ci].glyphLoc = static_cast<float>(g.atlasOffset);
        }
        scratch.insert(scratch.end(), {v[0], v[1], v[2], v[1], v[2], v[3]});
    }

    // Shapes one line (no '\n') and appends its glyph quads; returns the advance width.
    float ShapeLine(FontEntry& font, std::string_view line, float penX, float penY, float scale,
                    bool emitQuads) {
        hb_buffer_clear_contents(buffer);
        hb_buffer_add_utf8(buffer, line.data(), static_cast<int>(line.size()), 0, -1);
        hb_buffer_guess_segment_properties(buffer);
        hb_shape(font.font, buffer, nullptr, 0);

        unsigned count = 0;
        hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buffer, &count);
        hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buffer, nullptr);

        float x = penX;
        for (unsigned i = 0; i < count; i++) {
            if (emitQuads) {
                const GlyphEntry& g = LookupGlyph(font, infos[i].codepoint);
                if (!g.empty)
                    AppendGlyphQuad(g,
                                    x + scale*static_cast<float>(pos[i].x_offset),
                                    penY - scale*static_cast<float>(pos[i].y_offset),
                                    scale);
            }
            x += scale*static_cast<float>(pos[i].x_advance);
        }
        return x - penX;
    }

    // (Re)uploads scratch vertices, growing the VBO and rebinding attributes as needed.
    void UploadVertices() {
        const int sizeBytes = static_cast<int>(scratch.size()*sizeof(GlyphVertex));
        rlEnableVertexArray(vao);
        if (sizeBytes > vboCapacity) {
            if (vbo) rlUnloadVertexBuffer(vbo);
            vbo = rlLoadVertexBuffer(scratch.data(), sizeBytes, true);
            vboCapacity = sizeBytes;
            const int stride = sizeof(GlyphVertex);
            const auto bind = [stride](int loc, int comps, std::size_t offset) {
                if (loc < 0) return;
                const auto uloc = static_cast<unsigned>(loc);
                rlSetVertexAttribute(uloc, comps, RL_FLOAT, false, stride, static_cast<int>(offset));
                rlEnableVertexAttribute(uloc);
            };
            bind(attrPosition, 2, offsetof(GlyphVertex, x));
            bind(attrTexcoord, 2, offsetof(GlyphVertex, tx));
            bind(attrNormal, 2, offsetof(GlyphVertex, nx));
            bind(attrEmPerPos, 1, offsetof(GlyphVertex, emPerPos));
            bind(attrGlyphLoc, 1, offsetof(GlyphVertex, glyphLoc));
        } else {
            rlUpdateVertexBuffer(vbo, scratch.data(), sizeBytes, 0);
        }
        rlDisableVertexArray();
    }
};

TextRenderer::TextRenderer() : m_impl(std::make_unique<Impl>()) {}
TextRenderer::~TextRenderer() = default;

std::expected<std::unique_ptr<TextRenderer>, std::string> TextRenderer::Create() {
    if (!IsWindowReady())
        return std::unexpected("TextRenderer requires an initialized window");
    if (rlGetVersion() != RL_OPENGL_33 && rlGetVersion() != RL_OPENGL_43)
        return std::unexpected("TextRenderer requires OpenGL 3.3+ (texel buffer atlas)");

    auto renderer = std::unique_ptr<TextRenderer>(new TextRenderer());
    Impl& impl = *renderer->m_impl;

    const std::string vs = std::string(kGlslVersion) +
                           hb_gpu_shader_source(HB_GPU_SHADER_STAGE_VERTEX, HB_GPU_SHADER_LANG_GLSL) +
                           kVertexMain;
    const std::string fs = std::string(kGlslVersion) +
                           hb_gpu_shader_source(HB_GPU_SHADER_STAGE_FRAGMENT, HB_GPU_SHADER_LANG_GLSL) +
                           hb_gpu_draw_shader_source(HB_GPU_SHADER_STAGE_FRAGMENT, HB_GPU_SHADER_LANG_GLSL) +
                           kFragmentMain;

    impl.shader = LoadShaderFromMemory(vs.c_str(), fs.c_str());
    if (!IsShaderValid(impl.shader))
        return std::unexpected("failed to compile harfbuzz-gpu Slug shaders");

    impl.locMvp        = GetShaderLocation(impl.shader, "u_matViewProjection");
    impl.locViewport   = GetShaderLocation(impl.shader, "u_viewport");
    impl.locForeground = GetShaderLocation(impl.shader, "u_foreground");
    impl.locAtlas      = GetShaderLocation(impl.shader, "hb_gpu_atlas");
    impl.attrPosition  = rlGetLocationAttrib(impl.shader.id, "a_position");
    impl.attrTexcoord  = rlGetLocationAttrib(impl.shader.id, "a_texcoord");
    impl.attrNormal    = rlGetLocationAttrib(impl.shader.id, "a_normal");
    impl.attrEmPerPos  = rlGetLocationAttrib(impl.shader.id, "a_emPerPos");
    impl.attrGlyphLoc  = rlGetLocationAttrib(impl.shader.id, "a_glyphLoc");
    if (impl.locMvp < 0 || impl.locViewport < 0 || impl.locForeground < 0 ||
        impl.locAtlas < 0 || impl.attrPosition < 0 || impl.attrGlyphLoc < 0)
        return std::unexpected("harfbuzz-gpu shader is missing expected interface symbols");

    // Glyph atlas: an RGBA16I texel buffer holding the encoded Slug blobs.
    glGenBuffers(1, &impl.atlasBuf);
    glBindBuffer(GL_TEXTURE_BUFFER, impl.atlasBuf);
    glBufferData(GL_TEXTURE_BUFFER, kAtlasCapacityTexels*kAtlasTexelSize, nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
    glGenTextures(1, &impl.atlasTex);
    glActiveTexture(GL_TEXTURE0 + kAtlasTextureSlot);
    glBindTexture(GL_TEXTURE_BUFFER, impl.atlasTex);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA16I, impl.atlasBuf);
    glActiveTexture(GL_TEXTURE0);

    impl.vao = rlLoadVertexArray();
    if (!impl.atlasBuf || !impl.atlasTex || !impl.vao)
        return std::unexpected("failed to allocate GPU buffers for the glyph atlas");

    impl.encoder = hb_gpu_draw_create_or_fail();
    impl.buffer = hb_buffer_create();
    if (!impl.encoder || !hb_buffer_allocation_successful(impl.buffer))
        return std::unexpected("failed to allocate HarfBuzz GPU encoder");

    return renderer;
}

std::expected<TextRenderer::FontId, std::string> TextRenderer::LoadFont(std::string_view path) {
    hb_blob_t* blob = hb_blob_create_from_file_or_fail(std::string(path).c_str());
    if (!blob)
        return std::unexpected("failed to read font file: " + std::string(path));
    return LoadFontBlob(blob);
}

std::expected<TextRenderer::FontId, std::string> TextRenderer::LoadFontFromMemory(const void* data,
                                                                                  std::size_t size) {
    if (!data || size == 0)
        return std::unexpected("empty font data");
    hb_blob_t* blob = hb_blob_create(static_cast<const char*>(data), static_cast<unsigned>(size),
                                     HB_MEMORY_MODE_READONLY, nullptr, nullptr);
    return LoadFontBlob(blob);
}

// Takes ownership of blob (one reference).
std::expected<TextRenderer::FontId, std::string> TextRenderer::LoadFontBlob(hb_blob_t* blob) {
    hb_face_t* face = hb_face_create(blob, 0);
    hb_blob_destroy(blob);  // face holds its own reference
    if (hb_face_get_glyph_count(face) == 0) {
        hb_face_destroy(face);
        return std::unexpected("not a valid font file");
    }

    FontEntry entry;
    entry.face = face;
    entry.font = hb_font_create(face);  // default scale: upem, font units
    entry.upem = static_cast<int>(hb_face_get_upem(face));

    // Center the cap height inside the em box so a line of caps optically
    // fills [y, y+fontSize]. Raw ascenders are unreliable for placement —
    // e.g. Victor Mono declares 1.1 em — and would sink text in UI boxes.
    hb_position_t capHeight = 0;
    if (!hb_ot_metrics_get_position(entry.font, HB_OT_METRICS_TAG_CAP_HEIGHT, &capHeight) ||
        capHeight <= 0 || capHeight > entry.upem)
        capHeight = entry.upem*7/10;
    entry.baseline = (entry.upem + capHeight)/2;

    Impl& impl = *m_impl;
    impl.fonts.push_back(entry);
    return static_cast<FontId>(impl.fonts.size() - 1);
}

void TextRenderer::DrawText(FontId font, std::string_view text, Vector2 position,
                            float fontSize, Color color) {
    Impl& impl = *m_impl;
    if (font >= impl.fonts.size() || text.empty() || fontSize <= 0.0f || color.a == 0)
        return;
    FontEntry& fe = impl.fonts[font];
    const float scale = fontSize/static_cast<float>(fe.upem);

    impl.scratch.clear();
    float penY = position.y + scale*static_cast<float>(fe.baseline);
    for (std::string_view rest = text; !rest.empty() || rest.data() == text.data();) {
        const std::size_t nl = rest.find('\n');
        impl.ShapeLine(fe, rest.substr(0, nl), position.x, penY, scale, true);
        penY += fontSize;
        if (nl == std::string_view::npos) break;
        rest.remove_prefix(nl + 1);
    }
    if (impl.scratch.empty())
        return;

    // Glyph upload above may touch buffer bindings; flush raylib's pending
    // batch only now, right before taking over the pipeline state.
    rlDrawRenderBatchActive();

    const Matrix mvp = MatrixMultiply(MatrixMultiply(rlGetMatrixTransform(), rlGetMatrixModelview()),
                                      rlGetMatrixProjection());
    const float viewport[2] = {static_cast<float>(rlGetFramebufferWidth()),
                               static_cast<float>(rlGetFramebufferHeight())};
    const float foreground[4] = {static_cast<float>(color.r)/255.0f,
                                 static_cast<float>(color.g)/255.0f,
                                 static_cast<float>(color.b)/255.0f,
                                 static_cast<float>(color.a)/255.0f};
    const int atlasSlot = kAtlasTextureSlot;
    SetShaderValueMatrix(impl.shader, impl.locMvp, mvp);
    SetShaderValue(impl.shader, impl.locViewport, viewport, SHADER_UNIFORM_VEC2);
    SetShaderValue(impl.shader, impl.locForeground, foreground, SHADER_UNIFORM_VEC4);
    SetShaderValue(impl.shader, impl.locAtlas, &atlasSlot, SHADER_UNIFORM_INT);

    impl.UploadVertices();

    // rlSetBlendMode flushes the batch, which unbinds the current program —
    // it must come before rlEnableShader. Culling is disabled because the
    // quad winding flips under render-texture projections.
    rlSetBlendMode(RL_BLEND_ALPHA_PREMULTIPLY);
    rlDisableBackfaceCulling();
    rlEnableShader(impl.shader.id);
    glActiveTexture(GL_TEXTURE0 + kAtlasTextureSlot);
    glBindTexture(GL_TEXTURE_BUFFER, impl.atlasTex);
    glActiveTexture(GL_TEXTURE0);

    rlEnableVertexArray(impl.vao);
    rlDrawVertexArray(0, static_cast<int>(impl.scratch.size()));
    rlDisableVertexArray();
    rlDisableShader();
    rlEnableBackfaceCulling();
    rlSetBlendMode(RL_BLEND_ALPHA);
}

Vector2 TextRenderer::MeasureText(FontId font, std::string_view text, float fontSize) {
    Impl& impl = *m_impl;
    if (font >= impl.fonts.size() || text.empty() || fontSize <= 0.0f)
        return {0, 0};
    FontEntry& fe = impl.fonts[font];
    const float scale = fontSize/static_cast<float>(fe.upem);

    float width = 0;
    int lines = 0;
    for (std::string_view rest = text; !rest.empty() || rest.data() == text.data();) {
        const std::size_t nl = rest.find('\n');
        width = std::max(width, impl.ShapeLine(fe, rest.substr(0, nl), 0, 0, scale, false));
        lines++;
        if (nl == std::string_view::npos) break;
        rest.remove_prefix(nl + 1);
    }
    return {width, static_cast<float>(lines)*fontSize};
}

#endif // PLATFORM_WEB

// ============================================================================
// Text:: facade implementation — excluded from the standalone text_demo target
// which only exercises the low-level TextRenderer API.
// ============================================================================
#ifndef TEXT_DEMO_STANDALONE

#include <engine/util/file_store.hpp>

#include <toml++/toml.hpp>

#include <array>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// Generated at build time from the TTFs in resources/fonts/ (cmake/embed_resource.cmake)
extern const unsigned char gVictorMonoTtf[];
extern const std::size_t gVictorMonoTtfSize;
extern const unsigned char gEBGaramondTtf[];
extern const std::size_t gEBGaramondTtfSize;

namespace {

std::unique_ptr<TextRenderer> sRenderer;
std::array<TextRenderer::FontId, Text::KindCount> sKindFont{};
TextRenderer::FontId sFallbackFont = 0; // embedded EB Garamond; always valid when sRenderer

// Embedded fonts selectable by canonical name (the fallback when no file override exists).
struct Embedded {
    std::string_view  name;
    const unsigned char* data;
    const std::size_t*   size;
};
const Embedded kEmbedded[] = {
    {"EBGaramond", gEBGaramondTtf, &gEBGaramondTtfSize},
    {"VictorMono", gVictorMonoTtf, &gVictorMonoTtfSize},
};

const Embedded* FindEmbedded(std::string_view name) {
    for (const Embedded& e : kEmbedded)
        if (e.name == name)
            return &e;
    return nullptr;
}

// Common font-search location for both the GPU and raylib loaders. Kept in sync with
// CMake's FONTS_DIR and the web --preload-file mapping (resources@resources).
constexpr std::string_view kFontDir = "resources/fonts/";

// Resolves a font name to a FontId: resources/fonts/<name>.{ttf,otf} file override
// first (via FileStore), then an embedded canonical name, else the fallback font.
// Loaded fonts are cached so kinds sharing a name share one FontId.
TextRenderer::FontId ResolveFont(FileStore& fileStore, const std::string& name,
                                 std::unordered_map<std::string, TextRenderer::FontId>& cache) {
    if (auto it = cache.find(name); it != cache.end())
        return it->second;

    for (std::string_view ext : {".ttf", ".otf"}) {
        const std::string path = std::string(kFontDir) + name + std::string(ext);
        if (!fileStore.Exists(path))
            continue;
        // FullPath keeps path resolution inside FileStore; LoadFont reads the file.
        auto id = sRenderer->LoadFont(fileStore.FullPath(path));
        if (id) {
            cache.emplace(name, *id);
            return *id;
        }
        TraceLog(LOG_WARNING, "FONTS: failed to load '%s' (%s) — falling back", path.c_str(),
                 id.error().c_str());
        break;
    }

    if (const Embedded* e = FindEmbedded(name)) {
        auto id = sRenderer->LoadFontFromMemory(e->data, *e->size);
        if (id) {
            cache.emplace(name, *id);
            return *id;
        }
        TraceLog(LOG_WARNING, "FONTS: embedded '%s' failed (%s) — falling back",
                 std::string(name).c_str(), id.error().c_str());
    } else {
        TraceLog(LOG_WARNING, "FONTS: unknown font '%s' (no file or embedded match) — falling back",
                 name.c_str());
    }

    cache.emplace(name, sFallbackFont);
    return sFallbackFont;
}

TextRenderer::FontId FontFor(Text::Kind kind) {
    return sKindFont[static_cast<int>(kind)];
}

// --- font name configuration -------------------------------------------------
// One font name per Text::Kind: a file stem under resources/fonts/ ("MyFont" ->
// resources/fonts/MyFont.ttf) or an embedded canonical name ("EBGaramond"/"VictorMono").
// Read from config/fonts.toml; missing file/keys keep the defaults. This is an
// implementation detail of the text subsystem (formerly the standalone FontConfig),
// so it lives here rather than as a public core header.
using FontNames = std::array<std::string, Text::KindCount>;

FontNames DefaultFontNames() {
    FontNames d;
    d.fill("EBGaramond");                                    // prose everywhere...
    d[static_cast<int>(Text::Kind::Number)] = "VictorMono";  // ...mono only for numbers
    return d;
}

FontNames LoadFontNames(FileStore& fileStore) {
    static constexpr const char* kKeys[Text::KindCount] = {
        "title", "heading", "body", "label", "number", "button", "tooltip",
    };
    FontNames names = DefaultFontNames();
    if (!fileStore.Exists("config/fonts.toml"))
        return names; // keep defaults

    const toml::table table = fileStore.LoadToml("config/fonts.toml");
    const toml::table* fonts = table["fonts"].as_table();
    if (fonts == nullptr) {
        TraceLog(LOG_WARNING, "FONTS: config/fonts.toml has no [fonts] table — using defaults");
        return names;
    }
    for (int k = 0; k < Text::KindCount; ++k)
        if (auto name = (*fonts)[kKeys[k]].value<std::string>(); name && !name->empty())
            names[k] = *name;
    return names;
}

// --- raylib bitmap fallback --------------------------------------------------
// The GPU (Slug) renderer needs desktop GL 3.3 and is unavailable on web. Rather
// than drop to raylib's built-in default font, we load the real project TTFs as
// raylib Fonts and draw them with DrawTextEx, so the typeface still looks right.

bool sRaylibFallback = false;
constexpr int kRayFontBaseSize = 64;     // rasterization size; DrawTextEx scales from here
std::array<Font, Text::KindCount> sKindRayFont{};
std::vector<Font> sOwnedRayFonts;        // distinct loaded fonts to UnloadFont() on shutdown

// Loads a raylib Font for a name: resources/fonts/<name>.{ttf,otf} from the (possibly
// preloaded) VFS first, then the embedded bytes, else raylib's default font. Cached so
// kinds sharing a name share one Font; newly loaded fonts are tracked for cleanup.
Font ResolveRayFont(FileStore& fileStore, const std::string& name,
                    std::unordered_map<std::string, Font>& cache) {
    if (auto it = cache.find(name); it != cache.end())
        return it->second;

    Font font{};
    bool loaded = false;
    for (std::string_view ext : {".ttf", ".otf"}) {
        const std::string path = std::string(kFontDir) + name + std::string(ext);
        if (!fileStore.Exists(path))
            continue;
        font = ::LoadFontEx(fileStore.FullPath(path).c_str(), kRayFontBaseSize, nullptr, 0);
        loaded = font.texture.id != 0 && font.glyphCount > 0;
        break;
    }
    if (!loaded) {
        if (const Embedded* e = FindEmbedded(name)) {
            font = ::LoadFontFromMemory(".ttf", e->data, static_cast<int>(*e->size),
                                        kRayFontBaseSize, nullptr, 0);
            loaded = font.texture.id != 0 && font.glyphCount > 0;
        }
    }
    if (!loaded) {
        TraceLog(LOG_WARNING, "FONTS(raylib): '%s' unavailable — using default font", name.c_str());
        font = ::GetFontDefault();
    } else {
        ::SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR); // smooth when up/down-scaled
        sOwnedRayFonts.push_back(font);
    }
    cache.emplace(name, font);
    return font;
}

// Builds the per-kind raylib Font table from the same font names the GPU path uses.
void InitRaylibFallback(FileStore& fileStore, const FontNames& names) {
    std::unordered_map<std::string, Font> cache;
    for (int k = 0; k < Text::KindCount; ++k)
        sKindRayFont[k] = ResolveRayFont(fileStore, names[k], cache);
    sRaylibFallback = true;
}

} // namespace

namespace Text {

void Init(FileStore& fileStore) {
    const FontNames fonts = LoadFontNames(fileStore);

    auto renderer = TextRenderer::Create();
    if (!renderer) {
        // GPU path unavailable (e.g. web): use real TTFs via raylib instead of DrawText.
        TraceLog(LOG_WARNING, "TEXT: %s — using raylib font fallback", renderer.error().c_str());
        InitRaylibFallback(fileStore, fonts);
        return;
    }
    sRenderer = std::move(*renderer);

    // Guaranteed fallback typeface: embedded EB Garamond. If even this fails the
    // GPU path is unusable, so disarm it and use the raylib font fallback instead.
    auto fallback = sRenderer->LoadFontFromMemory(gEBGaramondTtf, gEBGaramondTtfSize);
    if (!fallback) {
        TraceLog(LOG_WARNING, "TEXT: embedded font failed (%s) — using raylib font fallback",
                 fallback.error().c_str());
        sRenderer.reset();
        InitRaylibFallback(fileStore, fonts);
        return;
    }
    sFallbackFont = *fallback;

    std::unordered_map<std::string, TextRenderer::FontId> cache;
    cache.emplace("EBGaramond", sFallbackFont);

    for (int k = 0; k < KindCount; ++k)
        sKindFont[k] = ResolveFont(fileStore, fonts[k], cache);
}

void Shutdown() {
    sRenderer.reset();
    if (sRaylibFallback) {
        for (Font& f : sOwnedRayFonts) // distinct fonts only; GetFontDefault() was never tracked
            ::UnloadFont(f);
        sOwnedRayFonts.clear();
        sKindRayFont = {};
        sRaylibFallback = false;
    }
}

void Draw(const char* text, int x, int y, int fontSize, Color color, Kind kind) {
    if (sRenderer) {
        sRenderer->DrawText(FontFor(kind), text,
                            {static_cast<float>(x), static_cast<float>(y)},
                            static_cast<float>(fontSize), color);
        return;
    }
    if (sRaylibFallback) {
        ::DrawTextEx(sKindRayFont[static_cast<int>(kind)], text,
                     {static_cast<float>(x), static_cast<float>(y)},
                     static_cast<float>(fontSize), 0.0f, color);
        return;
    }
    ::DrawText(text, x, y, fontSize, color);
}

int Measure(const char* text, int fontSize, Kind kind) {
    if (sRenderer) {
        const Vector2 size = sRenderer->MeasureText(FontFor(kind), text, static_cast<float>(fontSize));
        return static_cast<int>(size.x + 0.5f);
    }
    if (sRaylibFallback) {
        const Vector2 size = ::MeasureTextEx(sKindRayFont[static_cast<int>(kind)], text,
                                             static_cast<float>(fontSize), 0.0f);
        return static_cast<int>(size.x + 0.5f);
    }
    return ::MeasureText(text, fontSize);
}

std::vector<std::string> Wrap(const std::string& text, float maxWidth, int fontSize, Kind kind) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string word, current;
    while (stream >> word) {
        std::string candidate = current.empty() ? word : current + " " + word;
        if (Measure(candidate.c_str(), fontSize, kind) <= static_cast<int>(maxWidth))
            current = candidate;
        else {
            if (!current.empty()) lines.push_back(current);
            current = word;
        }
    }
    if (!current.empty()) lines.push_back(current);
    return lines;
}

} // namespace Text

#endif // TEXT_DEMO_STANDALONE
