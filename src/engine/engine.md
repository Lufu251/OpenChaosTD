# Engine

Reusable infrastructure shared across all game states and systems. Located under `src/engine/` and split into four subfolders by role.

```
engine/
├── core/       Windowing, rendering, input, resource loading, text
├── systems/    Optional gameplay-adjacent systems (particles, sound, UI)
├── util/       File I/O and profiling helpers
└── lib/        Generic data structures with no engine dependencies
```

---

## core/

### Resources

Loads and caches raylib assets by string key. Supports textures, sounds, fonts, and music. Assets are freed in `Shutdown()`, called automatically by the destructor.

```cpp
m_resources.SetAssetPath(path);

// Single file
m_resources.LoadTexture("tower_zapper", "textures/tower_zapper.png");
Texture2D& tex = m_resources.GetTexture("tower_zapper");

// Bulk load — key = filename stem for each file in the directory
m_resources.LoadTexturesFromDir("textures");
m_resources.LoadMusicFromDir("music");
```

Retrieval throws if the key is not found. Use `HasTexture` / `HasSound` / etc. to guard optional lookups.

OGG Vorbis is the recommended format for music streaming; WAV is best used with `LoadSound` for short one-shot effects.

---

### Screen

Renders the game into a fixed virtual resolution and scales it to fill the window with letterboxing via a custom projection matrix (no intermediate render texture — vector text stays crisp). The virtual size is set once at `Init()` and never changes; only the destination rectangle recalculates on resize. Internal letterbox math (formerly split into `letterbox.hpp`) is kept private inside `screen.cpp`.

```cpp
// game loop
if (IsWindowResized()) m_screen.OnResize();
m_screen.BeginFrame();
    // all game drawing here
m_screen.EndFrame();
```

`GetVirtualMouse()` converts real screen mouse coordinates into virtual game coordinates, accounting for letterbox offset and scale. All input should use this rather than raylib's `GetMousePosition()` directly.

---

### Input

Action-based input that maps named actions to keyboard keys via `keybindings.json`. Unknown action names return `false` silently.

Mouse buttons are queried directly via raw accessors — they are not part of the binding system and are not user-remappable.

```cpp
// keyboard action querying
if (input.IsPressed("Cancel")) { ... }

// raw mouse button querying
if (input.IsMousePressed(MOUSE_LEFT_BUTTON))  { ... }
if (input.IsMouseDown(MOUSE_RIGHT_BUTTON))    { ... }
if (input.IsMouseReleased(MOUSE_LEFT_BUTTON)) { ... }
float wheel = input.GetMouseWheelDelta();
```

Mouse input consumption prevents clicks from passing through HUD elements to the world:

```cpp
input.ConsumeMouseInput();        // called by HUD
input.IsMouseInputConsumed();     // checked by world interactions
```

Consumed state resets at the start of each frame in `Update()`.

---

### TextRenderer + Text facade

Single module (`text_renderer.hpp/.cpp`) containing the GPU vector text renderer, the global `Text::` facade, and drawing convenience helpers.

**TextRenderer** — GPU-accelerated vector text, independent of raylib's `DrawText`. Text is shaped by HarfBuzz (kerning, ligatures, complex scripts) and rasterized per-fragment on the GPU with the Slug algorithm (harfbuzz-gpu), so it stays pixel-sharp at any size and under any `Camera2D` zoom — no bitmap font atlas. Fonts are plain TTF/OTF files loaded directly through HarfBuzz; FreeType is not involved.

Errors are reported via `std::expected`; creation requires an open window (OpenGL 3.3 desktop — not available on web builds).

```cpp
auto text = TextRenderer::Create();             // once, after InitWindow()
auto font = (*text)->LoadFont("resources/fonts/some.ttf");

// In the draw loop — respects BeginMode2D / render texture transforms.
(*text)->DrawText(*font, "Hello\nworld", {x, y}, 32, BLACK);
Vector2 size = (*text)->MeasureText(*font, "Hello", 32);
```

Fonts can also be loaded from memory (`LoadFontFromMemory`), e.g. font data embedded in the binary — the data must outlive the renderer.

Glyph outlines are encoded lazily on first use and cached in a GPU texel-buffer atlas (8 MiB, ~thousands of glyphs); shaping runs per call. `tools/text_demo.cpp` (`text_demo` target) is a standalone showcase.

