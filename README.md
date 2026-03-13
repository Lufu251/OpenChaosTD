# OpenChaosTD
This is an open-map 2D Tower Defense game made in c++ with raylib that is still a work in progress. Towers can be placed anywhere on a grid, and enemies will search for the best path.

## Media
* (Add a screenshot or GIF of your project here)

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
    Now there should be a HTTP Server running under port 8000 to test your project. The URL should be visible in the active terminal.

## Project Structure
```
tower-defense/
└── src/                                Root
    ├── main.cpp                        - Run game
    ├── game.hpp / game.cpp             - Window creation, FPS, resizing
    │
    ├── states/                         Divide screens into individual states
    │   ├── IGameState.hpp              - Base class (input, logic, drawing)
    │   ├── MenuState.hpp/.cpp          - Pre playing state
    │   ├── PlayingState.hpp/.cpp       - Playing state
    │   ├── GameOverState.hpp/.cpp      - Gameover state display score and start new ✏️
    │   └── VictoryState.hpp/.cpp       - Victory state display score ✏️
    │
    ├── core/                           Core functionality that is needed for the hole game
    │   └── asset_manager.hpp/.cpp      - Load/cache textures, sounds, fonts
    │   └── renderer.hpp/.cpp           - Wrap all the rendering and letterbox scaling
    │   └── input_manager.hpp/.cpp      - Keybinding, virtual mouse and mouse consumption
    │
    ├── world/                          Grid, wave and paths
    │   ├── map.hpp/.cpp                - Tile grid + path definition ✏️
    │   ├── tile.hpp/.cpp               - Tile type (grass, path, buildable) ✏️
    │   └── wave_manager.hpp/.cpp       - Spawn timing, wave definitions ✏️
    │
    ├── entities/                       Entities only hold data
    │   ├── entity.hpp                  - Base class ✏️
    │   ├── enemy.hpp/.cpp              - Health, speed ✏️
    │   ├── tower.hpp/.cpp              - Range, damage, fire rate ✏️
    │   └── projectile.hpp/.cpp         - Damage, status ✏️
    │
    ├── systems/                        Implement functionality to the entities
    │   ├── collision_system.hpp/.cpp   - Detecting collisions ✏️
    │   └── render_system.hpp/.cpp      - Drawing entities ✏️
    │
    └── ui/                             User interface
        ├── hud.hpp/.cpp                - Lives, gold, score, wave counter ✏️
        └── tower_menu.hpp/.cpp         - Tower selection & placement UI ✏️
```

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.