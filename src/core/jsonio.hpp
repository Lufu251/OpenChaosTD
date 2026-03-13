#pragma once

#include <nlohmann/json.hpp>
#include <string>

class JsonIO {
public:
    JsonIO()  = default;
    ~JsonIO() = default;

    // Non-copyable — owns internal root path state
    JsonIO(const JsonIO&)            = delete;
    JsonIO& operator=(const JsonIO&) = delete;

    // Call once, project root as its parent for files stored.
    void SetRootPath(const std::string& assetPath);
    
    const std::string& GetRootPath() const { return m_rootPath; }

    void           Save(const std::string& path, const nlohmann::json& data);
    nlohmann::json Load(const std::string& path);
    bool           Exists(const std::string& path);
    void           Delete(const std::string& path);

private:
    // Resolves a relative path to a full absolute path (native only)
    std::string ResolvePath(const std::string& path) const;

    std::string m_rootPath; // Set by Init() — absolute path to project root
};