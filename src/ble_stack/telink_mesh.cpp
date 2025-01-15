
#include "../crypto/crypto.h"
#include <thread>

#include "telink_mesh.h"

TelinkMesh::TelinkMesh( BlueZProxy& bluetoothproxy,
                        const std::string& mesh_name,
                        const std::string& mesh_password,
                        const uint16_t& vendor_code                        
                                  ) : mesh_name(mesh_name),
                                      mesh_password(mesh_password),
                                      vendor_code(vendor_code),
                                      ble(bluetoothproxy)
{        
}

TelinkMesh::~TelinkMesh()
{}

void TelinkMesh::setConnectedCallback(sigc::slot<void> connectCallback)
{
    sigConnected.connect(connectCallback);
}

void TelinkMesh::setRxCallback(sigc::slot<void,std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket>> rxCallback)
{
    sigPacketRx.connect(rxCallback);
}

void TelinkMesh::connect()
{    
    current_best_device = nullptr;
    // start scanning for a mesh device
    ble.start_rssi_scan(sigc::mem_fun(this,&TelinkMesh::on_device_found_rssi));
    g_message("Scanning for %s",mesh_name.c_str());
    // Set a timeout to stop discovery after 'timeout_seconds' seconds
    Glib::signal_timeout().connect([this]() {                
        if (this->current_best_device)
        {
            this->ble.stop_scan();
            this->on_device_found(this->current_best_device->Address,this->current_best_device->Name);
            return false;
        }
        else
        {
            g_warning("No device with name %s found during scan. Continuing scan...",this->mesh_name.c_str());            
            return true;
        }
    }, 5000);
}

void TelinkMesh::send(const std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket> packet)
{
    if(packet_seq == 0) {packet_seq++;}
    packet->setSeq(packet_seq++);    
    packet->setVendorCode(vendor_code);

    g_debug("Sending mesh packet:");
    packet->debug();
    auto data = packet->getData();
    auto enc_packet = crypto::encrypt_packet(shared_key, macdata, data);
    ble.write(macaddress,"00010203-0405-0607-0809-0a0b0c0d1912",enc_packet);
}

void TelinkMesh::on_device_found_rssi(std::shared_ptr<BlueZProxy::Device> device_info)
{
    // check if device matches the filter and if device RSSI is better than what we have seen so far
    if ((device_info->Name == mesh_name)
        && (!current_best_device || device_info->RSSI > current_best_device->RSSI))
    {
        current_best_device = device_info;
    }
}

void TelinkMesh::on_device_found(const std::string address, const std::string name)
{
    if (name == mesh_name && !connected)
    {   
        g_message("Found %s with mac %s - connecting",name.c_str(),address.c_str());     
        
        if (!ble.connect(address))
        {
            Glib::signal_timeout().connect_once([this,address,name]() {
                  this->on_device_found(address,name);
            },5000);
        }

        // Prepare pairing request
        auto data = std::vector<uint8_t>(16,0x00);
        auto random_data = crypto::get_random_bytes(8);
        for (int i=0;i<8;i++){data[i]=random_data[i];}
        auto enc_data = crypto::key_encrypt(mesh_name, mesh_password, data);
        std::vector<uint8_t> packet = {0x0c}; // Start with 0x0c
        packet.insert(packet.end(), data.begin(), data.begin()+8); // Add data
        packet.insert(packet.end(), enc_data.begin(), enc_data.begin() + 8); // Add first 8 bytes of encrypted data

        const std::string pairing_char_uuid = "00010203-0405-0607-0809-0a0b0c0d1914";

        // wait for bluetooth stack to populate device attributes
        g_debug("Waiting for 3000ms...");
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));

        g_debug("Submitting pairing request");
        // submit pairing request
        ble.write(address,pairing_char_uuid,packet);

        // wait for response
        g_debug("Waiting for 500ms...");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // read pairing response
        g_debug("Reading pairing response");
        auto data2 = ble.read(address,pairing_char_uuid);       
                
        // Generate the shared key
        shared_key = crypto::generate_sk(mesh_name,mesh_password,random_data,std::vector<uint8_t>(data2.begin()+1,data2.begin()+9));

        macaddress = address;
        macdata = mac_to_reversed_vector(address);

        g_debug("Enabling data notifications");
        // IMPORTANT - IMPORTANT - IMPORTANT - IMPORTANT - IMPORTANT - IMPORTANT - IMPORTANT 
        //        This requires a patched bluez stack, or else a timeout will occur!!!!
        // IMPORTANT - IMPORTANT - IMPORTANT - IMPORTANT - IMPORTANT - IMPORTANT - IMPORTANT 
        ble.start_notify(address,
                         "00010203-0405-0607-0809-0a0b0c0d1911",
                        sigc::mem_fun(this, &TelinkMesh::on_data_rx));

        // we need to write this value to actually start notifications
        ble.write(address,"00010203-0405-0607-0809-0a0b0c0d1911",std::vector<uint8_t>({0x01}));

        connected = true;
        g_message("Connected to %s(%s)",name.c_str(),address.c_str());   

        sigConnected.emit();
    }        
}

void TelinkMesh::on_data_rx(const std::vector<uint8_t>& data)
{        
    auto decrypted_data = crypto::decrypt_packet(shared_key,macdata,data);
    
    auto packet = TelinkMeshProtocol::TelinkMeshPacket::create(decrypted_data);
    
    g_info("Received mesh packet");
    packet->debug();
    sigPacketRx.emit(packet);
}

// Function to convert MAC address to a reversed std::vector<uint8_t>
std::vector<uint8_t> TelinkMesh::mac_to_reversed_vector(const std::string& mac_address) {
    std::vector<uint8_t> mac_bytes;
    std::istringstream iss(mac_address);
    std::string byte_str;

    // Split the MAC address by ':' or '-' delimiters
    while (std::getline(iss, byte_str, (mac_address.find(':') != std::string::npos) ? ':' : '-')) {
        if (byte_str.length() != 2) {
            throw std::invalid_argument("Invalid MAC address format: " + mac_address);
        }

        // Convert each byte from hex to uint8_t
        uint8_t byte = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
        mac_bytes.push_back(byte);
    }

    if (mac_bytes.size() != 6) {
        throw std::invalid_argument("Invalid MAC address length: " + mac_address);
    }

    // Reverse the byte order
    std::reverse(mac_bytes.begin(), mac_bytes.end());
    return mac_bytes;
}