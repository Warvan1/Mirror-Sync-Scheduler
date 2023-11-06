#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <stdio.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <mirror/logger.h>

#include "rsync.h"
#include "mirrors.h"

//run an rsync command
void rsync(json &config, std::string &options){
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

    //if dry run
    command = "echo \"" + command + "\"";

    //run command
    system(command.c_str());
}

//sync a project handling multistep rsync and scrypt sync
void syncProject(std::string name, json &config, mirror::Logger* logger){
    std::cout << name << " started" << std::endl;
    logger->info(name + " started");

    //check if project has an rsync json object
    auto rsyncData = config.find("rsync");
    if(rsyncData != config.end()){
        //run rsync and sync the project
        std::string options = config["rsync"].value("options", "");
        rsync(config["rsync"], options);

        //2 stage syncs happen sometimes
        options = config["rsync"].value("second", "");
        if(options != ""){
            rsync(config["rsync"], options);
        }

        //a few mirrors are 3 stage syncs
        options = config["rsync"].value("third", "");
        if(options != ""){
            rsync(config["rsync"], options);
        }

    }
    else { //project uses script sync
        std::string command = config["script"].value("command", "");
        std::vector<std::string> arguments = config["script"].value("arguments", std::vector<std::string> {});
        for(std::string arg : arguments){
            command = command + " " + arg;
        }

        //if dry run
        command = "echo \"" + command + "\"";
        
        //run command
        system(command.c_str());
    }
    

    //temporary sleep for testing when doing a dry run
    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::cout << name << " completed" << std::endl;
    logger->info(name + " completed");
}