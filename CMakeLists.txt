cmake_minimum_required(VERSION 3.22)
project(syncScheduler)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/schedule.cpp
    src/readJson.cpp
    src/queue.cpp
    src/httpRequest.cpp
)

include(FetchContent)

FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

add_subdirectory(mirror-logging)

target_link_libraries(${PROJECT_NAME} 
    PUBLIC mirror-logging
    PRIVATE nlohmann_json::nlohmann_json
)