#include <factory/emitter_presets.hpp>
#include <engine/core/text_renderer.hpp>
#include <engine/util/file_store.hpp>
#include <toml++/toml.hpp>
#include <algorithm>
#include <cstdint>
#include <iostream>

Color ParseTomlColor(const toml::array& a) {
    return {
        (unsigned char)a[0].value_or(0), (unsigned char)a[1].value_or(0),
        (unsigned char)a[2].value_or(0), (unsigned char)a[3].value_or(0)
    };
}

static SpawnShape ParseShape(const std::string& s) {
    if (s == "Line")   return SpawnShape::Line;
    if (s == "Box")    return SpawnShape::Box;
    if (s == "Circle") return SpawnShape::Circle;
    if (s == "Ring")   return SpawnShape::Ring;
    return SpawnShape::Point;
}

static EmitterDesc ParseEmitterDesc(const toml::table& j) {
    EmitterDesc d;
    if (auto c = j["color"].as_array())    d.m_color    = ParseTomlColor(*c);
    if (auto c = j["endColor"].as_array()) d.m_endColor = ParseTomlColor(*c);
    d.m_count         = j["count"].value_or(0);
    d.m_speed         = j["speed"].value_or(50.0f);
    d.m_speedVariance = j["speedVariance"].value_or(20.0f);
    d.m_spread        = j["spread"].value_or(360.0f);
    d.m_angle         = j["angle"].value_or(0.0f);
    d.m_lifetime      = j["lifetime"].value_or(0.2f);
    d.m_size          = j["size"].value_or(3.0f);
    d.m_endSize       = j["endSize"].value_or(0.0f);

    // Spawn shape, unified dynamics, and continuous emission rate.
    d.m_shape           = ParseShape(j["shape"].value_or(std::string("Point")));
    d.m_shapeSize       = {j["shapeWidth"].value_or(0.0f), j["shapeHeight"].value_or(0.0f)};
    d.m_shapeRadius     = j["shapeRadius"].value_or(0.0f);
    d.m_radialSpeed     = j["radialSpeed"].value_or(0.0f);
    d.m_tangentialSpeed = j["tangentialSpeed"].value_or(0.0f);
    d.m_emitRate        = j["emitRate"].value_or(0.0f);

    return d;
}

// --- Serialization (inverse of ParseEmitterDesc) ----------------------------

static const char* ShapeName(SpawnShape s) {
    switch (s) {
        case SpawnShape::Line:   return "Line";
        case SpawnShape::Box:    return "Box";
        case SpawnShape::Circle: return "Circle";
        case SpawnShape::Ring:   return "Ring";
        default:                 return "Point";
    }
}

static toml::array ColorArray(Color c) {
    return toml::array{
        static_cast<int64_t>(c.r), static_cast<int64_t>(c.g),
        static_cast<int64_t>(c.b), static_cast<int64_t>(c.a)
    };
}

// Builds a preset table holding only the fields that differ from a default
// EmitterDesc — matching the hand-authored style of the data file.
// ParseEmitterDesc fills the omitted fields back in on load.
static toml::table BuildPresetTable(const EmitterDesc& d) {
    const EmitterDesc def;
    toml::table t;

    // Floats round-trip through the compact decimal string: inserting the raw
    // float widened to double would serialize as 0.3799999952316284-style noise.
    auto writeFloat = [&](const char* key, float value, float defValue) {
        if (value != defValue)
            t.insert(key, std::stod(FormatFloat(value)));
    };

    if (d.m_count != def.m_count)
        t.insert("count", static_cast<int64_t>(d.m_count));
    writeFloat("emitRate", d.m_emitRate, def.m_emitRate);
    if (!ColorEquals(d.m_color, def.m_color))
        t.insert("color", ColorArray(d.m_color));
    if (!ColorEquals(d.m_endColor, def.m_endColor))
        t.insert("endColor", ColorArray(d.m_endColor));
    writeFloat("speed", d.m_speed, def.m_speed);
    writeFloat("speedVariance", d.m_speedVariance, def.m_speedVariance);
    writeFloat("spread", d.m_spread, def.m_spread);
    writeFloat("angle", d.m_angle, def.m_angle);
    writeFloat("lifetime", d.m_lifetime, def.m_lifetime);
    writeFloat("size", d.m_size, def.m_size);
    writeFloat("endSize", d.m_endSize, def.m_endSize);
    if (d.m_shape != def.m_shape)
        t.insert("shape", ShapeName(d.m_shape));
    writeFloat("shapeWidth", d.m_shapeSize.x, def.m_shapeSize.x);
    writeFloat("shapeHeight", d.m_shapeSize.y, def.m_shapeSize.y);
    writeFloat("shapeRadius", d.m_shapeRadius, def.m_shapeRadius);
    writeFloat("radialSpeed", d.m_radialSpeed, def.m_radialSpeed);
    writeFloat("tangentialSpeed", d.m_tangentialSpeed, def.m_tangentialSpeed);

    return t;
}

