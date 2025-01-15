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
    dbus \
    libbluetooth-dev \
    libdbus-1-dev \
    libudev-dev \
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
    systemd \ 
    python3-docutils \
    libjsoncpp-dev \
    && apt-get clean


RUN mkdir -p /var/run/dbus
RUN chmod 755 /var/run/dbus

# Install BlueZ (Bluetooth stack)
#RUN apt-get update && apt-get install -y \
#    bluez \
#    && apt-get clean

# Optional: Install Doxygen for documentation (if needed)
RUN apt-get install -y doxygen

# Set up the working directory
WORKDIR /app

# Copy source code (if any)
COPY . /app

# Expose any ports you need (e.g., MQTT, BLE)
# EXPOSE 1883 (for MQTT)

# Build your project (if applicable)
# RUN cmake . && make

# Default command
CMD ["bash"]
