#pragma once

struct Task{
    std::string name;
    int syncs;
};

struct Job{
    std::vector<std::string> name;
    double target_time;
};