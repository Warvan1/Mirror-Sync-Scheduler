#pragma once

#include <mutex>
#include <list>

class Queue{
    private:
    Queue();

    public:
    //delete copy and move constructors
    Queue(Queue&) = delete;
    Queue(Queue&&) = delete;

    static std::shared_ptr<Queue> getInstance();

    void push_back_list(std::vector<std::string> * name);

    void jobQueueThread(json &config, std::size_t maxThreads);

    void jobQueueThread_single(json &config);

    private:
    std::mutex tLock;
    std::list<std::string> queue_;
    //connection to log server
    std::shared_ptr<mirror::Logger> logger = mirror::Logger::getInstance();
};