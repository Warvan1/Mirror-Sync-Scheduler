#pragma once

#include "structs.h"

void printJson(json object);

json readMirrors(std::string filename);

std::vector<Task> parseTasks(json &config);