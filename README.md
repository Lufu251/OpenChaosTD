# OpenChaosTD
This is an open-map 2D Tower Defense game made in c++ with raylib that is still a work in progress. Towers can be placed anywhere on a grid, and enemies will search for the best path.



## Building

### Prerequisites
* **CMake** (3.15 or newer)
* **C++ Compiler** (e.g.,GCC, Clang, MSVC)
* **Git** (for fetching dependencies)
* **Emscripten SDK** (only for web builds)
* **Python 3.8+** (only for web builds)

### Desktop Build (Windows, macOS, Linux)
1. **Configure and build with CMake:**
    ```bash
    mkdir build
    cd build
    cmake ..
    cmake --build .
    ```
    The example binaries will be located in the `build/bin` directory.

### Web Build (WebAssembly)
1.  **Configure and build with Emscripten**
    ```bash
    cd tools
    ./build_web.sh
    ```
    Now there will be a HTTP Server running under port 8000 to test your project. The URL should be visible in the active terminal.

## Media
* (Add a screenshot or GIF of your project here)

## Project Structure
```
OpenChaosTD/
└── src/                                Root
    ├── main.cpp                        - Run game
    ├── game.hpp / game.cpp             - Window creation, FPS, resizing
    │
    ├── states/                         Divide screens into individual states
    │   ├── game_state.hpp              - Base class (input, logic, drawing)
    │   ├── menu_state.hpp/.cpp         - Pre playing state
    │   ├── play_state.hpp/.cpp         - Playing state
    │   ├── gameover_state.hpp/.cpp     - Gameover state display score and start new ✏️
    │   └── victory_state.hpp/.cpp      - Victory state display score ✏️
    │
    ├── core/                           Core engine responsible for global functionality
    │   ├── Grid2D.h                    - Header-only resizable 2D array template
    │   ├── asset_manager.hpp/.cpp      - Load/cache textures, sounds, fonts
    │   ├── renderer.hpp/.cpp           - Rendering and letterbox scaling
    │   ├── input_manager.hpp/.cpp      - Keybinding, virtual mouse and mouse consumption
    │   ├── jsonio.hpp/.cpp             - Cross-platform JSON read/write
    │   └── PerformanceMonitor.h/.cpp   - Performance profiling avg, last, peak timings
    │
    ├── world/                          Grid, wave and paths
    │   ├── map.hpp/.cpp                - Grid2D, path calculation
    │   ├── tile.hpp                    - Bass class tile
    │   └── wave_manager.hpp/.cpp       - Spawn timing, wave definitions ✏️
    │
    ├── entities/                       Entities only hold data
    │   ├── entity.hpp                  - Base class ✏️
    │   ├── enemy.hpp/.cpp              - Health, speed ✏️
    │   ├── tower.hpp/.cpp              - Range, damage, fire rate ✏️
    │   └── projectile.hpp/.cpp         - Damage, status ✏️
    │
    ├── systems/                        Implement lokal functionality
    │   ├── entity_system.hpp/.cpp      - Controling behaviour of entities ✏️
    │   └── render_system.hpp/.cpp      - Drawing entities ✏️
    │
    └── ui/                             User interface
        ├── hud.hpp/.cpp                - Lives, gold, score, wave counter ✏️
        └── tower_menu.hpp/.cpp         - Tower selection & placement UI ✏️
```

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.