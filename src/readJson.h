#pragma once

json readJSONFromFile(std::string filename);

json readJSONFromHTTP(std::string host, int port, std::string resource, std::string query);