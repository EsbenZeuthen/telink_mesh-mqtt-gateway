# Use an official Ubuntu base image
FROM ubuntu:22.04

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    libgtest-dev \
    cmake \
    make \
    g++ \
    gcc \
    gdb \
    valgrind \
    libbluetooth-dev \
    libdbus-1-dev \
    libglibmm-2.4-dev \
    libpaho-mqtt-dev \
    libpaho-mqttpp-dev \
    pkg-config \
    libssl-dev \
    libglib2.0-dev \
    wget \
    curl \
    git \
    autoconf \
    automake \
    libtool \
    libcups2-dev \
    libical-dev \
    libreadline-dev \   
    libjsoncpp-dev \
    && apt-get clean


# Default command
CMD ["bash"]
