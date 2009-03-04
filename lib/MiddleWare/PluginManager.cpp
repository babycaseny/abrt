/*
    PluginManager.cpp

    Copyright (C) 2009  Zdenek Prikryl (zprikryl@redhat.com)
    Copyright (C) 2009  RedHat inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    */

#include <iostream>
#include "PluginManager.h"
#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include "Settings.h"

CPluginManager::CPluginManager(const std::string& pPlugisConfDir,
							   const std::string& pPlugisLibDir) :
	m_sPlugisConfDir(pPlugisConfDir),
	m_sPlugisLibDir(pPlugisLibDir)
{}

CPluginManager::~CPluginManager()
{}

void CPluginManager::LoadPlugins()
{
	DIR *dir = opendir(m_sPlugisConfDir.c_str());
	struct dirent *dent = NULL;
	if (dir != NULL)
	{
		while ((dent = readdir(dir)) != NULL)
		{
			if (dent->d_type == DT_REG)
			{
				std::string name = dent->d_name;
				std::string extension = name.substr(name.length()-sizeof(PLUGINS_CONF_EXTENSION)+1);
				if (extension == PLUGINS_CONF_EXTENSION)
				{
					name.erase(name.length()-sizeof(PLUGINS_CONF_EXTENSION));
					LoadPlugin(name);
				}
			}
		}
		closedir(dir);
	}
}

void CPluginManager::UnLoadPlugins()
{
    map_abrt_plugins_t::iterator it_p;
      while ((it_p = m_mapABRTPlugins.begin()) != m_mapABRTPlugins.end())
      {
          std::string pluginName = it_p->first;
          UnLoadPlugin(pluginName);
      }
}

void CPluginManager::LoadPlugin(const std::string& pName)
{
	if (m_mapABRTPlugins.find(pName) == m_mapABRTPlugins.end())
	{
		CABRTPlugin* abrtPlugin = NULL;
		try
		{
			std::string libPath = m_sPlugisLibDir + "/lib" + pName + "." + PLUGINS_LIB_EXTENSIONS;
			abrtPlugin = new CABRTPlugin(libPath);
			if (abrtPlugin->GetMagicNumber() != PLUGINS_MAGIC_NUMBER ||
			    (abrtPlugin->GetType() < LANGUAGE && abrtPlugin->GetType() > DATABASE))
			{
				throw std::string("non-compatible plugin");
			}
			std::cerr << "Plugin " << pName << " (" << abrtPlugin->GetVersion() << ") " << "succesfully loaded." << std::endl;
			m_mapABRTPlugins[pName] = abrtPlugin;
		}
		catch (std::string sError)
		{
			if (abrtPlugin != NULL)
			{
				delete abrtPlugin;
			}
			std::cerr << "Failed to load plugin " << pName << " (" << sError << ")." << std::endl;
		}
	}
}

void CPluginManager::UnLoadPlugin(const std::string& pName)
{
	if (m_mapABRTPlugins.find(pName) != m_mapABRTPlugins.end())
	{
		UnRegisterPlugin(pName);
		delete m_mapABRTPlugins[pName];
		m_mapABRTPlugins.erase(pName);
		std::cerr << "Plugin " << pName << " sucessfully unloaded." << std::endl;
	}
}


void CPluginManager::RegisterPlugin(const std::string& pName)
{
    if (m_mapABRTPlugins.find(pName) != m_mapABRTPlugins.end())
    {
        if (m_mapPlugins.find(pName) == m_mapPlugins.end())
        {
            map_settings_t settings;
            std::string path = m_sPlugisConfDir + "/" + pName + "." + PLUGINS_CONF_EXTENSION;
            load_settings(path, settings);
            CPlugin* plugin = m_mapABRTPlugins[pName]->PluginNew();
            plugin->Init();
            plugin->SetSettings(settings);
            m_mapPlugins[pName] = plugin;
            std::cerr << "Registred plugin " << pName << "("
                      << plugin_type_str_t[m_mapABRTPlugins[pName]->GetType()]
                      << ")" << std::endl;
        }
    }
}

void CPluginManager::UnRegisterPlugin(const std::string& pName)
{
    if (m_mapABRTPlugins.find(pName) != m_mapABRTPlugins.end())
    {
        if (m_mapPlugins.find(pName) != m_mapPlugins.end())
        {
            m_mapPlugins[pName]->DeInit();
            delete m_mapPlugins[pName];
            m_mapPlugins.erase(pName);
            std::cerr << "UnRegistred plugin " << pName << "("
                      << plugin_type_str_t[m_mapABRTPlugins[pName]->GetType()]
                      << ")" << std::endl;
        }
    }
}

CLanguage* CPluginManager::GetLanguage(const std::string& pName)
{
	if (m_mapPlugins.find(pName) == m_mapPlugins.end())
	{
		throw std::string("CPluginManager::GetLanguage():"
				          "Language plugin: '"+pName+"' is not loaded.");
	}
	return (CLanguage*) m_mapPlugins[pName];
}

CReporter* CPluginManager::GetReporter(const std::string& pName)
{
	if (m_mapPlugins.find(pName) == m_mapPlugins.end())
	{
		throw std::string("CPluginManager::GetReporter():"
				          "Reporter plugin: '"+pName+"' is not loaded.");
	}
	return (CReporter*) m_mapPlugins[pName];
}

CApplication* CPluginManager::GetApplication(const std::string& pName)
{
	if (m_mapPlugins.find(pName) == m_mapPlugins.end())
	{
		throw std::string("CPluginManager::GetApplication():"
				          "Application plugin: '"+pName+"' is not loaded.");
	}
	return (CApplication*) m_mapPlugins[pName];
}

CDatabase* CPluginManager::GetDatabase(const std::string& pName)
{
	if (m_mapPlugins.find(pName) == m_mapPlugins.end())
	{
		throw std::string("CPluginManager::GetDatabase():"
				          "Database plugin: '"+pName+"' is not loaded.");
	}
	return (CDatabase*) m_mapPlugins[pName];
}

