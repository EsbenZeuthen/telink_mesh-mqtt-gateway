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

    void connect();

    void send(const std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket> packet);

protected:
    
    void on_data_rx(const std::vector<uint8_t>& data);
    void on_device_found(const std::string address, const std::string name);
    void on_device_found_rssi(std::shared_ptr<BlueZProxy::Device> device_info);
    
    std::vector<uint8_t> mac_to_reversed_vector(const std::string& mac_address);

    std::string macaddress;
    std::string mesh_name;
    std::string mesh_password;
    uint16_t vendor_code;
    std::vector<uint8_t> macdata;
    std::vector<uint8_t> shared_key;
    std::shared_ptr<BlueZProxy::Device> current_best_device = nullptr;

    BlueZProxy& ble;

    bool connected = false;
    uint16_t packet_seq = 1;


    // callback signal
    sigc::signal<void,std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket>> sigPacketRx;
    sigc::signal<void> sigConnected;
};

#endif