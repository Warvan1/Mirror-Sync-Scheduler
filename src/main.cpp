#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "schedule.h"
#include "mirrors.h"
#include "queue.h"

int main(){
    //read in mirrors.json from file
    json config = readMirrors();
    //create Task vector from mirrors.json
    std::vector<Task> tasks = parseTasks(config);

    //create a new schedule
    Schedule schedule(tasks);
    //verify that the schedule passes sanity checks
    bool success = schedule.verify(tasks);
    std::cout << success << std::endl;

    //create job queue class
    Queue queue;
    //launch jobqueueThread on seperate thread 
    //jobqueueThread runs all the jobs that get entered into the queue
    std::thread jt(&Queue::jobQueueThread, &queue, std::ref(config), 4);
    //run the below thread instead if you want to only use a single thread on the queue
    // std::thread jt(&Queue::jobQueueThread_single, &queue, std::ref(config));

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

    //join our job queue thread before we end the program.
    jt.join();
    
    return 0;
}