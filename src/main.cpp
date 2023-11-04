#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <mirror/logger.h>    

#include "schedule.h"
#include "mirrors.h"
#include "queue.h"

int main(){
    //initialize and configure connection to log server
    mirror::Logger& logger = mirror::Logger::getInstance();
    logger.configure(4357, "sync-scheduler");

    //read in mirrors.json from file
    json config = readMirrors();

    //create and build new schedule
    Schedule& schedule = Schedule::getInstance();
    //build the schedule based on the mirrors.json config
    schedule.build(config);

    //create job queue class
    Queue& queue = Queue::getInstance();
    //start the queue (second parameter is number of threads)
    queue.startQueue(config, 4);

    std::vector<std::string> *name;
    int seconds_to_sleep;
    while(true){
        //get the name of the next job and how long we have to sleep till the next job from the schedule
        name = schedule.nextJob(seconds_to_sleep);

        //print the next jobs and the time to sleep
        for(int i = 0; i < name->size(); i++){
            std::cout << (*name)[i] << " " << std::endl;
        }
        std::cout << seconds_to_sleep << std::endl;

        //sleep till the next job
        std::this_thread::sleep_for(std::chrono::seconds(seconds_to_sleep));

        //add job names to job queue
        queue.push_back_list(name);
    }
    
    return 0;
}