#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <signal.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <mirror/logger.h>    

#include "schedule.h"
#include "mirrors.h"
#include "queue.h"

void exit_handler(int s){
    Queue* queue = Queue::getInstance();
    queue->setQueueStoped(true);
    while(queue->getQueueRunning()){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    exit(0);
}

int main(){
    //initialize and configure connection to log server
    mirror::Logger* logger = mirror::Logger::getInstance();
    logger->configure(4357, "sync-scheduler");

    //read in mirrors.json from file
    json config = readMirrors();

    //create and build new schedule
    Schedule* schedule = Schedule::getInstance();
    //build the schedule based on the mirrors.json config
    schedule->build(config);

    //create a pointer to the job queue class
    Queue* queue = Queue::getInstance();
    //start the queue (second parameter is number of threads)
    queue->startQueue(config, 4);

    //catch ctrl c to perform clean exits
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = exit_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    std::vector<std::string>* name;
    int seconds_to_sleep;
    while(true){
        //get the name of the next job and how long we have to sleep till the next job from the schedule
        name = schedule->nextJob(seconds_to_sleep);

        //print the next jobs and the time to sleep
        for(int i = 0; i < name->size(); i++){
            std::cout << (*name)[i] << " " << std::endl;
        }
        std::cout << seconds_to_sleep << std::endl;

        //sleep till the next job
        std::this_thread::sleep_for(std::chrono::seconds(seconds_to_sleep));

        //add job names to job queue
        queue->push_back_list(name);
    }
    
    return 0;
}