**Text facade** — global text drawing used by all game code (`Text::` namespace). Wraps a `TextRenderer` with embedded fonts (EB Garamond, Victor Mono) baked into the binary at build time. Signatures are drop-in for raylib's `DrawText`/`MeasureText`; if the GPU path is unavailable (web builds, shader failure) it falls back to raylib's bitmap font with a warning.

```cpp
Text::Init();      // Game ctor, after InitWindow()
Text::Shutdown();  // Game dtor, before CloseWindow()

Text::Draw("Gold: 42", x, y, fontSize, GOLD);   // top-left anchored, '\n' multi-line
int w = Text::Measure("Gold: 42", fontSize);     // widest-line width in px
```

**Draw helpers** — inline convenience functions also in `text_renderer.hpp`:

```cpp
DrawCenteredText("Title", centerX, y, fontSize, color);      // horizontally centered
DrawLabelInRow("Label", x, rowY, rowH, fontSize, color);     // vertically centered in row
std::string t = TruncateToWidth(longText, fontSize, maxW);   // trailing ellipsis
DrawTextureFitted(tex, region);                              // aspect-fitted in region
bool eq = ColorEquals(a, b);                                 // component-wise color compare
```

Game code must not call raylib's `DrawText`/`MeasureText` directly — always go through `Text::` or the helpers above.

---

## systems/

### ParticleSystem

CPU particle system backed by a fixed-capacity `ObjectPool<Particle>` (2048 slots). Owned by `Game` alongside the other core managers — not part of game state.

`EmitterDesc` configures both spawning paths; `Particle` and `Emitter` are internal runtime types. There are two ways to spawn:

- **Bursts** — fire-and-forget. `Emit` spawns `count` particles once at a position.
- **Continuous emitters** — engine-owned, timer-driven. The system keeps a `DenseSlotMap<Emitter>` of live emitters that spawn `count` particles `emitRate` times per second from `Tick`. The owner registers one, then refreshes its anchor every frame so it can follow a moving source (e.g. a status effect on an enemy). Each emitter has a short keepalive that is reset by `UpdateEmitter`; if the owner stops updating it (effect expired, enemy dead), it self-removes within ~0.15 s, so explicit removal is optional.

```cpp
// --- Bursts ---
game.GetParticles().Emit(position, desc);                 // one-shot
game.GetParticles().Emit(position, desc, sourceVelocity); // base velocity added to each particle

// --- Continuous emitters ---
EmitterHandle h = game.GetParticles().AddEmitter(desc, position, sourceVelocity);
game.GetParticles().UpdateEmitter(h, newPosition, newVelocity); // each frame; refreshes keepalive
game.GetParticles().RemoveEmitter(h);                           // optional prompt removal

// Drive from the game loop (advances emitters + integrates particles)
game.GetParticles().Tick(dt);
game.GetParticles().Draw();

// Clear all live particles and emitters (call alongside GameData::Reset)
game.GetParticles().Clear();
```

**Motion model.** Each particle's velocity each frame is the sum of three components, all relative to a per-particle `center` anchor captured at spawn:

- **linear** — the cone direction (`angle` ± `spread`) at `speed`, plus any base velocity;
- **radial** — `radialSpeed` along `normalize(position − center)` (outward if positive);
- **tangential** — `tangentialSpeed` perpendicular to the radial direction (spin around the anchor).

**Spawn shapes** offset each particle from the anchor at spawn while keeping `center` at the anchor, so radial/tangential math stays correct for shaped spawns. `Point` (default), `Line` (`shapeWidth` length along `angle`), `Box` (`shapeWidth` × `shapeHeight`), `Circle` (filled, `shapeRadius`), `Ring` (on the `shapeRadius` circumference).

`EmitterDesc` fields — all angles in degrees:

