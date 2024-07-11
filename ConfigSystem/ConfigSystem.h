#pragma once
#include <unordered_map>
#include <tuple>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "Mutex.hpp"

#include "json.hpp"


namespace fs = std::filesystem;
using json = nlohmann::json;

typedef uintptr_t Pointer;

class ConfigContext {
public:
	ConfigContext(bool _enableSaving, std::string _myName) {
		myName = _myName;
		enableSaving = _enableSaving;
	}

	std::unordered_map<std::string, std::tuple<int, Pointer>> saveValues = {};
	Mutex contextMutex = {};
	bool enableSaving = false;
	std::string myName = {};


	bool exists(std::string key) {
		if (saveValues.contains(key))
			return true;
		return false;
	}

	void make(std::string key, int typeSize) {
		Pointer buffer = (Pointer)new unsigned char[typeSize];
		saveValues[key] = std::tuple<int, Pointer>(typeSize, buffer);
	}

	void ensure(std::string key, int typeSize, Pointer defaultValue = 0) {
		if (!exists(key)) {
			make(key, typeSize);
			Pointer buf = std::get<1>(saveValues[key]);
			memset((void*)buf, 0, typeSize);
			if (defaultValue)
				memcpy((void*)buf, (void*)defaultValue, typeSize);
		}
	}

	template <typename T>
	T* get(std::string key, T defaultValue = T()) {
		auto l = contextMutex.lock();
		ensure(key, sizeof(T), (Pointer)&defaultValue);
		return (T*)std::get<1>(saveValues[key]);
	}

	template <typename T>
	T getValue(std::string key, T defaultValue = T()) {
		auto l = contextMutex.lock();
		ensure(key, sizeof(T), (Pointer)&defaultValue);
		return *(T*)std::get<1>(saveValues[key]);
	}

	template <typename T>
	void set(std::string key, T val) {
		auto l = contextMutex.lock();
		ensure(key, sizeof(T));
		*(T*)std::get<1>(saveValues[key]) = val;
	}

	template <typename T>
	void remove(std::string key) {
		auto l = contextMutex.lock();
		ensure(key, sizeof(T));
		delete[] (void*)std::get<1>(saveValues[key]);
		saveValues.erase(key);
	}
};

typedef class CFG
{
private:
	static ConfigContext mainContext;

	static std::unordered_map<std::string, ConfigContext*> configContexts;
	static std::string savePath;
	static std::string saveFileName;
public:

	static inline ConfigContext* makeContext(std::string name, bool enableSaving) {
		if (configContexts.contains(name))
			deleteContext(name);
		configContexts[name] = new ConfigContext(enableSaving, name);
		return getContext(name);
	}

	static inline ConfigContext* getContext(std::string name) {
		if (!configContexts.contains(name))
			return makeContext(name, true);
		return configContexts[name];
	}

	static inline void deleteContext(std::string name) {
		if (!configContexts.contains(name))
			return;
		auto context = getContext(name);
		auto l = context->contextMutex.lock();
		if (context->saveValues.size() > 0)
			for (auto value : context->saveValues)
				delete (void*)std::get<1>(value.second);
		configContexts.erase(name);
		delete context;
	}

	static inline void setSavePath(std::string _savePath, std::string _saveFileName) {
		savePath = _savePath;
		saveFileName = _saveFileName;
	}

	static inline std::string getAppDataPath() {
		return std::string(getenv("APPDATA")) + "\\";
	}

	static inline void save() {
		fs::create_directories(savePath);

		json saveOBJ;
		configContexts["main"] = &mainContext;
		for (auto context : configContexts) {
			if (!context.second->enableSaving)
				continue;
			json contextOBJ;
			for (auto val : context.second->saveValues) {
				json value;
				int dataLen = std::get<0>(val.second);
				Pointer data = std::get<1>(val.second);

				json binData;
				for (int i = 0; i < dataLen; i++) {
					binData[std::to_string(i)] = *(uint8_t*)(data + i);
				}

				value["data"] = binData;
				value["dlen"] = dataLen;
				value["key"] = val.first;

				contextOBJ["values"].push_back(value);
			}
			contextOBJ["name"] = context.second->myName;
			saveOBJ.push_back(contextOBJ);
		}
		configContexts.erase("main");

		std::ofstream ofs(savePath + saveFileName);
		ofs << saveOBJ;
		ofs.close();
	}

	static inline bool load() {
		std::string path = savePath + saveFileName;

		if (!fs::exists(path))
			return false;

		std::ifstream ifs(path);
		std::stringstream ss;
		ss << ifs.rdbuf();

		json saveOBJ;
		//Why try-catch ? Because fuck json
		try {
			saveOBJ = json::parse(ss.str());
		}
		catch (...) {
			return false;
		}

		ifs.close();


		for (auto context : saveOBJ) {

			json values = context["values"];
			std::string name = context["name"];

			ConfigContext* cfgContext = nullptr;

			if (name != "main") {
				cfgContext = new ConfigContext(true, name);
				configContexts[name] = cfgContext;
			}
			else
				cfgContext = &mainContext;

			for (auto value : values) {
				std::string key = value["key"];
				int len = value["dlen"];
				json dataBin = value["data"];

				cfgContext->ensure(key, len);

				Pointer buf = std::get<1>(cfgContext->saveValues[key]);

				for (int i = 0; i < len; i++) {
					*(uint8_t*)(buf + i) = dataBin[std::to_string(i)];
				}
			}
		}
		if (configContexts.contains("main")) {
			memcpy(&mainContext, configContexts["main"], sizeof(ConfigContext));
			configContexts.erase("main");
		}
		return true;
	}


	template <typename T>
	static inline T* get(std::string key, T defaultValue = T()) {
		auto l = mainContext.contextMutex.lock();
		mainContext.ensure(key, sizeof(T), (Pointer)&defaultValue);
		return (T*)std::get<1>(mainContext.saveValues[key]);
	}

	template <typename T>
	static inline T getValue(std::string key, T defaultValue = T()) {
		auto l = mainContext.contextMutex.lock();
		mainContext.ensure(key, sizeof(T), (Pointer)&defaultValue);
		return *(T*)std::get<1>(mainContext.saveValues[key]);
	}

	template <typename T>
	static inline void set(std::string key, T val) {
		auto l = mainContext.contextMutex.lock();
		mainContext.ensure(key, sizeof(T));
		*(T*)std::get<1>(mainContext.saveValues[key]) = val;
	}

	template <typename T>
	static inline void remove(std::string key) {
		auto l = mainContext.contextMutex.lock();
		mainContext.ensure(key, sizeof(T));
		delete[] (void*)std::get<1>(mainContext.saveValues[key]);
		mainContext.saveValues.erase(key);
	}
} cfg, Cfg, ConfigSystem, Config, config;

