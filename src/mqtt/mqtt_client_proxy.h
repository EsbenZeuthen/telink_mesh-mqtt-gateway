#ifndef MQTT_CLIENT_PROXY_H
#define MQTT_CLIENT_PROXY_H

#include <iostream>
#include <glibmm/main.h>
#include <mqtt/client.h>

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "MQTT Client"

class MQTTEventSource : public Glib::Source {
public:
    MQTTEventSource(mqtt::client& mqttclient)
        : mqttclient(mqttclient), pending_event(false) {

        sigc::slot<bool> slot = sigc::mem_fun(*this, &MQTTEventSource::on_dispatch);
        this->connect_generic(slot);

        attach();
    }
    
    virtual void trigger_event() {
        pending_event = true;
        get_context()->wakeup();
    }


protected:
    
    bool dispatch(sigc::slot_base* /*callback*/) override {
        // Perform the actual work for the source here
        std::cout << "Custom event triggered!" << std::endl;
        // Return true to keep the source active, or false to remove it
        pending_event = false;
        return true;
    }

    bool on_dispatch() {
        std::cout << "Custom event dispatched!" << std::endl;
        return true; // Keep the source active
    }

    bool prepare(int& timeout) override {
        timeout = -1;  // No timeout needed
        return false;
    }

    bool check() override {
        // Check if the source is ready to be dispatched
        return pending_event; // Always dispatch in this example
    }

    bool pending_event;

    mqtt::client& mqttclient;

};

class MQTTMessageSource : public MQTTEventSource {
public:

    MQTTMessageSource(mqtt::client& mqttclient) : MQTTEventSource(mqttclient){};

    void setCallback(sigc::slot<bool,mqtt::const_message_ptr> callback)
    {
        sigMessageRx.connect(callback);
    }

    bool dispatch(sigc::slot_base* callback) override {
        try{
            g_debug("Dequeueing MQTT message");
            // get message from the queue
            mqtt::const_message_ptr msg;
            pending_event = mqttclient.try_consume_message(&msg);

            if (pending_event)
            {
                if (!sigMessageRx.emit(msg))
                {
                    g_warning("Could not handle consumed MQTT message, stopping MQTT consumption...");
                    mqttclient.stop_consuming();
                };
            }
        }
        catch(const std::exception& e)
        {
            g_warning("Unexpected exception: %s",e.what());
        }                                
        // TODO: maybe we need to call wake up here if more messages are pending?
        return true;
    }

    sigc::signal<bool,mqtt::const_message_ptr> sigMessageRx;
        
};

 
class MQTTCallback : public virtual mqtt::callback {
public:
    MQTTCallback(MQTTMessageSource& rxSource)
        : rxSource(rxSource) {}

    void connected(const std::string& cause) override {
        
        //source->trigger_event();  // Trigger the event directly in the custom source
        g_message("Connected with MQTT broker. Cause: %s",cause.c_str());
    }

    void connection_lost(const std::string& cause) override {
        
      //  source->trigger_event();  // Trigger the event directly in the custom source
      g_warning("Lost connection with MQTT broker. Cause: %s",cause.c_str());
    }

    void message_arrived(mqtt::const_message_ptr msg) override {
        try
        {
            /* code */
                
            // Extract the payload and topic from the message
            std::string payload = msg->get_payload_str();  // Get the payload as a string
            std::string topic = msg->get_topic();

            // Use g_debug to output the topic and payload of the message
            g_debug("Received MQTT message with topic: %s", topic.c_str());
            g_debug("Message payload: %s", payload.c_str());
            
            rxSource.trigger_event();  // Trigger the event directly in the custom source
        }
        catch(const std::exception& e)
        {
            g_warning("Unexpected exception: %s",e.what());
        }                                
    }
    
    void delivery_complete (mqtt::delivery_token_ptr tok)
    {
        
    }

private:
    MQTTMessageSource& rxSource;  // Pointer to the custom MQTT source
};



// Connection event source
/*class ConnectionSource : public MQTTEventSource {
public:
    ConnectionSource(Glib::RefPtr<Glib::MainLoop> loop)
        : MQTTEventSource(loop) {}

    void trigger_event() override {
        // Logic for handling connection event
        Glib::signal_idle().connect_once([this]() {
            std::cout << "Handling MQTT connection event" << std::endl;
        });
    }
};*/




class MQTTClientProxy {
public:
    MQTTClientProxy(const std::string& server_uri, const std::string& client_id)
        : client(server_uri, client_id), rxSource(client), callback(rxSource)
    {

        client.set_callback(callback);
    }

    void connect() {
        mqtt::connect_options conn_opts;
        conn_opts.set_clean_session(true);
        try {
            client.connect(conn_opts);
            g_info("Connected to broker.");
        } catch (const mqtt::exception& e) {
            g_warning("Unexpected exception: %s",e.what());            
        }
    }

    void setCallback(sigc::slot<bool,mqtt::const_message_ptr> callback)
    {
        rxSource.setCallback(callback);
    }

    void subscribe(std::string topicfilter) {
        client.subscribe(topicfilter, 1);        
    }

    void start_consuming()
    {
        client.start_consuming();
    }

    void publish(mqtt::const_message_ptr 	msg)
    {
        // Extract the payload and topic from the message
        std::string payload = msg->get_payload_str();  // Get the payload as a string
        std::string topic = msg->get_topic();

        // Use g_debug to output the topic and payload of the message
        g_debug("Publishing MQTT message to topic: %s", topic.c_str());
        g_debug("Message payload: %s", payload.c_str());
        client.publish(msg);
    }

private:
    mqtt::client client;
    MQTTMessageSource rxSource;
    MQTTCallback callback;
};

#endif