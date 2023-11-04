#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <ctime>

#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <mirror/logger.h>

#include "schedule.h"
#include "mirrors.h"

//build a schedule of jobs spaced evenly throuout the day
Schedule::Schedule(): iterator(0){}

//create an instance of Schedule the first time its ran on the heap
//every other time its ran it returns that same instance
Schedule* Schedule::getInstance(){
    //a static variable is not updated when getInstance is called a second time
    static Schedule schedule;
    return &schedule;
}

void Schedule::build(json config){
    //create Task vector from mirrors.json
    std::vector<Task> tasks = parseTasks(config);

    //calculate the total number of jobs
    int total_jobs = 0;
    for(int i = 0; i < tasks.size(); i++){
        total_jobs += tasks[i].syncs;
    }
    
    //compute the least common multiple of all sync frequencies
    int lcm = 1;
    for(int i = 0; i < tasks.size(); i++){
        int syncs = tasks[i].syncs;
        int a;
        int b;
        int rem;
        if(lcm > syncs){
            a = lcm;
            b = syncs;
        }
        else {
            a = syncs;
            b = lcm;
        }
        while(b != 0){
            rem = a % b;
            a = b;
            b = rem;
        }
        //a is now the greatest common denominator;
        lcm = lcm * syncs / a;
    }

    double interval = 1.0/lcm;
    for(int i = 0; i < lcm; i++){
        // std::cout << "---------" << i << "-------------" << std::endl;
        Job job;
        for(int j = 0; j < tasks.size(); j++){
            Task task = tasks[j];
            if(i%(lcm/task.syncs) == 0){
                // std::cout << tasks[j].syncs << " " << tasks[j].name << std::endl;
                job.name.push_back(task.name);
            }
        }
        job.target_time = interval * i;
        jobs.push_back(job); //jobs is a vector<Job> created in the class
    }

    //verify that the schedule works
    bool success = verify(config);
}

//verify that the job schedule schedules each job the correct number of times.
//verify that the job.start_time increases for each job and 0 <= start_time <= 1
bool Schedule::verify(json config){
    //create Task vector from mirrors.json
    std::vector<Task> tasks = parseTasks(config);
    //create a map of tasks
    std::map<std::string, int> taskMap;
    for(int i = 0; i < tasks.size(); i++){
        taskMap[tasks[i].name] = 0;
    }
    
    double prev_start_time = 0.0;
    for(int i = 0; i < jobs.size(); i++){
        //check that start_time increases and that 0.0 <= start_time <= 1.0
        if(!(prev_start_time <= jobs[i].target_time <= 1.0)){
            logger->error("failed to create or verify schedule.");
            return false;
        }
        prev_start_time = jobs[i].target_time;

        //increment the appropriate task in our map for each name in our job name vector
        for(int j = 0; j < jobs[i].name.size(); j++){
            taskMap.find(jobs[i].name[j])->second++;
        }
    }

    //check that each job is scheduled the correct number of times
    for(int i = 0; i < tasks.size(); i++){
        if(tasks[i].syncs != taskMap.find(tasks[i].name)->second){
            logger->error("failed to create or verify schedule.");
            return false;
        }
        // std::cout << taskMap.find(tasks[i].name)->first << " " <<  taskMap.find(tasks[i].name)->second << " " << tasks[i].syncs << std::endl;
    }
    
    logger->info("created and verified schedule.");
    return true;
}

std::vector<std::string>* Schedule::nextJob(int &seconds_to_sleep){
    double total_seconds_day = 86400.0;

    //calculate seconds_since_midnight
    std::time_t now = std::time(0);
    std::tm* tm_gmt = std::gmtime(&now);
    int seconds_since_midnight_gmt = tm_gmt->tm_sec + (tm_gmt->tm_min*60) +  (tm_gmt->tm_hour*3600);

    //convert seconds_since_midnight to position in the schedule (0.0 <= scheduleTime <= 1.0)
    double scheduleTime = (double)seconds_since_midnight_gmt / total_seconds_day;

    //find the first job that is greater than the currient time.
    //we start at iterator so that we dont have to search the entire job array each time.
    //we do it in this way so that if we start the program in the middle of the day it will still select the correct job.
    //in normal operation it will just go through the loop once increasing the iteratior to the next job.
    while(iterator < jobs.size() && jobs[iterator].target_time <= scheduleTime){
        iterator++;
    }

    //if we are at the end of the schedule
    if(iterator == jobs.size()){
        //reset the iterator
        iterator = 0;

        //sleep till midnight
        //total seconds in a day - currient time in seconds cast to an interger and added 1
        seconds_to_sleep = (int)(total_seconds_day - (scheduleTime * total_seconds_day)) + 1;
        return &(jobs[jobs.size()-1].name);
    }

    //sleep till next job
    //target time in seconds - currient time in seconds cast to an interger and added 1
    seconds_to_sleep = (int)((jobs[iterator].target_time * total_seconds_day) - (scheduleTime * total_seconds_day)) + 1;
    return &(jobs[iterator-1].name);
}