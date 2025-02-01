#define G_LOG_USE_STRUCTURED 1
#include "ble_stack/telink_mesh.h"
#include "mqtt/mqtt_client_proxy.h"
#include "gateway/gateway.h"
#include "logging/log_handler.h"

int main() {
    g_log_set_writer_func(structured_log_writer, NULL, NULL);
    g_message("test");
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

            mainLoop->run();  // Start the loop that processes the incoming signals 
            /* code */
        }
        catch(const std::exception& e)
        {
            g_warning("Unhandled exception: %s \n\n Attempting to restart...",e.what());
        }
    }
}
