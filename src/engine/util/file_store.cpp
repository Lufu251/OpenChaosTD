#include <engine/util/file_store.hpp>
#include <algorithm>
#include <cassert>
#include <iostream>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
    #include <fstream>
    #include <sstream>
#else
    #include <fstream>
    #include <sstream>
    #include <filesystem>
#endif

// RootPath
void FileStore::SetRootPath(const std::string& rootPath) {
#if defined(PLATFORM_WEB)
    m_rootPath = rootPath;
    std::cout << "filestore: root set to '" << m_rootPath << "' (web)\n";
#else
    std::filesystem::path root = std::filesystem::path(rootPath);
    m_rootPath = root.string();
    std::cout << "filestore: root set to '" << m_rootPath << "'\n";
#endif
}

// Private: path resolution
std::string FileStore::ResolvePath(const std::string& path) const {
    // Internal invariant: SetRootPath() runs during startup before any file op.
    assert(!m_rootPath.empty() && "filestore: SetRootPath() must be called before any file operations");

#if defined(PLATFORM_WEB)
    return path;
#else
    return (std::filesystem::path(m_rootPath) / path).string();
#endif
}

// SaveJson
void FileStore::SaveJson(const std::string& path, const nlohmann::json& data) {
#if defined(PLATFORM_WEB)

    std::string jsonStr = data.dump();
    EM_ASM({
        localStorage.setItem(UTF8ToString($0), UTF8ToString($1));
    }, path.c_str(), jsonStr.c_str());
    std::cout << "filestore: saved '" << path << "' to localStorage\n";

#else

    std::string fullPath = ResolvePath(path);
    std::filesystem::create_directories(
        std::filesystem::path(fullPath).parent_path()
    );

    std::ofstream file(fullPath);
    if (!file.is_open()) {
        std::cerr << "filestore: could not write '" << fullPath << "'\n";
        return;
    }
    file << data.dump(4) << "\n";
    std::cout << "filestore: saved '" << fullPath << "'\n";

#endif
}

// LoadJson
nlohmann::json FileStore::LoadJson(const std::string& path) {
#if defined(PLATFORM_WEB)

    // Try VFS first (preloaded read-only game data)
    {
        std::ifstream vfsFile(path);
        if (vfsFile.is_open()) {
            try {
                nlohmann::json data;
                vfsFile >> data;
                return data;
            } catch (const nlohmann::json::parse_error& e) {
                std::cerr << "filestore: parse error loading '" << path << "': " << e.what() << "\n";
                return {};
            }
        }
    }

    // Fall back to localStorage (runtime save data)
    char* raw = (char*)EM_ASM_PTR({
        var value = localStorage.getItem(UTF8ToString($0));
        if (!value) return 0;
        var len = lengthBytesUTF8(value) + 1;
        var buf = _malloc(len);
        stringToUTF8(value, buf, len);
        return buf;
    }, path.c_str());

    if (!raw) {
        std::cout << "filestore: no data found for '" << path << "'\n";
        return {};
    }

    std::string jsonStr(raw);
    free(raw);

    try {
        return nlohmann::json::parse(jsonStr);
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "filestore: parse error loading '" << path << "': " << e.what() << "\n";
        return {};
    }

#else

    std::string fullPath = ResolvePath(path);
    std::ifstream file(fullPath);
    if (!file.is_open()) {
        std::cout << "filestore: no file found at '" << fullPath << "'\n";
        return {};
    }

    try {
        nlohmann::json data;
        file >> data;
        return data;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "filestore: parse error loading '" << fullPath << "': " << e.what() << "\n";
        return {};
    }

#endif
}

