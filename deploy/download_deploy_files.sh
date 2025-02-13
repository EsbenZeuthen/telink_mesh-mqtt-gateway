#!/bin/bash

# Define the base URL for the GitHub repository (raw files)
BASE_URL="https://raw.githubusercontent.com/EsbenZeuthen/telink_mesh-mqtt-gateway/main/deploy"

# List of files to download
FILES=(
    "mosquitto/mosquitto.conf"
    "docker-compose.yml"
)

# Download each file from the repository
for FILE in "${FILES[@]}"; do
    URL="$BASE_URL/$FILE"
    
    # Download the file to the current directory
    echo "Downloading $FILE..."
    wget -q "$URL" -O "./$(basename "$FILE")"
done

echo "Download complete!"
