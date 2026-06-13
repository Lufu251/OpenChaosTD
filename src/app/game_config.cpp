#include <app/game_config.hpp>
#include <engine/util/file_store.hpp>
#include <raylib.h>
#include <toml++/toml.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

// Baked into the binary at build time from resources/textures/openchaostd.png
// (cmake/embed_resource.cmake), mirroring how the fonts are embedded.
extern const unsigned char gOpenChaosTdIcon[];
extern const std::size_t gOpenChaosTdIconSize;

void GameConfig::Load(FileStore& fileStore) {
    if (!fileStore.Exists("config/settings.toml"))
        return;

    const toml::table table = fileStore.LoadToml("config/settings.toml");

    // Window bootstrap group (not editable from the settings menu)
    if (const toml::table* w = table["window"].as_table()) {
        if (auto v = (*w)["width"].value<int>())          gameWidth   = *v;
        if (auto v = (*w)["height"].value<int>())         gameHeight  = *v;
        if (auto v = (*w)["title"].value<std::string>())  title       = *v;
        if (auto v = (*w)["clampMargin"].value<float>())  clampMargin = *v;
    }
    // Display group
    if (const toml::table* d = table["display"].as_table()) {
        if (auto v = (*d)["fps"].value<int>())        fps      = *v;
        if (auto v = (*d)["hudScale"].value<float>()) hudScale = *v;
    }
    // Audio group
    if (const toml::table* a = table["audio"].as_table()) {
        if (auto v = (*a)["musicVolume"].value<float>()) musicVolume = *v;
        if (auto v = (*a)["sfxVolume"].value<float>())   sfxVolume   = *v;
    }

    // Clamp externally-supplied values to sane ranges so a hand-edited or corrupt settings.toml
    // can't put the app into a degenerate state (zero-size window, silent muting, inverted margin).
    gameWidth   = std::max(320, gameWidth);
    gameHeight  = std::max(240, gameHeight);
    fps         = std::max(1, fps);
    hudScale    = std::clamp(hudScale, 0.5f, 4.0f);
    musicVolume = std::clamp(musicVolume, 0.0f, 1.0f);
    sfxVolume   = std::clamp(sfxVolume, 0.0f, 1.0f);
    clampMargin = std::clamp(clampMargin, 0.1f, 1.0f);
}

void GameConfig::Save(FileStore& fileStore) {
    // Mirror the grouped on-disk shape. The window group is written straight from
    // the live struct so it round-trips untouched even though the menu never edits it.
    // Round slider-driven floats to 2 decimals so the snapped increments serialize
    // cleanly (e.g. 0.85, 1.25) instead of float-promotion noise.
    const toml::table data{
        {"window", toml::table{{"width", gameWidth}, {"height", gameHeight}, {"title", title}, {"clampMargin", clampMargin}}},
        {"display", toml::table{{"fps", fps},
                                {"hudScale", std::round(hudScale * 100.0) / 100.0}}},
        {"audio", toml::table{{"musicVolume", std::round(musicVolume * 100.0) / 100.0},
                              {"sfxVolume", std::round(sfxVolume * 100.0) / 100.0}}},
    };
    fileStore.SaveToml("config/settings.toml", data);
}

void GameConfig::ApplyIcon() {
    // Decode the PNG embedded in the binary (resources/textures/openchaostd.png) and
    // hand it to the window manager. No filesystem access, so it works on every platform.
    Image icon = LoadImageFromMemory(".png", gOpenChaosTdIcon, static_cast<int>(gOpenChaosTdIconSize));
    SetWindowIcon(icon);
    UnloadImage(icon);
}
