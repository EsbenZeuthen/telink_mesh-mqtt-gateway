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
    # Extract the directory from the file path
    DIR=$(dirname "$FILE")

    # Create the directory if it doesn't exist
    mkdir -p "$DIR"

    # Construct the URL for the raw file
    URL="$BASE_URL/$FILE"
    
    # Download the file to the appropriate directory
    echo "Downloading $FILE to $DIR..."
    wget -q "$URL" -O "./$FILE"
done

echo "Download complete!"
