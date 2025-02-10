# ---- Stage 1: Build BlueZ ----
    FROM debian:stable AS builder

    # Set environment variables
    ENV DEBIAN_FRONTEND=noninteractive
    
    # Install build dependencies
    RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        make \
        g++ \
        gcc \
        doxygen \
        git \
        autoconf \
        automake \
        libtool \
        pkg-config \
        wget \
        curl \
        ca-certificates \ 
        libbluetooth-dev \
        libdbus-1-dev \
        libudev-dev \
        libglibmm-2.4-dev \
        libssl-dev \
        libcups2-dev \
        libical-dev \
        libreadline-dev \
        python3-docutils \
        libjsoncpp-dev \
        systemd \
        udev \
        && apt-get clean \
        && rm -rf /var/lib/apt/lists/*
    
    COPY telink_notify_fix.patch .
    
    # Build BlueZ
    RUN git clone https://github.com/bluez/bluez.git \
        && cd bluez \
        && git checkout tags/5.79 \
        && git apply ../telink_notify_fix.patch \
        && ./bootstrap \
        && ./configure \
        && make \
        && make install
    
    # ---- Stage 2: Create a Minimal Production Image ----
    FROM debian:stable-slim
    
    # Set environment variables
    ENV DEBIAN_FRONTEND=noninteractive
    
    # Install only essential runtime dependencies
    RUN apt-get update && apt-get install -y --no-install-recommends \
        libbluetooth3 \
        libdbus-1-3 \
        libudev1 \
        libglib2.0-0 \
        libssl3 \
        libcups2 \
        libical3 \
        libreadline8 \
        systemd \
        udev \
        python3-docutils \
        libjsoncpp25 \
        && apt-get clean \
        && rm -rf /var/lib/apt/lists/*
    
    # Copy only necessary binaries from the builder stage
    COPY --from=builder /usr/local/libexec/bluetooth /usr/local/libexec/bluetooth
    COPY --from=builder /usr/local/bin /usr/local/bin
    COPY --from=builder /usr/local/lib /usr/local/lib
    COPY --from=builder /usr/local/include /usr/local/include
    COPY --from=builder /usr/local/etc /usr/local/etc

    
    # Define the entrypoint or default command
    CMD ["/usr/local/libexec/bluetooth/bluetoothd", "--nodetach"]
    