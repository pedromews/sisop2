FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    tcl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY .. .

RUN rm -rf build && mkdir build && cd build && cmake .. && make

EXPOSE 4000/udp

CMD ["/bin/bash"]
