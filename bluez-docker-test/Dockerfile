# Use a base image with support for Bluetooth and development tools
FROM debian:bullseye-slim

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    libdbus-1-dev \
    libglib2.0-dev \
    libudev-dev \
    libbluetooth-dev \
    bluez \
    dbus \
    wget \
    git \
    procps \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /run/dbus
RUN chmod 755 /run/dbus


# Start dbus service (required by BlueZ)
#RUN service dbus start

# Create a directory for BlueZ if you plan to build from source
# RUN git clone https://github.com/bluez/bluez.git /bluez
# RUN cd /bluez && ./configure && make && make install

# Make sure the container can access the host Bluetooth device
#VOLUME ["/dev"]

# Set up entrypoint to keep container running for testing
#ENTRYPOINT ["sleep", "infinity"]
