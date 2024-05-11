#include "ConfigSystem.h"
#include <string>


std::string CFG::savePath = {};
std::string CFG::saveFileName = {};

ConfigContext CFG::mainContext = ConfigContext(true,"main");
std::unordered_map<std::string, ConfigContext*> CFG::configContexts = {};