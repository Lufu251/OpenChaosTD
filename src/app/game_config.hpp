#pragma once

#include <string>

class FileStore;

// NOTE: members intentionally keep bare names (no m_ prefix) — the engine (sound_system) reads
// musicVolume/sfxVolume directly, and engine code is out of scope for the game-side naming sweep.
struct GameConfig {
    int gameWidth = 1200;
    int gameHeight = 1200;
    int fps = 120;
    float hudScale = 1.0f;
    std::string title = "OpenChaos TD";
    float musicVolume = 1.0f;
    float sfxVolume = 1.0f;

    void Load(FileStore& fileStore);
    void Save(FileStore& fileStore);
    void ApplyIcon();
};
