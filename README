# Telink Mesh MQTT Gateway

This solution creates an MQTT gateway service to communicate with Telink Mesh lights, such as older Briloner panel LED lights.

The solution consists of four Docker images:
1. **MQTT Telink Gateway**: Bridges communication between Telink Mesh lights and the MQTT broker.
2. **Modified BlueZ Stack**: Custom BlueZ stack, as the Telink CCCD handle does not conform to the BLE specification.
3. **Mosquitto MQTT Broker**: Handles MQTT communication between the gateway and Home Assistant.
4. **Home Assistant**: The home automation platform used to control your lights.

You can use alternatives to Mosquitto and Home Assistant, but you will be on your own.

## Prerequisites

- **Telink Mesh Lights**: One or more lights with a Telink Mesh MCU (tested with Briloner Piatto lights). To reset the lights, perform three quick (on for ~1s) and two slow (on for ~5s) power cycles. When successful, the lights will blink three times. The lights will then be reported as `telink_mesh1` during a Bluetooth scan.
- **Compatible Bluetooth Adapter**: Tested with Raspberry Pi 3's onboard Bluetooth adapter and StarTech USBBT1EDR4. Other adapters may also work, but I had no luck with Realtek based adapters. Proper support for BLE is important.
- **Host Machine**: A Linux distribution (tested with Ubuntu and DietPi) with Docker installed. You can, e.g., use a Raspberry Pi.
- **Docker**: You host must have Docker installed.
- **BlueZ**: Your host must have BlueZ installed.
- **Secure Network**: Ensure that the network is secure, as the endpoints are not hardened, and we use the host network (for bluetooth adapter access from docker containers).

## Deploying

The `deploy` folder contains Docker Compose and configuration files to get you started. You can download the relevant files with:

```bash
curl -sSL https://github.com/EsbenZeuthen/telink_mesh-mqtt-gateway/raw/main/deploy/download_deploy_files.sh | bash
```

### 1. Disable Host BlueZ Service

Stop and disable your current BlueZ service to prevent conflicts with the modified BlueZ stack running in the Docker container. On Debian/Ubuntu:

```bash
sudo systemctl stop bluetooth
sudo systemctl disable bluetooth
```

### 2. Modify `docker-compose.yml`

Edit the `docker-compose.yml` file as needed (for example, if you already have your own instances of Home Assistant and the MQTT broker).

### 3. Start Docker Containers

To start the Docker containers, run the following:

```bash
docker-compose up
```

### 4. Check Lights Discovery

You can check if your lights are being discovered by attaching to the `telink_mqtt_gateway` container:

```bash
docker exec -it telink_mqtt_gateway bash
```

Once the gateway connects to one of the lights, the lights should form a mesh automatically. The gateway will regularly send discovery messages to the Bluetooth mesh to identify all lights within range.

### 5. Home Assistant Setup
If you use the Home Assistant docker image referenced from the `docker-compose.yml`file, Home Assistant is available on port 8123 of your host, using your favorite web browser.

To have your lights show up in Home Assistant:
1. **Initial Setup**: Register an admin user in Home Assistant and complete the basic setup.
2. **Add MQTT Broker Integration**: Go to Home Assistant's integrations page and add the MQTT broker integration using the broker's IP address and port.
3. After a short while, your lights should appear in the Home Assistant overview, and you can start controlling them.

## Limitations and Known Issues

- **Telink Mesh Protocol**: The Telink Mesh protocol is proprietary. The current implementation only supports commands already reverse engineered in the [telinkpp](https://github.com/vpaeder/telinkpp) project.
- **Provisioning**: The provisioning command is not known. You will be stuck with the default `telink_mesh1` name and password. You may be able to provision using another app (e.g., a vendor's mobile app) and then update the credentials in the code.
- **Light Groups**: Telink Mesh supports light groups, but this feature has not been integrated into the MQTT gateway yet.
- **RGB/Color Temperature**: Setting RGB color or color temperature results in maximum brightness due to the stateless design of the gateway and MQTT behavior.
