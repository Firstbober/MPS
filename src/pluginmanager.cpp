#include "pluginmanager.hpp"
#include <dirent.h>
#include <stdio.h>
#include "defines.hpp"
#include <string.h>
#include <fstream>
#include <iostream>
#include "scenes/editor/toolbox/tool/alltools.hpp"

void Plugin::setName(std::string name)       {this->name = name;}
void Plugin::setVersion(std::string version) {this->version = version;}
void Plugin::setAutor(std::string author)    {this->author = author;}
void Plugin::setMainFile(std::string path)   {this->mainFilePath = path;}

std::string Plugin::getName()     {return this->name;}
std::string Plugin::getVersion()  {return this->version;}
std::string Plugin::getAutor()    {return this->author;}
std::string Plugin::getMainFile() {return this->mainFilePath;}

void PluginManager::init(sceneEditor& scene) {
    this->scene = &scene;
}

void PluginManager::loadPlugin(size_t index) {
    if (luaL_dofile(this->Lua, plugins[index].getMainFile().c_str()))
    {
        std::cout << "Error while running " + plugins[index].getName() + " plugin script: " << lua_tostring(this->Lua, -1) << std::endl;
    } else {
        std::cout << "LOADED" << std::endl;
    }
}

void PluginManager::searchPlugins() {
    struct dirent * file;
    DIR * plugDir;
    std::vector <std::string> pluginsDir;

    if((plugDir = opendir(std::string(LOC_ROOT+std::string("plugins/")).c_str()))) {
        while((file = readdir(plugDir))) {
            std::string plugDirName = file->d_name;
            pluginsDir.push_back(plugDirName);
        }
        closedir(plugDir);
    }
    
    for(size_t i = 0; i < pluginsDir.size(); i++)
    {
        if(pluginsDir[i] + "/manifest.txt" == "./manifest.txt" || pluginsDir[i] + "/manifest.txt" == "../manifest.txt"); 
        else {
            std::cout << LOC_ROOT + std::string("plugins/") + pluginsDir[i] + "/manifest.txt" << std::endl;
            if(pluginConfig.open(std::string(LOC_ROOT + std::string("plugins/") + pluginsDir[i] + "/manifest.txt").c_str()) == true) {
                Plugin plugin;plugin.getName();

                plugin.setName(pluginConfig.getvarS("name"));
                plugin.setVersion(pluginConfig.getvarS("version"));
                plugin.setAutor(pluginConfig.getvarS("author"));
                plugin.setMainFile(LOC_ROOT + std::string("plugins/") + pluginsDir[i] + "/" + pluginConfig.getvarS("main"));
                
                std::cout << 
                "Plugin name: " + plugin.getName() << 
                "\nPlugin version: " + plugin.getVersion() << 
                "\nPlugin author: " + plugin.getAutor() << 
                "\nPlugin main file: " + plugin.getMainFile() << std::endl;
                
                plugins.push_back(plugin);
            }
        }
    }
}

void PluginManager::loadPlugins() { 
    if(plugins.size() != 0) {

    }
    for(size_t i = 0; i < plugins.size(); i++) {
        loadPlugin(i);
    }
}

PluginManager::~PluginManager() {
    lua_close(this->Lua);
}