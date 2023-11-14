#pragma once

struct Task{
    std::string name;
    int syncs;
};

struct Job{
    std::vector<std::string> name;
    double target_time;
};

class Schedule{
    private:
    Schedule();
    
    public:
    //delete copy and move constructors
    Schedule(Schedule&) = delete;
    Schedule(Schedule&&) = delete;
    Schedule &operator=(const Schedule &) = delete;
    Schedule &operator=(const Schedule &&) = delete;

    static Schedule* getInstance();

    void build(json config);

    std::vector<std::string>* nextJob(int &seconds_to_sleep);

    private: //functions
    bool verify(std::vector<Task> tasks);

    std::vector<Task> parseTasks(json &config);

    private: //data
    int iterator;
    std::vector<Job> jobs;
    //connection to log server
    mirror::Logger* logger = mirror::Logger::getInstance();
};