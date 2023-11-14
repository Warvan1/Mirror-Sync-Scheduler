#include <iostream>
#include <fstream>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "mirrors.h"

//prints a json object
void printJson(json object){
    for (auto& x : object.items()){
        std::cout << "key: " << x.key() << ", value: " << x.value() << '\n';
    }
}

//read mirrors.json in from file
json readMirrors(std::string filename){
    //read in mirrors.json from file
    std::ifstream f(filename);
    json config = json::parse(f);
    f.close();
    return config;
}