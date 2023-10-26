#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <list>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "schedule.h"
#include "rsync.h"

//mutex lock for threads
std::mutex tLock;

//prints a json object
void printJson(json object){
    for (auto& x : object.items()){
        std::cout << "key: " << x.key() << ", value: " << x.value() << '\n';
    }
}

// runs in a seperate thread
// if there is a job in the queue it gets ran and then removed from the front of the queue in a loop
void jobQueueThread(std::list<std::string> &queue, json &config){
    while(true){
        tLock.lock();
        bool queueEmpty = queue.empty();
        tLock.unlock();

        if(!queueEmpty){
            //run the first job in the queue
            syncProject(queue.front(), config);
            //remove first item from queue
            tLock.lock();
            queue.pop_front();
            std::cout << queue.size() << std::endl;
            tLock.unlock();
        }

        //sleep for 5 seconds so that we arnt running constantly and to prevent constant locking
        std::this_thread::sleep_for(std::chrono::seconds(5));
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

    //jobqueue (locked with tLock)
    std::list<std::string> queue;

    //jobqueueThread runs all the jobs that get entered into the queue
    std::thread jt(jobQueueThread, std::ref(queue), std::ref(config));

    std::vector<std::string> *name;
    int seconds_to_sleep;
    while(true){
        //get the name of the next job and how long we have to sleep till the next job from the schedule
        name = schedule.nextJob(seconds_to_sleep);

        for(int i = 0; i < name->size(); i++){
            std::cout << (*name)[i] << " " << std::endl;
        }
        std::cout << seconds_to_sleep << std::endl;

        //sleep till the next job
        std::this_thread::sleep_for(std::chrono::seconds(seconds_to_sleep));

        //add job to job queue
        tLock.lock();
        for(int i = 0; i < name->size(); i++){
            queue.push_back((*name)[i]);
        }
        std::cout << queue.size() << std::endl;
        tLock.unlock();
    }

    //join our job queue thread before we end the program.
    jt.join();
    
    return 0;
}