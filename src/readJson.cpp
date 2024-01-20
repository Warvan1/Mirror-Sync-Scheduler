#include <iostream>
#include <fstream>

#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include "httpRequest.h"

#include "readJson.h"

//read json in from a file
json readJSONFromFile(std::string filename){
    std::ifstream f(filename);
    json config = json::parse(f);
    f.close();
    return config;
}

//read json in from a http request
json readJSONFromHTTP(std::string host, int port, std::string resource, std::string query){
    std::string response = http_request(host, port, resource, query);
    return json::parse(remove_header(response));
}