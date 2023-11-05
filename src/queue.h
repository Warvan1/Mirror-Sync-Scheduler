#pragma once

#include <list>

class Queue{
    public: //functions
    //delete copy and move constructors
    Queue(Queue&) = delete;
    Queue(Queue&&) = delete;

    static Queue* getInstance();

    void push_back_list(std::vector<std::string>* name);

    void startQueue(json &config, std::size_t maxThreads);

    bool getQueueRunning();

    void setQueueStoped(bool b);

    private: //functions
    Queue();

    void jobQueueThread(json &config, std::size_t maxThreads);

    void jobQueueThread_single(json &config);

    private: //data
    std::mutex tLock;
    std::list<std::string> queue_;
    //used to prevent the queue from being started more than once
    bool queueRunning;
    //used to stop the queue thread
    bool queueStoped;
    //connection to log server
    mirror::Logger* logger = mirror::Logger::getInstance();
};