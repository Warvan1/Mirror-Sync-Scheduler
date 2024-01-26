#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <fstream>

#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <mirror/logger.hpp>    

#include "schedule.h"
#include "queue.h"

//read json in from a file
json readJSONFromFile(std::string filename){
    std::ifstream f(filename);
    json config = json::parse(f);
    f.close();
    return config;
}

//function to handle std::cin in a seperate thread
void cin_thread(){
    //create a pointer to the queue
    Queue* queue = Queue::getInstance();
    //create a pointer to the schedule
    Schedule* schedule = Schedule::getInstance();

    while(true){
        std::string x;
        std::cin >> x;
        //reload the sync scheduler
        if(x == "reload"){
            //read mirror data in from mirrors.json
            json config = readJSONFromFile("configs/mirrors.json");

            //build the schedule based on the mirrors.json config
            schedule->build(config["mirrors"]);
            std::cout << "Reloaded mirrors.json" << std::endl;
        }
        //manually sync a project in a detached thread
        else{
            queue->manual_sync(x);
        }
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
    //read env data in from env.json
    json env = readJSONFromFile("configs/env.json");

    //read mirror data in from mirrors.json
    json config = readJSONFromFile("configs/mirrors.json");

    //initialize and configure connection to log server
    mirror::Logger* logger = mirror::Logger::getInstance();
    logger->configure(env["logServerPort"], "sync-scheduler");

    //create and build new schedule
    Schedule* schedule = Schedule::getInstance();
    //build the schedule based on the mirrors.json config
    schedule->build(config["mirrors"]);

    //create a pointer to the job queue class
    Queue* queue = Queue::getInstance();
    //set queue dryrun
    queue->setDryrun(env["dryrun"]);
    //generate the sync command maps
    queue->createSyncCommandMap(config["mirrors"]);
    //start the queue (second parameter is number of threads)
    queue->startQueue(env["queueThreads"]);

    //keep alive thrad
    std::thread kt(keep_alive_thread);

    //cin thread for program input
    std::thread ct(cin_thread);

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