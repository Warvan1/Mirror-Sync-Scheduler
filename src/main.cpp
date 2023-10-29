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

#include "schedule.h"
#include "rsync.h"
#include "mirrors.h"

//mutex lock for threads
std::mutex tLock;

// runs in a seperate thread
// if there is a job in the queue it gets ran and then removed from the front of the queue in a loop
void jobQueueThread(std::list<std::string> &queue, json &config, std::size_t maxThreads){
    while(true){
        tLock.lock();
        bool queueEmpty = queue.empty();
        tLock.unlock();

        if(!queueEmpty){
            //create thread pool
            std::vector<std::thread> syncThreads;
            //create currientJobs vector to keep track of what jobs we are curriently syncing so that we dont do the same one at the same time.
            std::vector<std::string> currientJobs;
            //calculate the currient number of needed threads
            tLock.lock();
            std::size_t numThreads = std::min(maxThreads, queue.size());
            tLock.unlock();

            //run up to "maxThreads" jobs in the queue
            for(std::size_t i = 0; i < numThreads; i++){
                //retrieve and then remove first item from queue
                tLock.lock();
                std::string jobName = queue.front();
                queue.pop_front();
                std::cout << queue.size() << std::endl;
                tLock.unlock();

                //check to make sure that jobName is not in currientJobs already
                bool abortSync = false;
                for(int i = 0; i < currientJobs.size(); i++){
                    if(currientJobs[i] == jobName){
                        abortSync = true;
                        break;
                    }
                }

                if(abortSync == false){
                    currientJobs.push_back(jobName);
                    syncThreads.push_back(std::thread(syncProject, jobName, std::ref(config[jobName])));
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

    //jobqueue (locked with tLock)
    std::list<std::string> queue;

    //jobqueueThread runs all the jobs that get entered into the queue
    std::thread jt(jobQueueThread, std::ref(queue), std::ref(config), 4);

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