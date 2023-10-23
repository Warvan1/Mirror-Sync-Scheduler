#pragma once
#include <string>

struct Task{
    std::string name;
    int syncs;
};

struct Job{
    std::string name;
    double target_time;
};

class Schedule{
    public:
    Schedule(std::vector<Task> &tasks);

    bool verify(std::vector<Task> &tasks);

    void nextJob(std::string &name, int &seconds_to_sleep);

    private:
    int iterator;
    std::vector<Job> jobs;
};