#pragma once

#include <raylib.h>
#include <string>

// One installed content pack: metadata parsed from its pack.toml plus the
// directory layout the game loads its data and assets from. A pack bundles its
// own gameplay configs (under data/) and assets (under resources/); the game
// activates exactly one at a time.
struct Datapack {
    std::string m_name;
    std::string m_author;
    std::string m_version;
    std::string m_description;
    std::string m_iconPath; // relative to the pack root (from pack.toml)
    std::string m_rootDir;  // relative to the project root, e.g. "datapacks/default"

    // Selection-screen thumbnail; owned and freed by DatapackRegistry.
    Texture2D m_icon = {};

    // Relative-to-project-root subdirectories the loaders resolve against.
    std::string DataDir() const { return m_rootDir + "/data"; }
    std::string ResourcesDir() const { return m_rootDir + "/resources"; }
};
