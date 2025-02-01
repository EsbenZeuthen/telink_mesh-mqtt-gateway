
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

/*void TelinkMesh::setConnectedCallback(sigc::slot<void> connectCallback)
{
    sigConnected.connect(connectCallback);
}*/

void TelinkMesh::setRxCallback(sigc::slot<void,std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket>> rxCallback)
{
    sigPacketRx.connect(rxCallback);
}

bool TelinkMesh::isReady()
{
    if (connectedDevice)
    {
        return true;
    }
    else
    {
        discover();
        return false;
    }
}
    
void TelinkMesh::onReady(std::function<void()> callback)
{
    callback_on_ready = callback;
}

void TelinkMesh::send(const std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket> packet)
{    
    try
    {
        if (!connectedDevice || !connectedDevice->send(packet))        
        {            
            throw std::runtime_error("Send failed");
        }
    }
    catch(const std::exception& e)
    {
        // assume the connection is broken
        g_debug("Send error %s, assuming connection is broken.",e.what());
        connectedDevice = nullptr;
        discover();
        throw;
    }
    catch(...)
    {
        // assume the connection is broken
        g_debug("Unknown exception type during send, assuming connection is broken.");
        connectedDevice = nullptr;
        discover();
        throw;
    }
}

void TelinkMesh::discover()
{   
    if (!discovering)
    {        
        ble.disconnect_by_name(mesh_name);
        discovering = true;
        connectedDevice = nullptr;
        current_best_device = nullptr;
        // start scanning for a mesh device
        ble.start_rssi_scan(sigc::mem_fun(this,&TelinkMesh::on_device_found_rssi));
        g_message("Scanning for %s",mesh_name.c_str());
        // Set a timeout to check discovery after 'timeout_seconds' seconds    
        Glib::signal_timeout().connect([this]()
        {                
            if (this->current_best_device)
            {
                this->ble.stop_scan();
                discovering = false;
                this->connect(1);                
                return false;
            }
            else
            {
                g_warning("No device with name %s found during scan. Continuing scan...",this->mesh_name.c_str());            
                return true;
            }
        }, 5000);
    }
}

