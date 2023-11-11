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

    void setDryrun(bool dr);

    private: //functions
    Queue();

    void jobQueueThread(json &config, std::size_t maxThreads);

    void syncProject(std::string name);

    std::vector<std::string> generateSyncCommands(json &config);

    std::string rsync(json &config, std::string &options);

    private: //data
    //thread lock to prevent access to data from multiple threads at the same time
    std::mutex tLock;
    //list of queued jobes
    std::list<std::string> queue_;
    //create currentJobs vector to keep track of what jobs we are currently syncing so that we dont do the same one at the same time.
    std::list<std::string> currentJobs;
    //map of syncCommands
    std::unordered_map<std::string, std::vector<std::string>> syncCommands;
    //used to prevent the queue from being started more than once
    bool queueRunning;
    //used to run program as a "dry run"
    bool dryrun;
    //connection to log server
    mirror::Logger* logger = mirror::Logger::getInstance();
};