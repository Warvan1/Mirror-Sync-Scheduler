#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <mirror/logger.h>

#include "rsync.h"
#include "mirrors.h"

void syncProject(std::string name, json &config, mirror::Logger& logger){
    //do rsync task here
    std::cout << name << " started" << std::endl;
    logger.info(name + " started");
    std::this_thread::sleep_for(std::chrono::seconds(10));
    printJson(config);
    std::cout << name << " completed" << std::endl;
    logger.info(name + " completed");
}