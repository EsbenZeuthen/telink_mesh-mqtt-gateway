#ifndef MAPPINGS_H
#define MAPPINGS_H

#include <mqtt/message.h>
#include <json/json.h> // Assuming you use the JsonCpp library for JSON
#include <iomanip>
#include "../ble_stack/telink_mesh_protocol.h"

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "Mappings"

mqtt::message::ptr_t telink_to_mqtt(std::shared_ptr<TelinkMeshProtocol::TelinkMeshAddressReport> msg)
{          
    auto mac_address = msg->getMAC();
    // Convert MAC address to string format
    std::ostringstream mac_ss;
    for (size_t i = 0; i < mac_address.size(); ++i) {
        if (i > 0) mac_ss << ":";
        mac_ss << std::hex << std::setw(2) << std::setfill('0') << (int)mac_address[i];
    }
    std::string mac_address_str = mac_ss.str();
    
    // Construct the topic string
    std::string topic = "homeassistant/light/" + std::to_string(msg->getNodeID()) + "/config";

    // Create the JSON payload
    Json::Value payload;
    payload["name"] = "Light " + std::to_string(msg->getNodeID());
    payload["unique_id"] = "mesh_light_" + std::to_string(msg->getNodeID());
    payload["state_topic"] = "homeassistant/light/" + std::to_string(msg->getNodeID()) + "/state";
    payload["command_topic"] = "homeassistant/light/" + std::to_string(msg->getNodeID()) + "/set";
    payload["availability_topic"] = "homeassistant/light/" + std::to_string(msg->getNodeID()) + "/available";
    payload["payload_available"] = "true";
    payload["payload_not_available"] = "false";
    payload["schema"] = "json";
    payload["brightness"] = true;
    payload["brightness_scale"] = 100;
   // payload["rgb"] = true;
   Json::Value color_modes(Json::arrayValue); // Create an array value
    color_modes.append("rgbw");
    color_modes.append("color_temp");
    payload["supported_color_modes"] = color_modes;    
    payload["white_value"] = true;
    //payload["color_temp"] = true;
    //payload["max_mireds"] = 370.37;
    //payload["min_mireds"] = 153.85;
    payload["platform"] = "mqtt";

    // Convert JSON payload to string
    Json::StreamWriterBuilder writer;
    std::string payload_str = Json::writeString(writer, payload);

    

    return mqtt::message::create(topic,payload_str);
}

/*
def publish_light_availability(mqtt_client,mesh_id, available=True):
    """
    Publish the availability status of a light on the mesh to MQTT.
    
    :param mesh_id: The mesh ID of the light.
    :param available: Boolean value indicating whether the light is available.
    """
    # Define the MQTT topic for light availability
    topic = f"homeassistant/light/{mesh_id}/available"

    # Define the payload (either "online" or "offline")
    payload = "true" if available else "false"

    # Publish the availability status to MQTT
    mqtt_client.publish(topic, payload=payload, retain=True)

    # Debugging output
    print(f"Published availability to MQTT: Topic: {topic}, Payload: {payload}")

*/

mqtt::message::ptr_t telink_to_mqtt_availability(std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket> msg)
{
    uint16_t node_id;
             
    switch (msg->getCommand())
    {
        case TelinkMeshProtocol::Command::COMMAND_ADDRESS_REPORT:
            node_id = std::dynamic_pointer_cast<TelinkMeshProtocol::TelinkMeshAddressReport>(msg)->getNodeID();
            break;
        case TelinkMeshProtocol::Command::COMMAND_ONLINE_STATUS_REPORT:
            node_id = std::dynamic_pointer_cast<TelinkMeshProtocol::TelinkMeshOnlineStatusReport>(msg)->getNodeID();
            break;
        default:
            node_id=0xFFFF;
    }
    
    // Build the topic string
    std::string topic = "homeassistant/light/" + std::to_string(node_id) + "/available";
    
    std::string payload_str = "true";

    return mqtt::message::create(topic,payload_str);
}

mqtt::message::ptr_t telink_to_mqtt(std::shared_ptr<TelinkMeshProtocol::TelinkMeshOnlineStatusReport> msg)
{        
    // Build the topic string
    std::string topic = "homeassistant/light/" + std::to_string(msg->getNodeID()) + "/state";

    // Create the JSON payload
    Json::Value payload;
    payload["mesh_id"] = msg->getNodeID();
    payload["brightness"] = msg->getBrightness();
    payload["state"] = (msg->isLightOn() ? "ON" : "OFF");

    // Convert JSON payload to string
    Json::StreamWriterBuilder writer;
    std::string payload_str = Json::writeString(writer, payload);

    return mqtt::message::create(topic,payload_str);
}

