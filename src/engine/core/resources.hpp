#pragma once

#include <raylib.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <expected>

class Resources {
public:
    Resources()  = default;
    ~Resources() { Shutdown(); }

    // Non-copyable
    Resources(const Resources&) = delete;
    Resources& operator=(const Resources&) = delete;

    // Search paths
    // The loader resolves relative paths against an ordered list of search roots,
    // highest priority first. SetAssetPath resets the list to a single base root
    // (the lowest-priority fallback). PushSearchPath adds a higher-priority root
    // on top of it (its assets shadow same-key assets in lower roots); PopSearchPath
    // removes the most recently pushed root, never the base.
    void SetAssetPath(const std::string& root);
    void PushSearchPath(const std::string& root);
    void PopSearchPath();

    // Load. Returns an error message describing the failure (file missing under every
    // search root, or a decode error) instead of crashing — caller decides whether a
    // missing asset is fatal. A key already loaded is a no-op success.
    std::expected<void, std::string> LoadTexture(const std::string& key, const std::string& relativePath);
    std::expected<void, std::string> LoadSound(const std::string& key, const std::string& relativePath);
    std::expected<void, std::string> LoadMusic(const std::string& key, const std::string& relativePath);

    // Bulk load from a subdirectory across every search root (key = filename stem).
    // Higher-priority roots win on key collisions. Returns the keys newly added by
    // this call so the caller can scope-unload exactly what it loaded.
    std::vector<std::string> LoadTexturesFromDir(const std::string& relativeDir);
    std::vector<std::string> LoadMusicFromDir(const std::string& relativeDir);
    std::vector<std::string> LoadSoundsFromDir(const std::string& relativeDir);

    // Retrieve
    Texture2D& GetTexture(const std::string& key);
    Sound&     GetSound(const std::string& key);
    Music&     GetMusic(const std::string& key);

    // Query
    bool HasTexture(const std::string& key) const;
    bool HasSound(const std::string& key)   const;
    bool HasMusic(const std::string& key)   const;

    // Per-key unload — frees a single asset so a caller can release the subset it
    // loaded without tearing down everything else. No-op for an unknown key.
    void UnloadTextureKey(const std::string& key);
    void UnloadSoundKey(const std::string& key);
    void UnloadMusicKey(const std::string& key);

    // Lifecycle
    void Shutdown();

private:
    // Resolves a relative path against the search roots, returning the first that
    // exists. Falls back to the base root so error messages stay meaningful.
    std::string ResolvePath(const std::string& relativePath) const;

    // Ordered highest priority first; the last entry is the base root.
    std::vector<std::string> m_searchPaths;

    std::unordered_map<std::string, Texture2D> m_textures;
    std::unordered_map<std::string, Sound>     m_sounds;
    std::unordered_map<std::string, Music>     m_music;
};
