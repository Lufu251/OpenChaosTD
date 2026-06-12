#include <app/game_config.hpp>
#include <engine/util/file_store.hpp>
#include <raylib.h>
#include <toml++/toml.hpp>
#include <cmath>
#include <string>

void GameConfig::Load(FileStore& fileStore) {
    if (!fileStore.Exists("config/settings.toml"))
        return;

    const toml::table table = fileStore.LoadToml("config/settings.toml");

    // Window bootstrap group (not editable from the settings menu)
    if (const toml::table* w = table["window"].as_table()) {
        if (auto v = (*w)["width"].value<int>())          gameWidth  = *v;
        if (auto v = (*w)["height"].value<int>())         gameHeight = *v;
        if (auto v = (*w)["title"].value<std::string>())  title      = *v;
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
}

void GameConfig::Save(FileStore& fileStore) {
    // Mirror the grouped on-disk shape. The window group is written straight from
    // the live struct so it round-trips untouched even though the menu never edits it.
    // Round slider-driven floats to 2 decimals so the snapped increments serialize
    // cleanly (e.g. 0.85, 1.25) instead of float-promotion noise.
    const toml::table data{
        {"window", toml::table{{"width", gameWidth}, {"height", gameHeight}, {"title", title}}},
        {"display", toml::table{{"fps", fps},
                                {"hudScale", std::round(hudScale * 100.0) / 100.0}}},
        {"audio", toml::table{{"musicVolume", std::round(musicVolume * 100.0) / 100.0},
                              {"sfxVolume", std::round(sfxVolume * 100.0) / 100.0}}},
    };
    fileStore.SaveToml("config/settings.toml", data);
}

void GameConfig::ApplyIcon() {
    // 64x64 icon: dark background, stone tower, gold orb
    Image icon = GenImageColor(64, 64, {15, 15, 25, 255});

    // Tower base (wider footing)
    ImageDrawRectangle(&icon, 17, 50, 30, 8, {80, 80, 100, 255});
    // Tower body
    ImageDrawRectangle(&icon, 22, 18, 20, 34, {80, 80, 100, 255});
    // Battlements (3 merlons)
    ImageDrawRectangle(&icon, 22, 10, 5, 10, {80, 80, 100, 255});
    ImageDrawRectangle(&icon, 30, 10, 4, 10, {80, 80, 100, 255});
    ImageDrawRectangle(&icon, 37, 10, 5, 10, {80, 80, 100, 255});
    // Arrow slit window
    ImageDrawRectangle(&icon, 29, 28, 6, 12, {25, 25, 40, 255});
    // Gold orb behind the slit
    ImageDrawCircle(&icon, 32, 34, 4, {255, 195, 50, 255});
    // Re-draw slit over orb so it reads as a window glow
    ImageDrawRectangle(&icon, 29, 28, 6, 12, {255, 220, 100, 180});

    SetWindowIcon(icon);
    UnloadImage(icon);
}
