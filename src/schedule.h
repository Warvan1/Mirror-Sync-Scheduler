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
    public:
    Schedule(std::vector<Task> &tasks);

    bool verify(std::vector<Task> &tasks);

    std::vector<std::string> * nextJob(int &seconds_to_sleep);

    private:
    int iterator;
    std::vector<Job> jobs;
};