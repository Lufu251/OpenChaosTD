#pragma once

#include <nlohmann/json.hpp>
#include <toml++/toml.hpp>
#include <cstdio>
#include <string>
#include <vector>

// Compact float formatting for serialization: trailing zeros stripped, at least one
// decimal digit kept (2.0, 0.38). Serializing the raw float widened to double would
// otherwise emit 0.3799999952316284-style noise; formatting to a fixed-precision
// decimal string first keeps saved values clean and round-trippable.
inline std::string FormatFloat(float v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.4f", v);
    std::string s(buf);
    size_t dot = s.find('.');
    size_t last = s.find_last_not_of('0');
    s.erase(last > dot ? last + 1 : dot + 2);
    return s;
}

class FileStore {
public:
    FileStore()  = default;
    ~FileStore() = default;

    // Non-copyable — owns internal root path state
    FileStore(const FileStore&)            = delete;
    FileStore& operator=(const FileStore&) = delete;

    // Call once, project root as its parent for files stored.
    void SetRootPath(const std::string& assetPath);
    
    const std::string& GetRootPath() const { return m_rootPath; }

    // JSON file I/O (nlohmann::json)
    void           SaveJson(const std::string& path, const nlohmann::json& data);
    nlohmann::json LoadJson(const std::string& path);

    // TOML file I/O (toml++)
    void           SaveToml(const std::string& path, const toml::table& data);
    toml::table    LoadToml(const std::string& path);

    // Raw binary read (e.g. images). Empty vector if missing/unreadable. On web,
    // reads the preloaded VFS file if present, else base64 data from localStorage.
    std::vector<unsigned char> LoadBytes(const std::string& path);

    // Format-agnostic (operate on the path, regardless of file contents)
    bool           Exists(const std::string& path);
    void           Delete(const std::string& path);

    // Directory operations. On web, folders are implicit in localStorage key names,
    // so these emulate a tree by treating the path as a key prefix.
    std::vector<std::string> ListSubfolders(const std::string& path); // immediate child names, sorted
    void                     CreateFolder(const std::string& path);
    void                     DeleteFolder(const std::string& path);   // recursive

    // Absolute on-disk path, for APIs that bypass FileStore (e.g. raylib ExportImage).
    // On web there is no real filesystem, so the path is returned unchanged.
    std::string              FullPath(const std::string& path);

private:
    // Resolves a relative path to a full absolute path (native only)
    std::string ResolvePath(const std::string& path) const;

    std::string m_rootPath; // Set by Init() — absolute path to project root
};