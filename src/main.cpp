#include "ble_stack/telink_mesh.h"
#include "mqtt/mqtt_client_proxy.h"
#include "gateway/gateway.h"

int main() {
    while(true)
    {
        try
        {
            
            
            Gio::init();
            // Create and run the main event loop to handle signals
            auto mainLoop = Glib::MainLoop::create();

            BlueZProxy btproxy;

            btproxy.disconnect_by_name("Telink tLight");
            btproxy.disconnect_by_name("telink_mesh1");

            auto mesh = std::make_shared<TelinkMesh>(
                            btproxy,
                            "telink_mesh1",
                            "123",
                            0x0211
                            );
            
            auto mqtt_client = std::make_shared<MQTTClientProxy>("tcp://localhost:1883", "mqtt-client");

            Gateway gateway(mesh,mqtt_client);
            
            mesh->setConnectedCallback([mesh]() {
                // Schedule the lambda function in the GLib main loop after a timeout
                Glib::signal_timeout().connect_once([mesh]() {
                    // Create the query
                    auto query = std::make_shared<TelinkMeshProtocol::TelinkMeshAddressEdit>();
                    query->setDestNode(0xFFFF);            
                    query->setMode(0xFFFF);

                    // Send the query
                    mesh->send(query);
                    
                }, 5000); // Timeout duration in milliseconds
            });

        // btproxy.start_rssi_scan();

            mesh->connect();
            mqtt_client->connect();
            mqtt_client->subscribe("homeassistant/light/+/set");
            
            mainLoop->run();  // Start the loop that processes the incoming signals 
            /* code */
        }
        catch(const std::exception& e)
        {
            g_warning("Unhandled exception: %s \n\n Attempting to restart...",e.what());
        }
    }
}
