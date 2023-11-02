#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "mirrors.h"

//prints a json object
void printJson(json object){
    for (auto& x : object.items()){
        std::cout << "key: " << x.key() << ", value: " << x.value() << '\n';
    }
}

//read mirrors.json in from file
json readMirrors(){
    //read in mirrors.json from file
    std::ifstream f("configs/mirrors.json");
    json config = json::parse(f);
    f.close();
    return config["mirrors"];
}

//read mirrors.json into a list of tasks
std::vector<Task> parseTasks(json &config){
    std::vector<Task> tasks;
    //create a vector of task structs from mirrors.json
    for (auto& x : config.items()){
        json Xvalue = x.value();
        json rsync = Xvalue["rsync"];
        json script = Xvalue["script"];

        if(!rsync.is_null()){
            Task task;
            task.name = x.key();
            task.syncs = rsync.value("syncs_per_day", 0);
            tasks.push_back(task);
        }
        else if(!script.is_null()){
            Task task;
            task.name = x.key();
            task.syncs = script.value("syncs_per_day", 0);
            tasks.push_back(task);
        }
    }
    return tasks;
}