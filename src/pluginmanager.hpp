#pragma once
#include <vector>
#include <string>
#include "configmanager.hpp"
#include "scenes/editor/editor.hpp"

extern "C"
{
    #include <lib/lua/lua.h>
    #include <lib/lua/lualib.h>
    #include <lib/lua/lauxlib.h>
}

class Plugin {
private:
    std::string name;
    std::string version;
    std::string author;
    std::string mainFilePath;
public:
    void setName(std::string name);
    void setVersion(std::string version);
    void setAutor(std::string author);
    void setMainFile(std::string path);

    std::string getName();
    std::string getVersion();
    std::string getAutor();
    std::string getMainFile();
};

class PluginManager {
private:
    std::vector <Plugin> plugins;
    lua_State* Lua = luaL_newstate();
    ConfigManager pluginConfig;
    sceneEditor* scene;

    lua_CFunction addTool(lua_State* Lua);
    void loadPlugin(size_t index);
public:
    PluginManager();

    void init(sceneEditor& scene);
    void searchPlugins();
    void loadPlugins();
    size_t numberOfPlugins();
    Plugin& getPlugin(size_t index);

    ~PluginManager();
};