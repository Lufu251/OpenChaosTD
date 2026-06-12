# OpenChaosTD
An open-map 2D Tower Defense game written in C++ with raylib. Towers can be placed anywhere on a grid and enemies dynamically pathfind around them. Still a work in progress.

## Building

### Prerequisites
* **CMake** 3.22 or newer
* **C++23 compiler** (GCC, Clang, MSVC)
* **Git** (for fetching dependencies via FetchContent)
* **Emscripten SDK** (web builds only)
* **Python 3.8+** (web builds only)

### Desktop (Windows, macOS, Linux)
```bash
mkdir build
cd build
cmake ..
cmake --build .
```
The binary is output to `build/bin/`.

### Web (WebAssembly)
```bash
cd tools
./build_web.sh
```
A local HTTP server starts on port 8000. The URL will be shown in the terminal.

## Dependencies
All fetched automatically via CMake FetchContent — no manual installation needed.

| Library | Version | Purpose |
|---|---|---|
| [raylib](https://github.com/raysan5/raylib) | 6.0 | Window, rendering, input, audio |
| [toml++](https://github.com/marzer/tomlplusplus) | 3.4.0 | TOML parsing for game data files |
| [nlohmann/json](https://github.com/nlohmann/json) | 3.11.3 | JSON parsing for config files (settings, keybindings) |

## Modding
Towers, enemies, and waves are fully data-driven — no recompile needed to tweak balance or add content.
Each TOML file under `data/` has a companion `.md` schema doc under `docs/`:

| File | Docs | Configures |
|---|---|---|
| `data/towers.toml`  | [towers.md](docs/towers.md)   | Tower stats and attack/effect modules (incl. the armor chip-damage floor) |
| `data/enemies.toml` | [enemies.md](docs/enemies.md) | Enemy stats and modules (armor, shield, regen, split spacing, immunities, upgrades) |
| `data/waves.toml`   | [waves.md](docs/waves.md)     | Procedural wave generator: budget scaling models, boss/upgrade cadence, enemy pool |

## Media
*(Add a screenshot or GIF here)*

## Project Structure
```
OpenChaosTD/
├── resources/
│   ├── textures/               - Sprites for towers, enemies and tiles
│   ├── music/                  - Streaming background music (OGG recommended)
│   └── sounds/                 - One-shot sound effects
│
├── config/
│   ├── settings.json           - Window resolution, FPS, HUD scale, title, audio volumes
│   └── keybindings.json        - Input action bindings (rebindable)
│
├── datapacks/
│   └── default/
│       ├── pack.toml           - Datapack metadata (name, author, version, description, icon)
│       └── data/
│           ├── gameplay.toml           - Starting lives, gold, sell rate, auto-spawn delay
│           ├── towers.toml             - Tower type definitions (stats, modules, description)
│           ├── enemies.toml            - Enemy type definitions (stats, modules, description)
│           ├── waves.toml              - Procedural wave generator: budget scaling, boss/upgrade cadence, enemy pool
│           └── particle_effects.toml   - Named particle emitter presets
│
├── docs/
│   └── *.md                    - Modder schema docs for towers/enemies/waves TOML
│
└── src/
    ├── app/                    Bootstrap, global config, and runtime session state
    │   ├── main                - Entry point
    │   ├── game                - Game loop, state machine, manager accessors
    │   ├── game_config         - Window/display settings loaded from JSON
    │   ├── game_data           - Runtime world state + starting values
    │   └── game_paths          - Save-file path constants
    │
    ├── engine/                 Reusable engine infrastructure — see engine/engine.md
    │
    ├── content/                Datapack loading + data-driven entity construction from TOML
    │   ├── datapack            - Datapack metadata struct
    │   ├── datapack_registry   - Discovers and lists available datapacks
    │   ├── tower_factory       - Builds Tower instances from towers.toml
    │   ├── enemy_factory       - Builds Enemy instances from enemies.toml
    │   └── emitter_presets     - Loads named EmitterDesc presets from particle_effects.toml
    │
    ├── hud/                    In-game HUD elements
    │   ├── hud                 - HUD base class (visibility, scaling, panels) + view structs + shared draw helpers
    │   ├── hud_theme           - Shared palette, fonts, alphas, and panel metrics
    │   ├── status_hud          - Top bar: lives, gold, wave readout, start/auto/speed/waves buttons
    │   ├── tower_hud           - Build bar (tower selection) + floating info panel (stats, upgrade, sell, targeting)
    │   ├── wave_hud            - Side panel: next-wave preview (enemy cards + budget)
    │   ├── event_hud           - Message log with timed fade-out
    │   └── pause_hud           - Pause overlay: resume / restart / main menu
    │
    ├── states/                 Game screen state machine
    │   ├── game_state          - Abstract base (ProcessInput, Update, Draw)
    │   ├── menu_state          - Main menu
    │   ├── settings_state      - Settings / options menu
    │   ├── play_state          - Active gameplay
    │   ├── end_state           - Victory / game over screen
    │   ├── datapack_select_state - Datapack selection screen
    │   └── particle_editor_state - Live particle emitter editor
    │
    ├── world/                  Passive entity definitions and data (no systems/UI dependencies)
    │   ├── tower               - Tower entity: simulation state + presentation (texture, color, style, emitters)
    │   ├── enemy               - Enemy entity: simulation state + presentation (texture, death sound/burst)
    │   ├── modules             - Tower modules (Attack/Passive, ArmorPierce, Slow, Burn, Shred, Weakness, Stun, Crit, RampUp) + enemy modules (BaseStats, Regeneration, Armor, Immune, Shield, Split)
    │   ├── combat              - Attack object, attack style, and status effects (Burn, Slow, ArmorShred, Stun, Weakness)
    │   ├── upgrade             - Tower & enemy upgrade definitions (stat deltas + added modules)
    │   ├── map                 - Grid, nest/core placement, path construction
    │   ├── pathfinding         - Path-mesh data types (Node, WalkableMask)
    │   └── tile                - Tile type, walkable/buildable flags, terrain buff modifier
    │
    └── systems/                Per-frame game logic and processes
        ├── pathfinder          - BFS solver over an abstract walkable grid (Pathfinder::Solve -> distance/predecessor mesh)
        ├── map_generator       - Procedural map generation
        ├── serialization       - Map TOML files + save-game JSON (de)serialization
        ├── wave_manager        - Procedural budget-based wave generation, auto-spawn, victory detection
        ├── world_system        - Placement, spawning, game-over checks
        ├── tower_system        - Cooldowns, targeting, attack creation and resolution
        ├── enemy_system        - Movement, status effects, module ticking
        └── render_system       - Drawing map, entities, attacks, UI
```

## License
MIT — see [LICENSE](LICENSE) for details.
