#pragma once

// Shared filesystem paths for persistent game data. Defined once so the writer
// (PlayingState) and reader (MenuState's "continue") never drift apart.
inline constexpr const char* kSaveGamePath = "saves/savegame.json";
