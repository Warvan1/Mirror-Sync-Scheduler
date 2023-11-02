#pragma once

#include "structs.h"

class Schedule{
    public:
    Schedule();

    void build(json config);

    bool verify(json config);

    std::vector<std::string> * nextJob(int &seconds_to_sleep);

    private:
    int iterator;
    std::vector<Job> jobs;
};