bool TelinkMesh::ConnectedDevice::send(const std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket> packet)
{
    if(packet_seq == 0) {packet_seq++;}
    packet->setSeq(packet_seq++);    
    packet->setVendorCode(vendor_code);

    g_debug("Sending mesh packet:");
    packet->debug();
    auto data = packet->getData();
    auto enc_packet = crypto::encrypt_packet(shared_key, macdata, data);

    return ble.write(device_info->Address,"00010203-0405-0607-0809-0a0b0c0d1912",enc_packet);   
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

void TelinkMesh::on_packet_rx(std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket> packet)
{
    sigPacketRx.emit(packet);
}

void TelinkMesh::connect(uint8_t retries)
{
    if (current_best_device && ble.connect(current_best_device->Address))    
    {
        connectedDevice=std::make_unique<ConnectedDevice>(ble,
                                                         current_best_device,
                                                         mesh_name,
                                                         mesh_password,
                                                         vendor_code,
                                                         sigc::mem_fun(this,&TelinkMesh::on_packet_rx));
        pair(1);
    }
    else if (retries>0)
    {
        g_warning("Connecting to device %s failed, trying again...",current_best_device->Address.c_str());
        Glib::signal_timeout().connect_once([this,retries]() { connect(retries-1);}, 5000);
    } else
    {
        // retry discovery
        g_warning("Connecting to device %s failed, fallback to discovery...",current_best_device->Address.c_str());
        this->discover();
    }
}

void TelinkMesh::pair(uint8_t retries)
{
    if (connectedDevice->pair())
    {
        connectedDevice->activate_notifications();        

        g_message("Paired with %s(%s)",
                   connectedDevice->device_info->Name.c_str(),
                   connectedDevice->device_info->Address.c_str());   
       // sigConnected.emit();

       Glib::signal_timeout().connect_once([this]() {
            if (callback_on_ready)
            {
                callback_on_ready();
                callback_on_ready=nullptr;
            }
       },500);
    }
    else if (retries>0)
    {
        g_warning("Pairing with device %s failed, trying again...",current_best_device->Address.c_str());
        Glib::signal_timeout().connect_once([this,retries]() { pair(retries-1);}, 5000);
    } else
    {
        // retry discovery
        g_warning("Pairing with device %s failed, fallback to discovery...",current_best_device->Address.c_str());
        this->discover();
    }

}

TelinkMesh::ConnectedDevice::ConnectedDevice( BlueZProxy& ble,
                std::shared_ptr<BlueZProxy::Device> device_info,
                std::string mesh_name,
                std::string mesh_password,
                uint16_t vendor_code,
                sigc::slot<void,std::shared_ptr<TelinkMeshProtocol::TelinkMeshPacket>> rxCallback)
                    : ble(ble),
                      device_info(device_info),
                      mesh_name(mesh_name),
                      mesh_password(mesh_password),
                      vendor_code(vendor_code)
{
    sigPacketRx.connect(rxCallback);
    macdata = mac_to_reversed_vector(device_info->Address);
}

TelinkMesh::ConnectedDevice::~ConnectedDevice()
{

}

bool TelinkMesh::ConnectedDevice::pair()
{
    // Prepare pairing request
    auto data = std::vector<uint8_t>(16,0x00);
    auto random_data = crypto::get_random_bytes(8);
    for (int i=0;i<8;i++){data[i]=random_data[i];}
    auto enc_data = crypto::key_encrypt(mesh_name, mesh_password, data);
    std::vector<uint8_t> packet = {0x0c}; // Start with 0x0c
    packet.insert(packet.end(), data.begin(), data.begin()+8); // Add data
    packet.insert(packet.end(), enc_data.begin(), enc_data.begin() + 8); // Add first 8 bytes of encrypted data

    const std::string pairing_char_uuid = "00010203-0405-0607-0809-0a0b0c0d1914";

    g_debug("Submitting pairing request");
    // submit pairing request
    if (!ble.write(device_info->Address,pairing_char_uuid,packet)){return false;};

    // wait for response
    g_debug("Waiting for 500ms...");
    std::this_thread::sleep_for(std::chrono::milliseconds(500)) ;
    // read pairing response
    g_debug("Reading pairing response");
    auto data2 = ble.read(device_info->Address,pairing_char_uuid);       
        
    // Generate the shared key
    shared_key = crypto::generate_sk(mesh_name,mesh_password,random_data,std::vector<uint8_t>(data2.begin()+1,data2.begin()+9));

    return true;
}

void TelinkMesh::ConnectedDevice::activate_notifications()
{
        g_debug("Enabling data notifications");
        // IMPORTANT - IMPORTANT - IMPORTANT - IMPORTANT - IMPORTANT - IMPORTANT - IMPORTANT 
        //        This requires a patched bluez stack, or else a timeout will occur!!!!
        // IMPORTANT - IMPORTANT - IMPORTANT - IMPORTANT - IMPORTANT - IMPORTANT - IMPORTANT 
        ble.start_notify(device_info->Address,
                         "00010203-0405-0607-0809-0a0b0c0d1911",
                        sigc::mem_fun(this, &TelinkMesh::ConnectedDevice::on_data_rx));

        // we need to write this value to actually start notifications
        ble.write(device_info->Address,"00010203-0405-0607-0809-0a0b0c0d1911",std::vector<uint8_t>({0x01}));
     
}

void TelinkMesh::ConnectedDevice::on_data_rx(const std::vector<uint8_t>& data)
{        
    try {    
        auto decrypted_data = crypto::decrypt_packet(shared_key,macdata,data);
        
        auto packet = TelinkMeshProtocol::TelinkMeshPacket::create(decrypted_data);
        
        g_info("Received mesh packet");
        packet->debug();
        sigPacketRx.emit(packet);
    }
    catch(std::exception e)
    {
        g_warning("Unexpected data from mesh, dropping data packet. Exception: %s",e.what());
    }
}

// Function to convert MAC address to a reversed std::vector<uint8_t>
std::vector<uint8_t> TelinkMesh::ConnectedDevice::mac_to_reversed_vector(const std::string& mac_address) {
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