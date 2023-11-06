#include <iostream>
#include <vector>
#include <string>

#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include <mirror/logger.h>

#include "rsync.h"
#include "mirrors.h"

//run an rsync command
std::string rsync(json &config, std::string &options){
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

std::vector<std::string> generateSyncCommands(json &config){
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
    else if(scriptData != config.end()){ //project uses script sync
        std::string command = config["script"].value("command", "");
        std::vector<std::string> arguments = config["script"].value("arguments", std::vector<std::string> {});
        for(std::string arg : arguments){
            command = command + " " + arg;
        }
        output.push_back(command);
    }
    return output;
}