/*  BOSS

    A plugin load order optimiser for games that use the esp/esm plugin system.

    Copyright (C) 2012    WrinklyNinja

    This file is part of BOSS.

    BOSS is free software: you can redistribute
    it and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, either version 3 of
    the License, or (at your option) any later version.

    BOSS is distributed in the hope that it will
    be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with BOSS.  If not, see
    <http://www.gnu.org/licenses/>.
*/

#include "game.h"
#include "globals.h"
#include "helpers.h"
#include "error.h"
#include "metadata.h"

#include <boost/algorithm/string.hpp>

using namespace std;

namespace fs = boost::filesystem;

namespace boss {

    std::vector<Game> GetGames(const YAML::Node& settings) {
        vector<Game> games;

        games.push_back(Game(GAME_TES4));
        games.push_back(Game(GAME_TES5));
        games.push_back(Game(GAME_FO3));
        games.push_back(Game(GAME_FONV));
        
        if (settings["Games"]) {
            for(YAML::const_iterator it=settings["Games"].begin(), endit=settings["Games"].end(); it != endit; ++it) {
                /*
                    Each game has the following variables: name, type, master, folder, url, path and registry.

                    The four archetypes have hardcoded values for each of these. Each game instance must have a unique name and a unique folder. Two or more games can be the same type, have the same master, use the same masterlist, be installed to the same path (eg. multiple install swapping) and have the same registry entry (again, swapping).

                    In the YAML file, at least a name must be supplied. If the name matches one of the four archetypes, that is all that is required, but if not, then a type and folder are also required.
                */

                Game game(it->first.as<string>());

                vector<Game>::iterator jt = find(games.begin(), games.end(), game);

                if (jt == games.end() && (!it->second["type"] || !it->second["name"]))
                    continue;

                string name, master, url, path, registry;
                if (it->second["name"])
                    name = it->second["name"].as<string>();
                if (it->second["master"])
                    master = it->second["master"].as<string>();
                if (it->second["url"])
                    url = it->second["url"].as<string>();
                if (it->second["path"])
                    path = it->second["path"].as<string>();
                if (it->second["registry"])
                    registry = it->second["registry"].as<string>();

                if (jt != games.end())
                    jt->SetDetails(name, master, url, path, registry);
                else {

                    if (boost::iequals(it->second["type"].as<string>(), Game(GAME_TES4).FolderName()))
                        game = Game(GAME_TES4, game.FolderName());
                    else if (boost::iequals(it->second["type"].as<string>(), Game(GAME_TES5).FolderName()))
                        game = Game(GAME_TES5, game.FolderName());
                    else if (boost::iequals(it->second["type"].as<string>(), Game(GAME_FO3).FolderName()))
                        game = Game(GAME_FO3, game.FolderName());
                    else if (boost::iequals(it->second["type"].as<string>(), Game(GAME_FONV).FolderName()))
                        game = Game(GAME_FONV, game.FolderName());
                    else
                        continue;

                    games.push_back(game.SetDetails(name, master, url, path, registry));
                }
            }
        }
        return games;
    }

    void DetectGames(std::vector<Game>& detected, std::vector<Game>& undetected) {
        //detected is also the input vector.
        vector<Game>::iterator it = detected.begin();
        while (it != detected.end()) {
            if (!it->IsInstalled()) {
                undetected.push_back(*it);
                it = detected.erase(it);
            } else
                ++it;
        }
    }

    Game::Game() : id(0) {}

    Game::Game(const std::string& folder) : bossFolderName(folder) {}

