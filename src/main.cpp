#define G_LOG_USE_STRUCTURED 1
#include "ble_stack/telink_mesh.h"
#include "mqtt/mqtt_client_proxy.h"
#include "gateway/gateway.h"
#include "logging/log_handler.h"

int main() {
    g_log_set_writer_func(structured_log_writer, NULL, NULL);
    const char* mesh_name = std::getenv("MESH_NAME");
    const char* mesh_password = std::getenv("MESH_PASSWORD");
    const char* mesh_connected_name = std::getenv("MESH_CONNECTED_NAME");
    const char* mqtt_broker_url = std::getenv("MQTT_BROKER_URL");
    const char* mqtt_client_id = std::getenv("MQTT_CLIENT_ID");

    while(true)
    {
        try
        {            
            Gio::init();
            // Create and run the main event loop to handle signals
            auto mainLoop = Glib::MainLoop::create();

            BlueZProxy btproxy;

            btproxy.disconnect_by_name(mesh_connected_name);
            btproxy.disconnect_by_name(mesh_name);

            auto mesh = std::make_shared<TelinkMesh>(
                            btproxy,
                            mesh_name,
                            mesh_password,
                            0x0211
                            );
            
            auto mqtt_client = std::make_shared<MQTTClientProxy>(mqtt_broker_url, mqtt_client_id);

            Gateway gateway(mesh,mqtt_client);

            mainLoop->run();  // Start the loop that processes the incoming signals 
            /* code */
        }
        catch(const std::exception& e)
        {
            g_warning("Unhandled exception: %s \n\n Attempting to restart...",e.what());
        } catch (const Glib::Error& e) {
            g_warning("Unhandled exception: %s",e.what().c_str());   
        } catch (...)   
        {
            g_warning("Caught unknown exception type...");   
        }
    
    }
}
