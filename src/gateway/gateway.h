#ifndef GATEWAY_H
#define GATEWAY_H

#include "../ble_stack/telink_mesh.h"
#include "../mqtt/mqtt_client_proxy.h"
#include "mappings.h"

class Gateway
{
    public:

        Gateway(std::shared_ptr<TelinkMesh> mesh, std::shared_ptr<MQTTClientProxy> mqtt)
            : mesh(mesh), mqtt(mqtt)
        {

            mesh->setRxCallback(sigc::mem_fun(this,&Gateway::onMeshMessage));
            mqtt->setCallback(sigc::mem_fun(this,&Gateway::onMqttMessage));

             mesh->setConnectedCallback([mesh]() {
                // Schedule the lambda function in the GLib main loop after a timeout
                Glib::signal_timeout().connect([mesh]() {
                    // TODO: check if connection is alive                           
                    auto query = prepareStatusQuery();
                    mesh->send(query);
                    return true;
                }, 5000);

                Glib::signal_timeout().connect([mesh]() {                           
                    // TODO: check if connection is alive
                    auto query = prepareAddressQuery();
                    mesh->send(query);
                    return true;
                }, 10000);

                Glib::signal_timeout().connect_once([mesh]() {                           
                    // TODO: check if connection is alive
                    auto query = prepareAddressQuery();
                    mesh->send(query);                    
                }, 3000);
            });

             
            }

        void onMeshMessage(std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket> msg)
        {
            // map to mqtt and publish
            auto mqttmsg = telink_to_mqtt(msg);

            if (mqttmsg)
            {
                mqtt->publish(mqttmsg);
            }

            if (msg->getCommand() == TelinkMeshProtocol::Command::COMMAND_ONLINE_STATUS_REPORT
             || msg->getCommand() == TelinkMeshProtocol::Command::COMMAND_ADDRESS_REPORT)
             {
                // announce availability
                auto mqttmsg = telink_to_mqtt_availability(msg);
                if (mqttmsg)
                {
                    mqtt->publish(mqttmsg);
                }
             }
        }

        void onMqttMessage(mqtt::const_message_ptr msg)
        {
            // map to telink and submit
            auto packets = mqtt_to_telink(msg);

            for (auto packet : packets)
            {               
                mesh->send(packet);
            }
        }
    
    protected:
        std::shared_ptr<TelinkMesh> mesh;
        std::shared_ptr<MQTTClientProxy> mqtt;
};

#endif