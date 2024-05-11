#include <iostream>
#include "Configsystem.h"

int main()
{
    cfg::setSavePath("./", "test.json");
    cfg::load();


    auto context = cfg::getContext("testContext");

    std::cout << "testValue: " << context->getValue<bool>("testValue", false) << std::endl;;
    context->set<bool>("testValue", true);


    std::cout << "Test 2: " << cfg::getValue<bool>("Test 2", false) << std::endl;
    cfg::set<bool>("Test 2", true);


    cfg::save();
}
