# Build
FROM ubuntu:latest as builder
RUN apt update && apt upgrade -y
RUN apt install -y g++ cmake libzmq3-dev ninja-build git
WORKDIR /sync_scheduler
COPY ./src /sync_scheduler/src
COPY ./mirror-logging /sync_scheduler/mirror-logging
COPY CMakeLists.txt /sync_scheduler/CMakeLists.txt
RUN cmake -S/sync_scheduler -B/sync_scheduler/build -G Ninja
RUN cmake --build /sync_scheduler/build --target clean
RUN cmake --build /sync_scheduler/build --target all

# Run
FROM ubuntu:latest
RUN apt update && apt upgrade -y
RUN apt install -y libzmq3-dev rsync
WORKDIR /mirror/sync_scheduler
COPY --from=builder /sync_scheduler/build/syncScheduler /mirror/sync_scheduler/syncScheduler
ENTRYPOINT ["./syncScheduler"]
