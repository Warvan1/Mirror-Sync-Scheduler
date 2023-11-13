#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <list>
#include <algorithm>
#include <unordered_map>
// #include <stdio.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <mirror/logger.h>

#include "queue.h"

Queue::Queue(): queueRunning(false){}

//create an instance of Queue the first time its ran on the heap
//every other time its ran it returns that same instance
Queue* Queue::getInstance(){
    //a static variable is not updated when getInstance is called a second time
    static Queue* queue = new Queue;
    return queue;
}

//set the dryrun variable
void Queue::setDryrun(bool dr){
    dryrun = dr;
}

//used to add a list of jobs to the queue
void Queue::push_back_list(std::vector<std::string>* name){
    tLock.lock();
    for(int i = 0; i < name->size(); i++){
        auto search = syncCommands.find((*name)[i]);
        if(search != syncCommands.end()){ //check to make sure that given string is in syncCommands
            queue_.push_back((*name)[i]);
        }
        else{
            logger->warn((*name)[i] + " is not valid");
        }
    }
    std::cout << queue_.size() << std::endl;
    tLock.unlock();
}

//used by manual sync to add a task to the front of the queue
void Queue::push_front_single(std::string &s){
    tLock.lock();
    auto search = syncCommands.find(s);
    if(search != syncCommands.end()){ //check to make sure that given string is in syncCommands
        queue_.push_front(s);
    }
    else{
        logger->warn(s + " is not valid");
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
    //create a pool of threads to run syncs in
    for(std::size_t i = 0; i < maxThreads; i++){
        std::thread t(&Queue::jobQueueThread, this, std::ref(config), maxThreads);
        t.detach();
    }
    queueRunning = true;
}

//checks the queue every 5 seconds and runs any added jobs 
//using threads to run up to "maxThreads" in parallel
void Queue::jobQueueThread(json &config, std::size_t maxThreads){
    while(true){
        //lock thread
        tLock.lock();

        //check if the queue is empty
        if(!queue_.empty()){
            //select and remove the first element from queue
            std::string jobName = queue_.front();
            queue_.pop_front();
            std::cout << queue_.size() << std::endl;

            //check to make sure that jobName is not in currentJobs already
            if(std::find(currentJobs.begin(), currentJobs.end(), jobName) == currentJobs.end()){
                //add job to current jobs
                currentJobs.push_back(jobName);

                //unlock before running the job
                tLock.unlock();
                //run the job within our threadpool
                syncProject(jobName);
                //lock after running the job
                tLock.lock();

                //erase the job from the currientJobs vector
                currentJobs.erase(std::find(currentJobs.begin(), currentJobs.end(), jobName));
            }
        }

        //unlock thread
        tLock.unlock();

        //sleep for 5 seconds so that we arnt running constantly and to prevent constant locking
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

//sync a project using the commands from the the syncCommands map
void Queue::syncProject(std::string name){
    std::cout << name << " started" << std::endl;
    logger->info(name + " started");

    //for each command in the vector of commands for the given name in the syncCommands map
    for(std::string command : syncCommands[name]){
        if(dryrun == true){
            command = "echo \"" + command + "\"";  
        }
        //check if password
        else if(passwordFiles.find(name) != passwordFiles.end()){
            //create string with environment variable
            std::string passwordStr = "RSYNC_PASSWORD="+passwordFiles[name];
            //convert std::string to char*
            char* password = const_cast<char*>(passwordStr.c_str());
            putenv(password);
        }
        //run command
        system(command.c_str());
    }
            
    //temporary sleep for testing when doing a dry run
    if(dryrun == true){
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    std::cout << name << " completed" << std::endl;
    logger->info(name + " completed");
}

//create a map that maps a task to the commands needed to sync it
void Queue::createSyncCommandMap(json &config){
    //make sure the syncCommand and passwordFiles maps are empty
    syncCommands.clear();
    passwordFiles.clear();

    //loop through all mirror entries
    for (auto& x : config.items()){
        //generate sync commands for each entry and add it to our map
        syncCommands[x.key()] = generateSyncCommands(config[x.key()], x.key());
    }
}

//generate commands to sync a given project config
std::vector<std::string> Queue::generateSyncCommands(json &config, std::string name){
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

        //handle passwords
        options = config["rsync"].value("password_file", "");
        if(options != ""){
            passwordFiles[name] = options;
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