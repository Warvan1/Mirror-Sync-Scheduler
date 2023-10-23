#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "rsync.h"

void syncProject(std::string name, json &config){
    //do rsync task here
    std::cout << name << " started" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
    std::cout << name << " completed" << std::endl;
}