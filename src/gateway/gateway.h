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
                         
            mqtt->connect();
            mqtt->subscribe("homeassistant/light/+/set");

            // start the heartbeat with an address query
            heartbeat(true,30000);

            // Schedule heartbeat/discovery
            /*Glib::signal_timeout().connect([this]() {
                try
                {
                    g_debug("Status query heartbeat");
                    auto query = prepareStatusQuery();
                    this->send_if_ready({query});
                }
                catch(const std::exception& e)
                {
                    g_warning("Unexpected exception: %s",e.what());
                }                                
                return true;
            }, 30000);

            Glib::signal_timeout().connect([this]() {
                try
                {
                    g_debug("Address query heartbeat");
                    auto query = prepareAddressQuery();
                    this->send_if_ready({query});
                }
                catch(const std::exception& e)
                {
                    g_warning("Unexpected exception: %s",e.what());
                }                                
                return true;
            }, 60000);*/
        }

        void heartbeat(bool address_or_status, uint32_t interval)
        {
            try
            {
                if (address_or_status){
                    g_debug("Address query heartbeat");
                    auto query = prepareAddressQuery();
                    this->send_if_ready({query});
                } else {
                    g_debug("Status query heartbeat");
                    auto query = prepareStatusQuery();
                    this->send_if_ready({query});
                }
            }
            catch(const std::exception& e)
            {
                g_warning("Unexpected exception: %s",e.what());
            }
            Glib::signal_timeout().connect_once([this,address_or_status,interval]() {heartbeat(!address_or_status,interval);},interval);
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

        void mqtt_publish() // vector of mqtt messages
        {
            // check mqtt connection state
            // if not connected, connect
        }

        // return true if message was sent synchronously, false otherwise
        // (message will be sent when mesh is ready)
        bool onMqttMessage(mqtt::const_message_ptr msg)
        {
            if (mqtt_enabled)
            {
                // map to telink and submit
                auto packets = mqtt_to_telink(msg);
                mqtt_enabled = send_when_ready(packets);
            }
            return mqtt_enabled;
        }
     
        bool readyToSend(std::function<bool()> callback) {
            if (mesh->isReady()) {
                // Mesh is ready, invoke the callback immediately
                return callback();                
            } else {
                // Set the callback to be invoked when the mesh becomes ready                
                mesh->onReady([callback,this]() {
                    if (callback())
                    {
                        // enable mqtt
                        mqtt_enabled = true;
                        //start consuming
                        mqtt->start_consuming();
                    }
                });
                return false;
            }
        }

        // return true if message was sent synchronously, false otherwise
        // (message will be sent when mesh is ready)
        bool send_when_ready(std::vector<std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket>> packets, int retryCount = 0) {
            const int maxRetries = 5;            
            return readyToSend([packets, retryCount, this]() {
                try {
                    for (auto packet : packets)
                    {               
                        mesh->send(packet);                        
                    }                    
                    return true;
                } catch (const std::exception& e) {                    
                    if (retryCount < maxRetries) {
                        g_warning("Send failed: %s. Retrying (%u/%u).",e.what(),retryCount+1,maxRetries);                    

                        Glib::signal_timeout().connect_once(
                            [packets, retryCount, this]() {
                                try {
                                    if(send_when_ready(packets, retryCount + 1))
                                    {
                                        // enable mqtt
                                        mqtt_enabled = true;
                                        //start consuming
                                        mqtt->start_consuming();
                                    }
                                }
                                catch (std::exception e)
                                {
                                    g_warning("Unexpected exception: %s",e.what());
                                }
                            },
                            1000 // Retry delay in milliseconds
                        );
                    } else {
                        g_warning("Send failed after maximum retries, dropping packets!");
                        // if we are here, mesh has not detected an error state - assume we were just unlucky
                        mqtt_enabled = true;                        
                        mqtt->start_consuming();
                    }
                    return false;
                }
            });
        }


        void send_if_ready(std::vector<std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket>> packets)
        {
            if (mesh->isReady())
            {
                 try {
                    for (auto packet : packets)
                    {               
                        mesh->send(packet);
                    }                    
                } catch (const std::exception& e)                
                {
                    g_warning("Error during send - dropping packets. Exception %s",e.what());
                } catch (...)
                {
                    g_warning("Error during send - dropping packets. Exception type unknown.");
                }
            }
            else
            {
                g_warning("Mesh not ready - dropping packets.");
            }           
        }
    
    protected:
        std::shared_ptr<TelinkMesh> mesh;
        std::shared_ptr<MQTTClientProxy> mqtt;
        bool mqtt_enabled;
};

#endif