void EmitterPresets::Clear() {
    m_presets.clear();
}

void EmitterPresets::Load(FileStore& fileStore, const std::string& dataDir) {
    Clear(); // replace any previously loaded pack's presets
    m_presetPath = dataDir + "/particle_effects.toml";

    auto data = fileStore.LoadToml(m_presetPath);
    auto presets = data["presets"].as_table();
    if (!presets) {
        std::cerr << "EmitterPresets: failed to load " << m_presetPath << "\n";
        return;
    }
    for (auto&& [name, descNode] : *presets) {
        if (auto descTbl = descNode.as_table()) {
            std::string nm(name.str());
            m_presets[nm] = ParseEmitterDesc(*descTbl);
            std::cout << "EmitterPresets: loaded '" << nm << "'\n";
        }
    }
}

EmitterDesc EmitterPresets::Get(const std::string& name) const {
    auto it = m_presets.find(name);
    if (it != m_presets.end()) return it->second;
    std::cerr << "EmitterPresets: unknown preset '" << name << "'\n";
    return {};
}

const EmitterDesc* EmitterPresets::GetPtr(const std::string& name) const {
    auto it = m_presets.find(name);
    if (it != m_presets.end()) return &it->second;
    std::cerr << "EmitterPresets: unknown preset '" << name << "'\n";
    return nullptr;
}

bool EmitterPresets::Has(const std::string& name) const {
    return m_presets.count(name) > 0;
}

void EmitterPresets::Set(const std::string& name, const EmitterDesc& desc) {
    m_presets[name] = desc;
}

std::vector<std::string> EmitterPresets::Names() const {
    std::vector<std::string> names;
    names.reserve(m_presets.size());
    for (auto& [name, desc] : m_presets)
        names.push_back(name);
    std::sort(names.begin(), names.end());
    return names;
}

bool EmitterPresets::SavePreset(FileStore& fileStore, const std::string& name,
                                const EmitterDesc& desc) {
    toml::table data = fileStore.LoadToml(m_presetPath);

    // An existing file that parses to an empty table means the read failed —
    // abort rather than clobbering every other preset. Empty + missing = new file.
    if (data.empty() && fileStore.Exists(m_presetPath)) {
        std::cerr << "EmitterPresets: could not read '" << m_presetPath << "', save aborted\n";
        return false;
    }

    auto presets = data["presets"].as_table();
    if (!presets) {
        data.insert("presets", toml::table{});
        presets = data["presets"].as_table();
    }
    presets->insert_or_assign(name, BuildPresetTable(desc));

    fileStore.SaveToml(m_presetPath, data);
    Set(name, desc);
    std::cout << "EmitterPresets: saved '" << name << "'\n";
    return true;
}

bool EmitterPresets::DeletePreset(FileStore& fileStore, const std::string& name) {
    toml::table data = fileStore.LoadToml(m_presetPath);

    // Same don't-clobber guard as SavePreset.
    if (data.empty() && fileStore.Exists(m_presetPath)) {
        std::cerr << "EmitterPresets: could not read '" << m_presetPath << "', delete aborted\n";
        return false;
    }

    if (auto presets = data["presets"].as_table())
        presets->erase(name);

    fileStore.SaveToml(m_presetPath, data);
    m_presets.erase(name);
    std::cout << "EmitterPresets: deleted '" << name << "'\n";
    return true;
}
