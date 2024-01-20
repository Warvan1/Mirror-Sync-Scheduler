#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <mirror/logger.hpp>    

#include "schedule.h"
#include "readJson.h"
#include "queue.h"

//temp function to handle std::cin in a seperate thread
void temp_cin_thread(){
    Queue* queue = Queue::getInstance();
    while(true){
        std::string x;
        std::cin >> x;
        //manually sync a project in a detached thread based on cin input
        queue->manual_sync(x);
    }
}

//thread that sends a message to the log server every 29 minutes
//this keeps the socket from closing
void keep_alive_thread(){
    mirror::Logger* logger = mirror::Logger::getInstance();

    while(true){
        //sleep for 29 minutes
        std::this_thread::sleep_for(std::chrono::minutes(29));
        //send keepalive message
        logger->info("keep alive.");
    }
}

int main(){
    //read in mirrors.json from mirrorapi
    json config = readJSONFromHTTP("localhost", 8080, "/api/mirrors", "");

    //read env data in from env.json
    json env = readJSONFromFile("configs/env.json");

    //initialize and configure connection to log server
    mirror::Logger* logger = mirror::Logger::getInstance();
    logger->configure(env["logServerPort"], "sync-scheduler");

    //create and build new schedule
    Schedule* schedule = Schedule::getInstance();
    //build the schedule based on the mirrors.json config
    schedule->build(config);

    //create a pointer to the job queue class
    Queue* queue = Queue::getInstance();
    //set queue dryrun
    queue->setDryrun(env["dryrun"]);
    //generate the sync command maps
    queue->createSyncCommandMap(config);
    //start the queue (second parameter is number of threads)
    queue->startQueue(env["queueThreads"]);

    //keep alive thrad
    std::thread kt(keep_alive_thread);

    //temp cin thread for manual sync
    std::thread ct(temp_cin_thread);

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