    Game::Game(const unsigned int gameCode, const std::string& folder) : id(gameCode) {
        string libespmGame;
        if (Id() == GAME_TES4) {
            _name = "TES IV: Oblivion";
            registryKey = "Software\\Bethesda Softworks\\Oblivion\\Installed Path";
            bossFolderName = "Oblivion";
            _masterFile = "Oblivion.esm";
            libespmGame = "Oblivion";
            _masterlistURL = "http://better-oblivion-sorting-software.googlecode.com/svn/data/boss-oblivion/masterlist.yaml";
        } else if (Id() == GAME_TES5) {
            _name = "TES V: Skyrim";
            registryKey = "Software\\Bethesda Softworks\\Skyrim\\Installed Path";
            bossFolderName = "Skyrim";
            _masterFile = "Skyrim.esm";
            libespmGame = "Skyrim";
            _masterlistURL = "http://better-oblivion-sorting-software.googlecode.com/svn/data/boss-skyrim/masterlist.yaml";
        } else if (Id() == GAME_FO3) {
            _name = "Fallout 3";
            registryKey = "Software\\Bethesda Softworks\\Fallout3\\Installed Path";
            bossFolderName = "Fallout3";
            _masterFile = "Fallout3.esm";
            libespmGame = "Fallout3";
            _masterlistURL = "http://better-oblivion-sorting-software.googlecode.com/svn/data/boss-fallout/masterlist.yaml";
        } else if (Id() == GAME_FONV) {
            _name = "Fallout: New Vegas";
            registryKey = "Software\\Bethesda Softworks\\FalloutNV\\Installed Path";
            bossFolderName = "FalloutNV";
            _masterFile = "FalloutNV.esm";
            libespmGame = "FalloutNV";
            _masterlistURL = "http://better-oblivion-sorting-software.googlecode.com/svn/data/boss-fallout-nv/masterlist.yaml";
        } else
            throw error(ERROR_INVALID_ARGS, "Invalid game ID supplied.");

        if (!folder.empty())
            bossFolderName = folder;

        if (fs::exists(libespm_options_path))
            espm_settings = espm::Settings(libespm_options_path, libespmGame);
        else
            throw error(ERROR_PATH_NOT_FOUND, "Libespm settings file could not be found.");
    }

    Game& Game::SetDetails(const std::string& name, const std::string& masterFile,
                        const std::string& url, const std::string& path, const std::string& registry) {

        if (!name.empty())
            _name = name;

        if (!masterFile.empty())
            _masterFile = masterFile;

        if (!url.empty())
            _masterlistURL = url;

        if (!path.empty())
            gamePath = path;

        if (!registry.empty())
            registryKey = registry;

        return *this;
    }
                      
    Game& Game::Init() {
        //First look for local install, then look for Registry.
        if (gamePath.empty() || !fs::exists(gamePath / "Data" / _masterFile)) {
            if (fs::exists(fs::path("..") / "Data" / _masterFile))
                gamePath = "..";
            else {
                string path;
                string key_parent = fs::path(registryKey).parent_path().string();
                string key_name = fs::path(registryKey).filename().string();
                if (RegKeyExists("HKEY_LOCAL_MACHINE", key_parent, key_name)) {
                    path = RegKeyStringValue("HKEY_LOCAL_MACHINE", key_parent, key_name);
                    if (fs::exists(fs::path(path) / "Data" / _masterFile))
                        gamePath = fs::path(path);
                }
            }
        }

        if (gamePath.empty())
            throw error(ERROR_PATH_NOT_FOUND, "Game path could not be detected.");

        RefreshActivePluginsList();
        CreateBOSSGameFolder();

        return *this;
    }
    
    bool Game::IsInstalled() const {
        if (!gamePath.empty() && fs::exists(gamePath / "Data" / _masterFile))
            return true;

        if (fs::exists(fs::path("..") / "Data" / _masterFile))
            return true;

        string path;
        string key_parent = fs::path(registryKey).parent_path().string();
        string key_name = fs::path(registryKey).filename().string();
        if (RegKeyExists("HKEY_LOCAL_MACHINE", key_parent, key_name)) {
            path = RegKeyStringValue("HKEY_LOCAL_MACHINE", key_parent, key_name);
            if (fs::exists(fs::path(path) / "Data" / _masterFile))
                return true;
        }

        return false;
    }

    bool Game::operator == (const Game& rhs) const {
        return (_name == rhs.Name() || bossFolderName == rhs.FolderName());
    }

    unsigned int Game::Id() const {
        return id;
    }

    string Game::Name() const {
        return _name;
    }

    string Game::FolderName() const {
        return bossFolderName;
    }

    std::string Game::Master() const {
        return _masterFile;
    }

    std::string Game::RegistryKey() const {
        return registryKey;
    }

    std::string Game::URL() const {
        return _masterlistURL;
    }


    fs::path Game::GamePath() const {
        return gamePath;
    }

    fs::path Game::DataPath() const {
        return GamePath() / "Data";
    }

    fs::path Game::MasterlistPath() const {
        return fs::path(bossFolderName) / "masterlist.yaml";
    }
    
