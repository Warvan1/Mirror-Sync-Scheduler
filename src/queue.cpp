#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <list>
#include <algorithm>
#include <unordered_map>
#include <stdio.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <mirror/logger.h>

#include "queue.h"

Queue::Queue(): queueRunning(false), queueStoped(false){}

//create an instance of Queue the first time its ran on the heap
//every other time its ran it returns that same instance
Queue* Queue::getInstance(){
    //a static variable is not updated when getInstance is called a second time
    static Queue* queue = new Queue;
    return queue;
}

//used to check if the queue thread is running
bool Queue::getQueueRunning(){
    return queueRunning;
}

//used to stop the queue thread
void Queue::setQueueStoped(bool b){
    queueStoped = b;
}

//used to add a list of jobs to the queue
void Queue::push_back_list(std::vector<std::string>* name){
    tLock.lock();
    for(int i = 0; i < name->size(); i++){
        queue_.push_back((*name)[i]);
    }
    std::cout << queue_.size() << std::endl;
    tLock.unlock();
}

//start the job queue thread
void Queue::startQueue(json &config, std::size_t maxThreads){
    if(queueRunning == true){
        logger->warn("startQueue tried to start a second time");
        return;
    }
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
    queueRunning = true;
    queueStoped = false;
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
                    syncThreads.push_back(std::thread(&Queue::syncProject, this, jobName));
                    //add job to current jobs
                    currentJobs.push_back(jobName);
                }
            }

            //join all our threads before we continue
            for(std::size_t i = 0; i < syncThreads.size(); i++){
                syncThreads[i].join();
            }
        }

        if(queueStoped == true){
            queueRunning = false;
            return;
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
            syncProject(jobName);
        }

        if(queueStoped == true){
            queueRunning = false;
            return;
        }
        //sleep for 5 seconds so that we arnt running constantly and to prevent constant locking
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

//sync a project using the commands from the the syncCommands map
void Queue::syncProject(std::string name){
    std::cout << name << " started" << std::endl;
    logger->info(name + " started");

    //check if name in map for command iteration
    for(std::string command : syncCommands[name]){
        //if dry run
        command = "echo \"" + command + "\"";
        //run command
        system(command.c_str());
    }
            
    //if dry run
    //temporary sleep for testing when doing
    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::cout << name << " completed" << std::endl;
    logger->info(name + " completed");
}

void Queue::createSyncCommandMap(json &config){
    //make sure the vector of maps is clear
    syncCommands.clear();

    //loop through all mirror entries
    for (auto& x : config.items()){
        //generate sync commands for each entry and add it to our map
        syncCommands[x.key()] = generateSyncCommands(config[x.key()]);
    }
}

//compose an rsync command
std::string Queue::rsync(json &config, std::string &options){
    std::string command = "rsync " + options + " ";

    std::string user = config.value("user", "");
    std::string host = config.value("host", "");
    std::string src = config.value("src", "");
    std::string dest = config.value("dest", "");

    if(user != ""){
        command = command + user + "@" + host + "::" + src + " " + dest;
    }
    else{
        command = command + host + "::" + src + " " + dest;
    }
    return command;
}

//generate commands to sync a given project config
std::vector<std::string> Queue::generateSyncCommands(json &config){
    std::vector<std::string> output;
    //check if project has an rsync json object
    auto rsyncData = config.find("rsync");
    auto scriptData = config.find("script");
    if(rsyncData != config.end()){
        //run rsync and sync the project
        std::string options = config["rsync"].value("options", "");
        output.push_back(rsync(config["rsync"], options));

        //2 stage syncs happen sometimes
        options = config["rsync"].value("second", "");
        if(options != ""){
            output.push_back(rsync(config["rsync"], options));
        }

        //a few mirrors are 3 stage syncs
        options = config["rsync"].value("third", "");
        if(options != ""){
            output.push_back(rsync(config["rsync"], options));
        }

    }
    //project uses script sync
    else if(scriptData != config.end()){ 
        std::string command = config["script"].value("command", "");
        std::vector<std::string> arguments = config["script"].value("arguments", std::vector<std::string> {});
        for(std::string arg : arguments){
            command = command + " " + arg;
        }
        output.push_back(command);
    }
    return output;
}