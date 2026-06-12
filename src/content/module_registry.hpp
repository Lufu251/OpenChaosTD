#pragma once

#include <world/module_def.hpp>
#include <cstdio>

// Look up a module builder by its type string and invoke it, logging and returning a
// null module pointer when the type is unknown. Shared by TowerFactory and EnemyFactory,
// whose builder registries differ only in the module pointer type they produce — the
// pointer type is supplied as the explicit first template argument.
template <typename ModulePtr, typename Builders>
ModulePtr BuildFromRegistry(const Builders& builders, const ModuleDef& mod, const char* factoryName) {
    auto it = builders.find(mod.m_type);
    if (it != builders.end())
        return it->second(mod);
    std::fprintf(stderr, "%s: unknown module type '%s'\n", factoryName, mod.m_type.c_str());
    return nullptr;
}