| Field | Default | Meaning |
|---|---|---|
| `color` / `endColor` | WHITE / transparent | Start and end tint, lerped over lifetime |
| `count` | 0 | Particles per burst / per emission step; 0 = disabled for `Emit` |
| `speed` / `speedVariance` | 50 / 20 | Ejection speed ± random |
| `angle` | 0 | Centre direction (0=right, 90=down, 180=left, 270=up) |
| `spread` | 360 | Total arc of the spawn cone in degrees |
| `lifetime` | 0.2 | Seconds each particle lives |
| `size` / `endSize` | 3 / 0 | Radius at birth and death, lerped over lifetime |
| `shape` | Point | Spawn shape: Point / Line / Box / Circle / Ring |
| `shapeSize` (`shapeWidth`/`shapeHeight`) | 0 / 0 | Box extents; Line uses width as length |
| `shapeRadius` | 0 | Circle (max) / Ring (fixed) radius |
| `radialSpeed` | 0 | Outward (+) / inward (−) speed relative to the anchor |
| `tangentialSpeed` | 0 | Spin speed around the anchor |
| `emitRate` | 0 | Continuous emitter spawns/sec; 0 = burst-only |

Presets are defined in `data/particle_effects.toml` and loaded by `EmitterPresets` (`src/factory/`); the TOML keys match the field names above (`shapeWidth`/`shapeHeight` for the shape extents).

---

### SoundSystem

One-track music streamer plus fire-and-forget sound effects. Owned by `Game`, initialised after resources are loaded. `Tick` must be called every frame to feed the active music stream.

```cpp
// Startup (called once in Game::LoadResources after LoadMusicFromDir)
game.GetSoundSystem().Init(m_resources);

// Music — one track at a time; starting a new track stops the previous one
game.GetSoundSystem().PlayMusic("openchaostd_main");
game.GetSoundSystem().PauseMusic();
game.GetSoundSystem().ResumeMusic();
game.GetSoundSystem().StopMusic();

// Sound effects — fire-and-forget, played at the current sfx volume
game.GetSoundSystem().PlaySfx("explosion");

// Volume — [0.0, 1.0]; music volume applies immediately to the active track
game.GetSoundSystem().SetMusicVolume(0.8f);
game.GetSoundSystem().SetSfxVolume(0.5f);

// Game loop — must run every frame
game.GetSoundSystem().Tick(dt);
```

---

### UI Widgets

Immediate-mode GUI widgets for menus and HUDs. Provides `Button`, `Slider`, `Toggle`, `TextInput`, `ProgressBar`, and `ScrollableList` with two built-in style presets (`kDefaultStyle`, `kDisabledStyle`). All widgets render through the `Text::` facade.

---

## util/

### FileStore

Cross-platform file read/write for JSON (nlohmann/json) and TOML (toml++). The root path is set once and all subsequent paths are resolved relative to it. Also provides `FormatFloat` for compact decimal serialization (trailing zeros stripped).

```cpp
m_fileStore.SetRootPath(projectRoot);

// JSON (keybindings, save games)
auto j = m_fileStore.LoadJson("config/keybindings.json");
m_fileStore.SaveJson("data/save.json", j);

// TOML (settings, game data)
auto tbl = m_fileStore.LoadToml("config/settings.toml");
m_fileStore.SaveToml("data/save.toml", tbl);

bool exists = m_fileStore.Exists("config/settings.toml");
```

Used by all factories and config types. On web builds, path resolution accounts for the Emscripten virtual filesystem.

---

### Profiler

Named scope profiler using a rolling window of samples. Tracks average, last, and peak frame times per scope. Results are printed to stdout on shutdown.

```cpp
m_profiler.Begin("Update");
    // ...
m_profiler.End("Update");

double avg = m_profiler.GetAvgMs("Update");
```

The window size defaults to 120 frames (2 s at 60 fps).

---

## lib/

Header-only data structures with no engine or game dependencies.

### DenseSlotMap

Stable-ID container with dense storage optimised for fast iteration. Handles are stable across insertions and removals. Used for towers, enemies, and the ParticleSystem's live emitters.

```cpp
DenseSlotMap<Tower> towers;
auto key = towers.Insert(tower);
Tower& t = towers.Get(key);
towers.Erase(key);
for (auto& t : towers) { ... }
```

### ObjectPool

Fixed-capacity pool with O(1) allocate and release. Release uses swap-and-pop, so iteration order is not stable. Used by `ParticleSystem`.

### Grid2D

Resizable 2D array backed by a flat `std::vector`. Used for the tile map and the BFS path mesh.

```cpp
Grid2D<Tile> grid(width, height);
grid.At(x, y) = Tile{...};
```
