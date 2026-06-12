#pragma once

#include <content/datapack.hpp>
#include <engine/util/file_store.hpp>
#include <string>
#include <vector>

// Discovers installed datapacks by scanning the project's "datapacks" directory
// for subfolders holding a valid pack.toml, and owns the icon textures shown on
// the selection screen. Malformed packs are skipped with a warning so a broken
// mod never crashes the game.
class DatapackRegistry {
public:
    // Rescan the datapacks directory and rebuild the pack list. Does not load icons.
    void Scan(FileStore& fileStore);

    // Load / free the selection-screen icon textures. Call LoadIcons when the
    // selection screen opens and UnloadIcons when it closes.
    void LoadIcons();
    void UnloadIcons();

    const std::vector<Datapack>& Packs() const { return m_packs; }
    bool Empty() const { return m_packs.empty(); }

private:
    std::string m_rootPath; // absolute project root, captured during Scan (for icon paths)
    std::vector<Datapack> m_packs;
};
