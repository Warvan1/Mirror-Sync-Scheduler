#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <thread>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "schedule.h"
#include "rsync.h"

//prints a json object
void printJson(json object){
    for (auto& x : object.items()){
        std::cout << "key: " << x.key() << ", value: " << x.value() << '\n';
    }
}

int main(){
    //read in mirrors.json from file
    std::ifstream f("configs/mirrors.json");
    json config = json::parse(f);
    f.close();

    //create Task vector
    std::vector<Task> tasks;
    //create a vector of task structs from mirrors.json
    for (auto& x : config["mirrors"].items()){
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

    //create a new schedule
    Schedule schedule(tasks);
    //verify that the schedule passes sanity checks
    bool success = schedule.verify(tasks);
    std::cout << success << std::endl;

    //used to prevent a project from syncing twice at the same time.
    std::map<std::string, bool> syncLocks;
    for(int i = 0; i < tasks.size(); i++){
        syncLocks[tasks[i].name] = false;
    }

    std::string name;
    int seconds_to_sleep;
    while(true){
        //get the name of the next job and how long we have to sleep till the next job from the schedule
        schedule.nextJob(name, seconds_to_sleep);
        std::cout << name << " " << seconds_to_sleep << std::endl;
        //sleep till the next job
        std::this_thread::sleep_for(std::chrono::seconds(seconds_to_sleep));

        //run syncProject in a seperate detatched thread
        std::thread pt(syncProject, name, std::ref(syncLocks), std::ref(config["mirrors"]));
        pt.detach();
    }
    
    return 0;
}