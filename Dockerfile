FROM ubuntu:latest
WORKDIR /mirror
RUN apt update
RUN apt upgrade -y
RUN apt install -y libzmq3-dev
COPY build/syncScheduler /mirror/syncScheduler
ENTRYPOINT ["./syncScheduler"]
