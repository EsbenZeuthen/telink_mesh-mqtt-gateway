#ifndef TELINK_MESH_H
#define TELINK_MESH_H

#include <vector>
#include <string>

#include "bluezproxy.h"
#include "telink_mesh_protocol.h"

/* Responsibilities:
    establish and maintain mesh node connection
    send and receive mesh packets
    encrypt/decrypt packets
*/
class TelinkMesh {
public:
    
    TelinkMesh(BlueZProxy& bluetoothproxy,
               const std::string& mesh_name,
               const std::string& mesh_password,
               const uint16_t& vendor_code
    );
    ~TelinkMesh();
    
    void setConnectedCallback(sigc::slot<void> connectCallback);
    void setRxCallback(sigc::slot<void,std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket>> rxCallback);    

    bool isReady();
    
    void onReady(std::function<void()> callback);

    void send(const std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket> packet);

protected:
    
    enum mesh_state : uint8_t
    {
        INITIALIZED = 1,
        BLUEZ_CONNECTED = 2,
        DISCOVERING = 3,
        DISCOVERED = 4,
        CONNECTED = 5,
        PAIRED = 6,
        NOTIFYING = 7,
        READY = 8
    };

    class ConnectedDevice : public sigc::trackable
    {
        public:
            ConnectedDevice( BlueZProxy& ble,
                             std::shared_ptr<BlueZProxy::Device> device_info,
                             std::string mesh_name,
                             std::string mesh_password,
                             uint16_t vendor_code,
                             sigc::slot<void,std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket>> rxCallback);                             
            
            ~ConnectedDevice();

            void pair();
            void activate_notifications();
            void send(const std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket> packet);

            std::shared_ptr<BlueZProxy::Device> device_info;
        protected:
            void on_data_rx(const std::vector<uint8_t>& data);
            std::vector<uint8_t> mac_to_reversed_vector(const std::string& mac_address);

            BlueZProxy& ble;        
            uint16_t packet_seq = 1;
            std::string mesh_name;
            std::string mesh_password;
            uint16_t vendor_code;
            std::vector<uint8_t> macdata;
            std::vector<uint8_t> shared_key;

            sigc::signal<void,std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket>> sigPacketRx;
            
    };

    void discover();
    void connect(uint8_t retries);
    void on_device_found_rssi(std::shared_ptr<BlueZProxy::Device> device_info);    
    void on_packet_rx(std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket> packet);
    
    
    std::string mesh_name;
    std::string mesh_password;
    uint16_t vendor_code;

    std::shared_ptr<BlueZProxy::Device> current_best_device = nullptr;
    std::unique_ptr<ConnectedDevice> connectedDevice = nullptr;

    BlueZProxy& ble;

    bool discovering = false;
        

    std::function<void()> callback_on_ready=nullptr;;
    // callback signal
    sigc::signal<void,std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket>> sigPacketRx;
    //sigc::signal<void> sigConnected;
};

#endif