// SaveToml
void FileStore::SaveToml(const std::string& path, const toml::table& data) {
    // Relaxed float precision: prints shortest 6-significant-digit form (0.28
    // instead of 0.28000000000000003 — GCC builds lack float charconv in toml++).
    toml::toml_formatter formatter{data,
        toml::toml_formatter::default_flags | toml::format_flags::relaxed_float_precision};

#if defined(PLATFORM_WEB)

    std::stringstream ss;
    ss << formatter;
    std::string tomlStr = ss.str();
    EM_ASM({
        localStorage.setItem(UTF8ToString($0), UTF8ToString($1));
    }, path.c_str(), tomlStr.c_str());
    std::cout << "filestore: saved '" << path << "' to localStorage\n";

#else

    std::string fullPath = ResolvePath(path);
    std::filesystem::create_directories(
        std::filesystem::path(fullPath).parent_path()
    );

    std::ofstream file(fullPath);
    if (!file.is_open()) {
        std::cerr << "filestore: could not write '" << fullPath << "'\n";
        return;
    }
    file << formatter << "\n";
    std::cout << "filestore: saved '" << fullPath << "'\n";

#endif
}

// LoadToml
toml::table FileStore::LoadToml(const std::string& path) {
#if defined(PLATFORM_WEB)

    // Try VFS first (preloaded read-only game data)
    {
        std::ifstream vfsFile(path);
        if (vfsFile.is_open()) {
            try {
                return toml::parse(vfsFile);
            } catch (const toml::parse_error& e) {
                std::cerr << "filestore: parse error loading '" << path << "': " << e.description() << "\n";
                return {};
            }
        }
    }

    // Fall back to localStorage (runtime save data)
    char* raw = (char*)EM_ASM_PTR({
        var value = localStorage.getItem(UTF8ToString($0));
        if (!value) return 0;
        var len = lengthBytesUTF8(value) + 1;
        var buf = _malloc(len);
        stringToUTF8(value, buf, len);
        return buf;
    }, path.c_str());

    if (!raw) {
        std::cout << "filestore: no data found for '" << path << "'\n";
        return {};
    }

    std::string tomlStr(raw);
    free(raw);

    try {
        return toml::parse(tomlStr);
    } catch (const toml::parse_error& e) {
        std::cerr << "filestore: parse error loading '" << path << "': " << e.description() << "\n";
        return {};
    }

#else

    std::string fullPath = ResolvePath(path);
    if (!std::filesystem::exists(fullPath)) {
        std::cout << "filestore: no file found at '" << fullPath << "'\n";
        return {};
    }

    try {
        return toml::parse_file(fullPath);
    } catch (const toml::parse_error& e) {
        std::cerr << "filestore: parse error loading '" << fullPath << "': " << e.description() << "\n";
        return {};
    }

#endif
}

// LoadBytes — raw binary read
std::vector<unsigned char> FileStore::LoadBytes(const std::string& path) {
    auto readStream = [](std::ifstream& file) -> std::vector<unsigned char> {
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        if (size <= 0) return {};
        file.seekg(0, std::ios::beg);
        std::vector<unsigned char> bytes(static_cast<size_t>(size));
        file.read(reinterpret_cast<char*>(bytes.data()), size);
        // A short/failed read would leave garbage in the tail; report it as unreadable (empty) per
        // the contract instead of handing back a partially-filled buffer with no error signal.
        if (file.gcount() != size) {
            std::cerr << "filestore: short read (" << file.gcount() << " of " << size << " bytes)\n";
            return {};
        }
        return bytes;
    };

#if defined(PLATFORM_WEB)

    // Preloaded VFS file first (shipped datapack assets)…
    {
        std::ifstream vfsFile(path, std::ios::binary);
        if (vfsFile.is_open())
            return readStream(vfsFile);
    }

    // …then localStorage, where runtime-authored binaries are stored base64-encoded.
    int len = 0;
    unsigned char* buf = (unsigned char*)EM_ASM_PTR({
        var b64 = localStorage.getItem(UTF8ToString($0));
        if (b64 === null) return 0;
        var bin = atob(b64);
        var n = bin.length;
        var p = _malloc(n);
        for (var i = 0; i < n; i++) HEAPU8[p + i] = bin.charCodeAt(i);
        setValue($1, n, 'i32');
        return p;
    }, path.c_str(), &len);

    if (!buf || len <= 0) {
        std::cout << "filestore: no bytes found for '" << path << "'\n";
        return {};
    }
    std::vector<unsigned char> bytes(buf, buf + len);
    free(buf);
    return bytes;

#else

    std::ifstream file(ResolvePath(path), std::ios::binary);
    if (!file.is_open()) {
        std::cout << "filestore: no file found at '" << ResolvePath(path) << "'\n";
        return {};
    }
    return readStream(file);

#endif
}