mqtt::message::ptr_t telink_to_mqtt(std::shared_ptr<TelinkMeshProtocol::TelinkLightStatusReport> msg)
{    
    const std::string topic = "homeassistant/light/" + std::to_string(msg->getSrcNode()) + "/status";


    // Construct the JSON payload
    Json::Value payloadJson;
    payloadJson["mesh_id"] = msg->getSrcNode();
    payloadJson["brightness"] = msg->get_brightness();
    Json::Value rgbArray(Json::arrayValue);
    rgbArray.append(msg->get_red());
    rgbArray.append(msg->get_green());
    rgbArray.append(msg->get_blue());
    payloadJson["rgb"] = rgbArray;
    payloadJson["white"] = msg->get_white();

    // Convert JSON to string
    Json::StreamWriterBuilder writer;
    std::string payload_str = Json::writeString(writer, payloadJson);

    return mqtt::message::create(topic,payload_str);
}

mqtt::message::ptr_t telink_to_mqtt(std::shared_ptr<TelinkMeshProtocol::TelinkMeshGroupIDReport> msg)
{    
    const std::string topic = "homeassistant/light/" + std::to_string(msg->getSrcNode()) + "/status";


    // Construct the JSON payload
    Json::Value payloadJson;
  
  // TODO :: build the payload

    // Convert JSON to string
    Json::StreamWriterBuilder writer;
    std::string payload_str = Json::writeString(writer, payloadJson);

    return mqtt::message::create(topic,payload_str);
}

mqtt::message::ptr_t telink_to_mqtt(std::shared_ptr<TelinkMeshProtocol::TelinkMeshDeviceInfoReport> msg)
{    
    const std::string topic = "homeassistant/light/" + std::to_string(msg->getSrcNode()) + "/status";


    // Construct the JSON payload
    Json::Value payloadJson;
   


    // Convert JSON to string
    Json::StreamWriterBuilder writer;
    std::string payload_str = Json::writeString(writer, payloadJson);

    return mqtt::message::create(topic,payload_str);
}

mqtt::message::ptr_t telink_to_mqtt(std::shared_ptr<TelinkMeshProtocol::TelinkMeshTimeReport> msg)
{    
    const std::string topic = "homeassistant/light/" + std::to_string(msg->getSrcNode()) + "/status";


    // Construct the JSON payload
    Json::Value payloadJson;
  
    // Convert JSON to string
    Json::StreamWriterBuilder writer;
    std::string payload = Json::writeString(writer, payloadJson);

    return mqtt::message::create(topic,payload);
}

mqtt::message::ptr_t telink_to_mqtt(std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket> msg)
{       
        switch (msg->getCommand())
        {
            case TelinkMeshProtocol::Command::COMMAND_ADDRESS_REPORT:
                return telink_to_mqtt(std::dynamic_pointer_cast<TelinkMeshProtocol::TelinkMeshAddressReport>(msg));
                break;
            case TelinkMeshProtocol::Command::COMMAND_ONLINE_STATUS_REPORT:
                return telink_to_mqtt(std::dynamic_pointer_cast<TelinkMeshProtocol::TelinkMeshOnlineStatusReport>(msg));
                break;
            case TelinkMeshProtocol::Command::COMMAND_STATUS_REPORT:
                return telink_to_mqtt(std::dynamic_pointer_cast<TelinkMeshProtocol::TelinkLightStatusReport>(msg));
                break;
            case TelinkMeshProtocol::Command::COMMAND_GROUP_ID_REPORT:
                //return telink_to_mqtt(std::dynamic_pointer_cast<TelinkMeshProtocol::TelinkMeshGroupIDReport>(msg));
                break;
            case TelinkMeshProtocol::Command::COMMAND_DEVICE_INFO_REPORT:
                //return telink_to_mqtt(std::dynamic_pointer_cast<TelinkMeshProtocol::TelinkMeshDeviceInfoReport>(msg));
                break;
            case TelinkMeshProtocol::Command::COMMAND_TIME_REPORT:
                //return telink_to_mqtt(std::dynamic_pointer_cast<TelinkMeshProtocol::TelinkMeshTimeReport>(msg));
                break;
            default:
                throw std::runtime_error("Unsupported command type");
        }
        throw std::runtime_error("Not implemented yet");
}

