#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <list>
#include <algorithm>

#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <mirror/logger.h>

#include "queue.h"
#include "rsync.h"

Queue::Queue(){}

//create an instance of Queue the first time its ran on the heap
//every other time its ran it returns that same instance
Queue& Queue::getInstance(){
    //a static variable is not updated when getInstance is called a second time
    static Queue queue;
    return queue;
}

//used to add a list of jobs to the queue
void Queue::push_back_list(std::vector<std::string> * name){
    tLock.lock();
    for(int i = 0; i < name->size(); i++){
        queue_.push_back((*name)[i]);
    }
    std::cout << queue_.size() << std::endl;
    tLock.unlock();
}

//start the job queue thread
void Queue::startQueue(json &config, std::size_t maxThreads){
    //start single threaded queue on detatched thread
    if(maxThreads == 1){
        std::thread t(&Queue::jobQueueThread_single, this, std::ref(config));
        t.detach();
    }
    //start multithreaded queue on detatched thread
    else{
        std::thread t(&Queue::jobQueueThread, this, std::ref(config), maxThreads);
        t.detach();
    }
}

//checks the queue every 5 seconds and runs any added jobs 
//using threads to run up to "maxThreads" in parallel
void Queue::jobQueueThread(json &config, std::size_t maxThreads){
    while(true){
        tLock.lock();
        bool queueEmpty = queue_.empty();
        tLock.unlock();

        if(!queueEmpty){
            //create thread pool
            std::vector<std::thread> syncThreads;
            //create currentJobs vector to keep track of what jobs we are currently syncing so that we dont do the same one at the same time.
            std::vector<std::string> currentJobs;
            //calculate the current number of needed threads
            tLock.lock();
            std::size_t numThreads = std::min(maxThreads, queue_.size());
            tLock.unlock();

            //run up to "maxThreads" jobs in the queue
            for(std::size_t i = 0; i < numThreads; i++){
                //retrieve and then remove first item from queue
                tLock.lock();
                std::string jobName = queue_.front();
                queue_.pop_front();
                std::cout << queue_.size() << std::endl;
                tLock.unlock();

                //check to make sure that jobName is not in currentJobs already
                if(std::find(currentJobs.begin(), currentJobs.end(), jobName) == currentJobs.end()){
                    //run the job within our threadpool
                    syncThreads.push_back(std::thread(syncProject, jobName, std::ref(config[jobName]), std::ref(logger)));
                    //add job to current jobs
                    currentJobs.push_back(jobName);
                }
            }

            //join all our threads before we continue
            for(std::size_t i = 0; i < syncThreads.size(); i++){
                syncThreads[i].join();
            }
        }

        //sleep for 5 seconds so that we arnt running constantly and to prevent constant locking
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

//checks the queue every 5 seconds and runs any added jobs
//runs jobs in sequence
void Queue::jobQueueThread_single(json &config){
    while(true){
        tLock.lock();
        bool queueEmpty = queue_.empty();
        tLock.unlock();

        if(!queueEmpty){
            //retrieve and then remove first item from queue
            tLock.lock();
            std::string jobName = queue_.front();
            queue_.pop_front();
            std::cout << queue_.size() << std::endl;
            tLock.unlock();

            //run the first job in the queue
            syncProject(jobName, config[jobName], logger);
        }

        //sleep for 5 seconds so that we arnt running constantly and to prevent constant locking
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}