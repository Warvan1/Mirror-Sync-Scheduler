// this library was adapted from this stackoverflow page
// https://stackoverflow.com/questions/70550983/http-request-using-sockets-on-c

#pragma once

std::string http_request(std::string host, int port, std::string resource, std::string query);

std::string remove_header(std::string response);