std::vector<std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket>> mqtt_to_telink(mqtt::const_message_ptr msg)
{

    std::vector<std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket>> packets = {};
    // Parse the topic to match "homeassistant/light/+/set" and "homeassistant/light/+/state"
    std::string topic = msg->get_topic();    

    try {
        std::vector<std::string> topic_parts;
        std::istringstream topic_stream(topic);
        std::string part;

        while (std::getline(topic_stream, part, '/')) {
            topic_parts.push_back(part);
        }

        if (topic_parts.size() != 4 || topic_parts[0] != "homeassistant" || topic_parts[1] != "light") {
            g_warning("Invalid topic format, ignoring message."); 
        }

        // Extract the mesh node ID from the topic
        uint16_t node_id = std::stoi(topic_parts[2]); // The mesh ID is the third part of the topic
        
                
        Json::Value payload;
        Json::CharReaderBuilder reader;
        std::string errs;

        // Create a std::istringstream from the payload_str
        std::istringstream payload_str(msg->get_payload());

        if (!Json::parseFromStream(reader, payload_str, &payload, &errs)) {
            g_warning("Error decoding JSON payload: %s. Ignoring message.",errs.c_str());
        }

    // Determine purpose and map to telink packet

    // Process the "set" topic
        if (topic_parts[3] == "set") {
            if (payload.isMember("state")) {
                std::string state = payload["state"].asString();
                std::transform(state.begin(), state.end(), state.begin(), ::toupper);
                
                auto tmsg = std::make_shared<TelinkMeshProtocol::TelinkLightOnOff>();
                
                tmsg->set_on_off(state == "ON");
                tmsg->setDestNode(node_id);
                packets.push_back(std::dynamic_pointer_cast<TelinkMeshProtocol::TelinkMeshPacket>(tmsg));
            }

            if (payload.isMember("brightness")) {
                uint8_t brightness = payload["brightness"].asInt();                            
                auto tmsg = std::make_shared<TelinkMeshProtocol::TelinkLightSetAttributes>();
                tmsg->setDestNode(node_id);
                tmsg->set_brightness(brightness);
                packets.push_back(std::dynamic_pointer_cast<TelinkMeshProtocol::TelinkMeshPacket>(tmsg));
            }

            if (payload.isMember("color")) {
                const Json::Value& color = payload["color"];
                if (color.isMember("r") && color.isMember("g") && color.isMember("b")) {
                    int r = color["r"].asInt();
                    int g = color["g"].asInt();
                    int b = color["b"].asInt();
                    auto tmsg = std::make_shared<TelinkMeshProtocol::TelinkLightSetAttributes>();
                    tmsg->setDestNode(node_id);
                    tmsg->set_red(r);
                    tmsg->set_green(g);
                    tmsg->set_blue(b);
                    tmsg->set_brightness(100);
                    packets.push_back(std::dynamic_pointer_cast<TelinkMeshProtocol::TelinkMeshPacket>(tmsg));
                }
            }

            if (payload.isMember("color_temp")) {
                int t_mired = payload["color_temp"].asInt();

                auto tmsg = std::make_shared<TelinkMeshProtocol::TelinkLightSetAttributes>();
                uint8_t W = 0xff, Y = 0xff;
                

                int tK = 1e6/t_mired;
                
                tK = std::max(std::min(6500, tK), 2700);
                if (tK > 4600) {
                    Y = static_cast<unsigned char>((((float) (6500 - tK)) * 255.0f) / 1900.0f);
                } else {
                    W = static_cast<unsigned char>((((float) (tK - 2700)) * 255.0f) / 1900.0f);
                }
                tmsg->setDestNode(node_id);
                tmsg->set_red(0);
                tmsg->set_green(0);
                tmsg->set_blue(0);
                tmsg->set_brightness(100);  
                tmsg->set_yellow(Y);  
                tmsg->set_white(W);
                packets.push_back(std::dynamic_pointer_cast<TelinkMeshProtocol::TelinkMeshPacket>(tmsg));                
            }
        }
        // Process the "state" topic
       /* else if (topic_parts[3] == "state") {
            std::cout << "State request for mesh ID " << mesh_id << ": " << payload_str << std::endl;
            // Handle state logic here
        } else {
            std::cerr << "Unrecognized topic ending: " << topic_parts[3] << ", ignoring message." << std::endl;
        }*/
    } catch (const std::exception& e) {
        std::cerr << "Error processing MQTT message: " << e.what() << std::endl;
    }

    return packets;
}

std::shared_ptr<TelinkMeshProtocol::TelinkMeshAddressEdit> prepareAddressQuery()
{
    auto query = std::make_shared<TelinkMeshProtocol::TelinkMeshAddressEdit>();
    query->setDestNode(0xFFFF);            
    query->setMode(0xFFFF);
    return query;
}

std::shared_ptr<TelinkMeshProtocol::TelinkMeshGroupIDQuery> prepareGroupQuery()
{
    auto query = std::make_shared<TelinkMeshProtocol::TelinkMeshGroupIDQuery>();
    query->setDestNode(0xFFFF);            
    query->setMode(0x010A);
    return query;
}

std::shared_ptr<TelinkMeshProtocol::TelinkLightStatusQuery> prepareStatusQuery()
{
    auto query = std::make_shared<TelinkMeshProtocol::TelinkLightStatusQuery>();
    query->setDestNode(0xFFFF);            
    query->setMode(0x10);
    return query;
}

#endif