    fs::path Game::UserlistPath() const {
        return fs::path(bossFolderName) / "userlist.yaml";
    }
    
    fs::path Game::ReportPath() const {
        return fs::path(bossFolderName) / "report.html";
    }

    void Game::RefreshActivePluginsList() {
        lo_game_handle gh;
        char ** pluginArr;
        size_t pluginArrSize;
        int ret;
        if (Id() == GAME_TES4)
            ret = lo_create_handle(&gh, LIBLO_GAME_TES4, gamePath.string().c_str());
        else if (Id() == GAME_TES5)
            ret = lo_create_handle(&gh, LIBLO_GAME_TES5, gamePath.string().c_str());
        else if (Id() == GAME_FO3)
            ret = lo_create_handle(&gh, LIBLO_GAME_FO3, gamePath.string().c_str());
        else if (Id() == GAME_FONV)
            ret = lo_create_handle(&gh, LIBLO_GAME_FNV, gamePath.string().c_str());

        if (ret != LIBLO_OK)
            throw error(ERROR_LIBLO_ERROR, "libloadorder game handle creation failed.");

        ret = lo_set_game_master(gh, _masterFile.c_str());

        if (ret != LIBLO_OK)
            throw error(ERROR_LIBLO_ERROR, "libloadorder total conversion support setup failed.");
                
        if (lo_get_active_plugins(gh, &pluginArr, &pluginArrSize) != LIBLO_OK)
            throw error(ERROR_LIBLO_ERROR, "Active plugin list lookup failed.");
        else {
            for (size_t i=0; i < pluginArrSize; ++i) {
                activePlugins.insert(string(pluginArr[i]));
            }
        }

        lo_destroy_handle(gh);
    }

    bool Game::IsActive(const std::string& plugin) const {
        return activePlugins.find(boost::to_lower_copy(plugin)) != activePlugins.end();
    }

    void Game::SetLoadOrder(const std::list<Plugin>& loadOrder) const {
        lo_game_handle gh;
        char ** pluginArr;
        size_t pluginArrSize;

        int ret;
        if (Id() == GAME_TES4)
            ret = lo_create_handle(&gh, LIBLO_GAME_TES4, gamePath.string().c_str());
        else if (Id() == GAME_TES5)
            ret = lo_create_handle(&gh, LIBLO_GAME_TES5, gamePath.string().c_str());
        else if (Id() == GAME_FO3)
            ret = lo_create_handle(&gh, LIBLO_GAME_FO3, gamePath.string().c_str());
        else if (Id() == GAME_FONV)
            ret = lo_create_handle(&gh, LIBLO_GAME_FNV, gamePath.string().c_str());

        if (ret != LIBLO_OK)
            throw error(ERROR_LIBLO_ERROR, "libloadorder game handle creation failed.");

        ret = lo_set_game_master(gh, _masterFile.c_str());

        if (ret != LIBLO_OK)
            throw error(ERROR_LIBLO_ERROR, "libloadorder total conversion support setup failed.");

        pluginArrSize = loadOrder.size();
        pluginArr = new char*[pluginArrSize];
        int i = 0;
        for (list<Plugin>::const_iterator it=loadOrder.begin(),endIt=loadOrder.end(); it != endIt; ++it) {
            pluginArr[i] = new char[it->Name().length() + 1];
            strcpy(pluginArr[i], it->Name().c_str());
            ++i;
        }
                
        if (lo_set_load_order(gh, pluginArr, pluginArrSize) != LIBLO_OK) {
            for (size_t i=0; i < pluginArrSize; i++)
                delete [] pluginArr[i];
            delete [] pluginArr;
            throw error(ERROR_LIBLO_ERROR, "Setting load order failed.");
        }
        
        for (size_t i=0; i < pluginArrSize; i++)
            delete [] pluginArr[i];
        delete [] pluginArr;
    
        lo_destroy_handle(gh);
    }

    void Game::CreateBOSSGameFolder() {
        //Make sure that the BOSS game path exists.
        try {
            if (!fs::exists(bossFolderName))
                fs::create_directory(bossFolderName);
        } catch (fs::filesystem_error& e) {
            throw error(ERROR_PATH_WRITE_FAIL, "Could not create BOSS folder for game.");
        }
    }
}
