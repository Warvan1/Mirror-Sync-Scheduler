// this library was adapted from this stackoverflow page
// https://stackoverflow.com/questions/70550983/http-request-using-sockets-on-c

#include <iostream>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>

#include "httpRequest.h"

//custom exception class
class HttpException : public std::exception {
    public:
    char * what () {
        return "HTTP Error";
    }
};

//make an http GET request
std::string http_request(std::string host, int port, std::string resource, std::string query){
    char buffer[4096];

    //initialize a socket
    int socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    //check if socket was made correctly
    if (socket_desc < 0) {
        std::cerr << "ERROR: failed to create socket" << std::endl;
        throw HttpException();
    }

    //resolve a hostname
    struct hostent *server = gethostbyname(host.c_str());
    //exit function if failed to resolve
    if (server == NULL){
        close(socket_desc);
        std::cerr << "ERROR: could not resolve hostname" << std::endl;
        throw HttpException();
    }

    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    if (connect(socket_desc, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        close(socket_desc);
        std::cerr << "ERROR: connection failed" << std::endl;
        throw HttpException();
    }

    //create an http get request
    std::string request = "GET " + resource + query + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";

    //send http get request
    if (send(socket_desc, request.c_str(), request.size(), 0) < 0){
        close(socket_desc);
        std::cerr << "ERROR: failed to send request" << std::endl;
        throw HttpException();
    }

    //recieve the response from the request
    int n;
    std::string raw_site;
    while ((n = recv(socket_desc, buffer, sizeof(buffer), 0)) > 0){
        raw_site.append(buffer, n);
    }

    close(socket_desc);

    return raw_site;
}

//remove the header from an http response
std::string remove_header(std::string response){
    int body_start = response.find("\r\n\r\n") + 4;
    return response.substr(body_start, response.size() - body_start);
}