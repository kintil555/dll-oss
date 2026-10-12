#include "Manager.hpp"

#include "Modules/Misc/Input/GUIKeyListener.hpp"
#include "Modules/Misc/SaveConfig/SaveConfigListener.hpp"
#include "Modules/Misc/RGB/rgbListener.hpp"
#include "Modules/Misc/TextAlias/TextAliasListener.hpp"
#include "Modules/ClickGUI/ClickGUI.hpp"
#include "Modules/Module.hpp"
#include "Modules/Misc/CentreCursor/CentreCursorListener.hpp"
#include "Modules/Misc/Uninject/UninjectListener.hpp"
#include "Modules/Misc/HiveModeCatcher/HiveModeCatcherListener.hpp"
#include <Modules/Misc/InputImGUi/GUIMouseListener.hpp>
#include <Modules/Misc/InputImGUi/GUIKeyListener.hpp>
#include <Modules/Misc/PackChanger/PackChanger.hpp>
#include <Modules/Misc/ScriptMarketplace/ScriptMarketplace.hpp>



#include "Scripting/ScriptManager.hpp"

#include "Modules/Misc/F1Listener/F1Listener.hpp"

#ifdef COMPILE_DOOM
#endif

void ModuleManager::getModules() {
	for (const auto& pair : moduleMap) {
		if (pair.second == nullptr) continue;
		modulesVector.push_back(pair.second);
	}
}

bool compareEnabled(std::shared_ptr<Module>& obj1, std::shared_ptr<Module>& obj2) {
	return obj1->isEnabled() >
		obj2->isEnabled();
}

bool compareFavorite(std::shared_ptr<Module>& obj1, std::shared_ptr<Module>& obj2) {
	return obj1->settings.getSettingByName<bool>("favorite")->value >
		obj2->settings.getSettingByName<bool>("favorite")->value;
}


bool compareNames(std::shared_ptr<Module>& obj1, std::shared_ptr<Module>& obj2) {
	return obj1->name < obj2->name;
}


void ModuleManager::updateModulesVector() {
	if (modulesVector.empty()) getModules();
	if (Client::settings.getSettingByName<bool>("enabledModulesOnTop")->value)
		std::sort(modulesVector.begin(), modulesVector.end(), compareEnabled);
	else std::sort(modulesVector.begin(), modulesVector.end(), compareNames);
	std::sort(modulesVector.begin(), modulesVector.end(), compareFavorite);
}


void ModuleManager::initialize() {
	addModule<ClickGUI>();

	addService<GUIKeyListener>();
	if (!VersionUtils::checkAboveOrEqual(21, 60)) {
		addService<PackChanger>();
	}
	addService<UninjectListener>();
	addService<SaveConfigListener>();
	addService<CentreCursorListener>();
	addService<rgbListener>();
	addService<HiveModeCatcherListener>();
	addService<ImGUIMouseListener>();
	addService<ImGUIKeyListener>();
	addService<ScriptMarketplace>();
	addService<F1Listener>();

	initialized = true;
}

void ModuleManager::terminate() {
	initialized = false;
	for (const auto& val : moduleMap | std::views::values) if (val != nullptr) val->terminate();
	moduleMap.clear();
	services.clear();
}

void ModuleManager::restart() {
	initialized = false;
	Client::LoadSettings();
	for (const auto& val : moduleMap | std::views::values) {
		if (val) {
			std::shared_ptr mod = getModule(val->name);
			if (mod != nullptr) {
				bool old = mod->enabledState;
				if (mod->isEnabled()) mod->onDisable();
				mod->settings.reset();
				mod->prevAlignments = std::vector<DWRITE_TEXT_ALIGNMENT>(100, DWRITE_TEXT_ALIGNMENT_JUSTIFIED);
				mod->loadSettings();
			}
		}
	}
	
	initialized = true;
	Client::PerformPostLegacySetup();
	ScriptManager::reloadScripts();
	Client::SaveSettings();
}


void ModuleManager::syncState() {
	if (!initialized) return;

	ScriptManager::_reloadScripts();

	if (restartModules) {
		restartModules = false;
		restart();
		return;
	}
	for (const auto& module : moduleMap | std::views::values) {
		if (!module || module->enabledState == module->isEnabled() || module->delayDisable) continue;

		if (module->enabledState) module->onEnable();
		else module->onDisable();
	}
}

// TODO: use enums?
bool ModuleManager::doesAnyModuleHave(const std::string& settingName) {

	bool result = false;

	if (!ModuleManager::initialized) return false;

	for (const auto& pair : moduleMap) {
		if (!pair.second) continue;

		if (pair.second->settings.getSettingByName<bool>(settingName) != nullptr)
			if (pair.second->settings.getSettingByName<bool>(settingName)->value &&
				pair.second->isEnabled() && (pair.second->active || pair.second->defaultKeybind.empty())) {
				result = true;
				break;
			}
	}

	return result;

}

std::shared_ptr<Module> ModuleManager::getModule(const std::string& name) {
	size_t hash = std::hash<std::string>{}(name);

	auto it = moduleMap.find(hash);

	if (it != moduleMap.end()) return moduleMap[hash];
	else return nullptr;
}
