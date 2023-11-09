#pragma once

#include <list>

class Queue{
    public: //functions
    //delete copy and move constructors
    Queue(Queue&) = delete;
    Queue(Queue&&) = delete;
    Queue &operator=(const Queue &) = delete;
    Queue &operator=(const Queue &&) = delete;

    static Queue* getInstance();

    void push_back_list(std::vector<std::string>* name);

    void push_front_single(std::string &s);

    void startQueue(json &config, std::size_t maxThreads);

    void createSyncCommandMap(json &config);

    bool getQueueRunning();

    void setQueueStoped(bool b);

    void setDryrun(bool dr);

    private: //functions
    Queue();

    void jobQueueThread(json &config, std::size_t maxThreads);

    void jobQueueThread_single(json &config);

    void syncProject(std::string name);

    std::string rsync(json &config, std::string &options);

    std::vector<std::string> generateSyncCommands(json &config);

    private: //data
    std::mutex tLock;
    std::list<std::string> queue_;
    //map of syncCommands
    std::unordered_map<std::string, std::vector<std::string>> syncCommands;
    //used to prevent the queue from being started more than once
    bool queueRunning;
    //used to stop the queue thread
    bool queueStoped;
    //used to run program as a "dry run"
    bool dryrun;
    //connection to log server
    mirror::Logger* logger = mirror::Logger::getInstance();
};