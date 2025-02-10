# ---------
# Stage 1: Development Environment (No Forced Build)
# ---------
    FROM debian:stable AS dev

    # Set environment variables
    ENV DEBIAN_FRONTEND=noninteractive
    
    # Install all development dependencies
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
    
    # Set working directory
    WORKDIR /workspace
    
    # Default shell entry (you manually run CMake when developing)
    CMD ["/bin/bash"]
    
    
    # ---------
    # Stage 2: Build Stage (Compiles for Production)
    # ---------
    FROM dev AS builder
    
    # Copy source code into the container
    COPY ./src ./src
    COPY ./CMakeLists.txt .
    
    # Create a build directory and compile the application
    RUN mkdir -p build && cd build && \
        cmake .. -DCMAKE_BUILD_TYPE=Release  && \
        make -j$(nproc)
    
    # Optionally strip debug symbols for a smaller binary
    RUN strip build/meshgateway
    
    
    # ---------
    # Stage 3: Production (Minimal Runtime)
    # ---------
    FROM debian:stable-slim AS production
    
    # Set environment variables
    ENV DEBIAN_FRONTEND=noninteractive
    
    # Install only runtime dependencies
    RUN apt-get update && apt-get install -y --no-install-recommends\
        #libbluetooth3\
       # libdbus-1-3 \
       # libudev1 \
        libglibmm-2.4 \        
        libpaho-mqttpp3-1 \
        libssl3 \
        libglib2.0 \
       # libcups2 \
       # libical3 \
       # libreadline8 \
        libjsoncpp25 \
        && apt-get clean \
        && rm -rf /var/lib/apt/lists/*  # Removes package lists to reduce image size

    
    # Set working directory
    WORKDIR /app
    
    # Copy only the compiled binary from the builder stage
    COPY --from=builder /workspace/build/meshgateway .
    
    # Ensure executable permissions
    RUN chmod +x meshgateway
    
    # Define the entrypoint
    CMD ["./meshgateway"]    