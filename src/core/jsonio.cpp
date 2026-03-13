#include <core/jsonio.hpp>
#include <iostream>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#else
    #include <fstream>
    #include <filesystem>
#endif

// RootPath
void JsonIO::SetRootPath(const std::string& rootPath) {
#if defined(PLATFORM_WEB)
    // No filesystem on web — root path is unused
    m_rootPath = "";
    std::cout << "Storage: initialized (web — using localStorage)\n";
#else
    // Derive project root from the parent of assets/
    std::filesystem::path root = std::filesystem::path(rootPath);
    m_rootPath = root.string();
    std::cout << "Storage: root set to '" << m_rootPath << "'\n";
#endif
}

// Private: path resolution (native only)
std::string JsonIO::ResolvePath(const std::string& path) const {
    if (m_rootPath.empty())
        throw std::runtime_error("Storage: Init() must be called before any file operations");

    return (std::filesystem::path(m_rootPath) / (path + ".json")).string();
}

// Save
void JsonIO::Save(const std::string& path, const nlohmann::json& data) {
#if defined(PLATFORM_WEB)

    std::string jsonStr = data.dump();
    EM_ASM({
        localStorage.setItem(UTF8ToString($0), UTF8ToString($1));
    }, path.c_str(), jsonStr.c_str());
    std::cout << "Storage: saved '" << path << "' to localStorage\n";

#else

    std::string fullPath = ResolvePath(path);
    std::filesystem::create_directories(
        std::filesystem::path(fullPath).parent_path()
    );

    std::ofstream file(fullPath);
    if (!file.is_open()) {
        std::cerr << "Storage: could not write '" << fullPath << "'\n";
        return;
    }
    file << data.dump(4) << "\n";
    std::cout << "Storage: saved '" << fullPath << "'\n";

#endif
}

// Load
nlohmann::json JsonIO::Load(const std::string& path) {
#if defined(PLATFORM_WEB)

    char* raw = (char*)EM_ASM_PTR({
        var value = localStorage.getItem(UTF8ToString($0));
        if (!value) return 0;
        var len = lengthBytesUTF8(value) + 1;
        var buf = _malloc(len);
        stringToUTF8(value, buf, len);
        return buf;
    }, path.c_str());

    if (!raw) {
        std::cout << "Storage: no data found for '" << path << "' in localStorage\n";
        return {};
    }

    std::string jsonStr(raw);
    free(raw);

    try {
        return nlohmann::json::parse(jsonStr);
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "Storage: parse error loading '" << path << "': " << e.what() << "\n";
        return {};
    }

#else

    std::string fullPath = ResolvePath(path);
    std::ifstream file(fullPath);
    if (!file.is_open()) {
        std::cout << "Storage: no file found at '" << fullPath << "'\n";
        return {};
    }

    try {
        nlohmann::json data;
        file >> data;
        return data;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "Storage: parse error loading '" << fullPath << "': " << e.what() << "\n";
        return {};
    }

#endif
}

// Exists
bool JsonIO::Exists(const std::string& path) {
#if defined(PLATFORM_WEB)

    int exists = EM_ASM_INT({
        return localStorage.getItem(UTF8ToString($0)) !== null ? 1 : 0;
    }, path.c_str());
    return exists == 1;

#else

    return std::filesystem::exists(ResolvePath(path));

#endif
}

// Delete
void JsonIO::Delete(const std::string& path) {
#if defined(PLATFORM_WEB)

    EM_ASM({
        localStorage.removeItem(UTF8ToString($0));
    }, path.c_str());

#else

    std::string fullPath = ResolvePath(path);
    if (std::filesystem::exists(fullPath))
        std::filesystem::remove(fullPath);

#endif
}