// Exists
bool FileStore::Exists(const std::string& path) {
#if defined(PLATFORM_WEB)

    // Check VFS first
    if (FILE* f = fopen(path.c_str(), "r")) {
        fclose(f);
        return true;
    }

    // Check localStorage
    int exists = EM_ASM_INT({
        return localStorage.getItem(UTF8ToString($0)) !== null ? 1 : 0;
    }, path.c_str());
    return exists == 1;

#else

    return std::filesystem::exists(ResolvePath(path));

#endif
}

// Delete
void FileStore::Delete(const std::string& path) {
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

// ListSubfolders — immediate child directory names, sorted
std::vector<std::string> FileStore::ListSubfolders(const std::string& path) {
#if defined(PLATFORM_WEB)

    // Folders are implicit in localStorage key names. Collect the first path
    // segment that follows "path/" across every stored key, de-duplicated.
    std::string prefix = path + "/";
    char* raw = (char*)EM_ASM_PTR({
        var value = Object.keys(localStorage).join("\n");
        var len = lengthBytesUTF8(value) + 1;
        var buf = _malloc(len);
        stringToUTF8(value, buf, len);
        return buf;
    });
    std::vector<std::string> folders;
    if (raw) {
        std::stringstream keys(raw);
        free(raw);
        std::string key;
        while (std::getline(keys, key, '\n')) {
            if (key.rfind(prefix, 0) != 0) continue;
            std::string rest = key.substr(prefix.size());
            size_t slash = rest.find('/');
            if (slash == std::string::npos) continue; // a file directly in path, not a subfolder
            std::string name = rest.substr(0, slash);
            if (std::find(folders.begin(), folders.end(), name) == folders.end())
                folders.push_back(name);
        }
    }
    std::sort(folders.begin(), folders.end());
    return folders;

#else

    std::string fullPath = ResolvePath(path);
    std::vector<std::string> folders;
    if (!std::filesystem::is_directory(fullPath))
        return folders;

    for (const auto& entry : std::filesystem::directory_iterator(fullPath)) {
        if (entry.is_directory())
            folders.push_back(entry.path().filename().string());
    }
    std::sort(folders.begin(), folders.end());
    return folders;

#endif
}

// CreateFolder
void FileStore::CreateFolder(const std::string& path) {
#if defined(PLATFORM_WEB)
    // No-op: localStorage has no directories; SaveJson/SaveToml materialize the
    // folder implicitly the moment a file is written under it.
    (void)path;
#else
    std::filesystem::create_directories(ResolvePath(path));
#endif
}

// DeleteFolder — recursive
void FileStore::DeleteFolder(const std::string& path) {
#if defined(PLATFORM_WEB)

    // Remove every key under "path" (the folder itself and anything beneath it).
    EM_ASM({
        var prefix = UTF8ToString($0);
        var keys = Object.keys(localStorage);
        for (var i = 0; i < keys.length; i++) {
            if (keys[i] === prefix || keys[i].indexOf(prefix + "/") === 0)
                localStorage.removeItem(keys[i]);
        }
    }, path.c_str());

#else

    std::string fullPath = ResolvePath(path);
    if (std::filesystem::exists(fullPath))
        std::filesystem::remove_all(fullPath);

#endif
}

// FullPath
std::string FileStore::FullPath(const std::string& path) {
#if defined(PLATFORM_WEB)
    return path;
#else
    return ResolvePath(path);
#endif
}