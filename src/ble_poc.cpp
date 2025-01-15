#include <giomm/dbusproxy.h>
#include <giomm/dbusconnection.h>
#include <giomm.h>
#include <iostream>
#include <iomanip>
#include <map>
#include <mutex>
#include <vector>
#include <thread>
#include "crypto/crypto.h"

class BluetoothDevice {
public:
    BluetoothDevice(const std::string& devicePath)
        : devicePath(devicePath) {}

    // Retrieve a characteristic proxy by UUID
    Glib::RefPtr<Gio::DBus::Proxy> getCharacteristicProxy(const std::string& uuid) {
        std::lock_guard<std::mutex> lock(mutex);

        if (characteristicsCache.find(uuid) != characteristicsCache.end()) {
            return characteristicsCache[uuid];
        }

        auto characteristic = queryCharacteristic(uuid);
        characteristicsCache[uuid] = characteristic;
        return characteristic;
    }

private:
    Glib::RefPtr<Gio::DBus::Proxy> queryCharacteristic(const std::string& uuid) {
        try {
            auto deviceProxy = Gio::DBus::Proxy::create_sync(connection, "org.bluez", devicePath, "org.bluez.Device1");
            auto introspect = deviceProxy->call_sync("org.freedesktop.DBus.Introspectable.Introspect", Glib::VariantContainerBase());
            auto xmlVariant = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(introspect.get_child(0));
            auto xmlData = xmlVariant.get();

            std::string characteristicPath = findCharacteristicPathFromXML(xmlData, uuid);
            auto characteristicProxy = Gio::DBus::Proxy::create_sync(connection, "org.bluez", characteristicPath, "org.bluez.GattCharacteristic1");
            return characteristicProxy;
        } catch (const Glib::Error& e) {
            std::cerr << "Error querying characteristic: " << e.what() << std::endl;
            return Glib::RefPtr<Gio::DBus::Proxy>();
        }
    }

    std::string findCharacteristicPathFromXML(const Glib::ustring& xmlData, const std::string& uuid) {
        size_t pos = xmlData.find(uuid);
        if (pos != std::string::npos) {
            size_t pathStart = xmlData.find("/org/bluez", pos);
            if (pathStart != std::string::npos) {
                size_t pathEnd = xmlData.find(" ", pathStart);
                return xmlData.substr(pathStart, pathEnd - pathStart);
            }
        }
        return "";
    }

private:
    std::string devicePath;
    Glib::RefPtr<Gio::DBus::Connection> connection;
    std::map<std::string, Glib::RefPtr<Gio::DBus::Proxy>> characteristicsCache;
    std::mutex mutex;
};

class BluetoothManager {
public:
    BluetoothManager() {
        connection = Gio::DBus::Connection::get_sync(Gio::DBus::BUS_TYPE_SYSTEM);
        startDeviceDiscovery();
    }

    // Start scanning for devices
   void startDeviceDiscovery() {
    try {
        // Listen for DeviceAdded signals (new devices discovered) before starting the scan
       connection->signal_subscribe(
            sigc::mem_fun(this, &BluetoothManager::onDeviceAdded), // Slot for the callback
            "org.bluez",         // Sender name (can be empty if not filtering by sender)
            "org.freedesktop.DBus.ObjectManager", // Interface name
            "InterfacesAdded",   // Signal member (signal name)
            {},        // Object path (can be empty if not filtering by path)
            {}                  // First argument to filter by (optional, typically empty)            
        );
        
        // Get the DBus proxy for BlueZ's adapter
        adapterProxy = Gio::DBus::Proxy::create_sync(connection, "org.bluez", "/org/bluez/hci0", "org.bluez.Adapter1");



        auto result = adapterProxy->call_sync("StartDiscovery", Glib::VariantContainerBase());

        // Handle the returned value if any
        if (!result.gobj()) { // Check if the result is empty
            std::cerr << "No response received from StartDiscovery call." << std::endl;
        } else {
            std::cout << "Started scanning for devices successfully." << std::endl;
        }
    

    } catch (const Glib::Error& e) {
        std::cerr << "Error starting device discovery: " << e.what() << std::endl;
    }
}

void stopDeviceDiscovery() {        
    try {
        adapterProxy->call_sync("StopDiscovery");
        std::cout << "Stopped scanning." << std::endl;
    } catch (const Glib::Error& e) {
        std::cerr << "Failed to stop scanning: " << e.what() << std::endl;
    }
}

void connectToDevice(const Glib::ustring& device_path) {
        connected_device_proxy = Gio::DBus::Proxy::create_for_bus_sync(
        Gio::DBus::BUS_TYPE_SYSTEM,
        "org.bluez",
        device_path,
        "org.bluez.Device1"
    );

    try {
        connected_device_proxy->call_sync("Connect");        
        std::cout << "Connected to device: " << device_path << std::endl;

// Set the "Trusted" property to true
        auto proxy = Gio::DBus::Proxy::create_sync(
            connection,
            "org.bluez",
            device_path,
            "org.freedesktop.DBus.Properties"
        );

        proxy->call_sync(
            "Set",
            Glib::VariantContainerBase::create_tuple(std::vector<Glib::VariantBase>({
                Glib::Variant<Glib::ustring>::create("org.bluez.Device1"),
                Glib::Variant<Glib::ustring>::create("Trusted"),
                Glib::Variant<Glib::Variant<bool>>::create(Glib::Variant<bool>::create(true))
            })));


    } catch (const Glib::Error& e) {
        std::cerr << "Failed to connect: " << e.what() << std::endl;
    }
}

std::string findCharacteristic(const Glib::ustring& device_path,const std::string& target_uuid) {


    auto om_proxy = Gio::DBus::Proxy::create_sync(
        connection,
        "org.bluez",                 // BlueZ service
        "/",                 // Specific device path
        "org.freedesktop.DBus.ObjectManager" // Interface for managed objects
    );

    auto result = om_proxy->call_sync("GetManagedObjects");

    // Unpack the variant containing the managed objects
    auto managed_objects = Glib::VariantBase::cast_dynamic<
                                Glib::Variant<
                                std::map<Glib::DBusObjectPathString,
                                    std::map<Glib::ustring,
                                        std::map<Glib::ustring,
                                                Glib::VariantBase>>>>>
                            (result.get_child(0));
    
               
        

        for (const auto& [path, interfaces] : managed_objects.get()) {
            // Check if the object path is under the given device path
          //  std::cout << path << std::endl;
            if (path.find(device_path) == 0) {
      

                auto it = interfaces.find("org.bluez.GattCharacteristic1");
                if (it != interfaces.end()) {
                    // Get the UUID property of the GattCharacteristic1 interface
                    auto properties = it->second;                    
                    if (properties.find("UUID") != properties.end()) {
                        auto uuid = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(properties.at("UUID")).get();                        
                        if (uuid == target_uuid) {
                            std::cout << "Found GATT characteristic path: " << path << std::endl;
                            return path;
                        }
                    }
                }
            }
        }

    // If not found, throw an exception
    throw std::runtime_error("Characteristic with UUID " + target_uuid + " not found");
}


void pairWithDevice(const Glib::ustring& device_path)
{
     std::string pairing_characteristic = findCharacteristic(device_path,"00010203-0405-0607-0809-0a0b0c0d1914");

    auto data = std::vector<uint8_t>(16,0x00);    
    //auto random_data = crypto::get_random_bytes(8);
    auto random_data = std::vector<uint8_t>({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08});
    for (int i=0;i<8;i++){data[i]=random_data[i];}
    auto enc_data = crypto::key_encrypt("telink_mesh1", "123", data);

    
    std::vector<uint8_t> packet = {0x0c}; // Start with 0x0c
    packet.insert(packet.end(), data.begin(), data.begin()+8); // Add data
    packet.insert(packet.end(), enc_data.begin(), enc_data.begin() + 8); // Add first 8 bytes of encrypted data

    // Output the packet bytes
    std::cout << "Packet Bytes: ";
    for (const auto& byte : packet) {
        std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
    }
    std::cout << std::dec << std::endl; // Reset to decimal output


    // Prepare options dictionary
    std::map<Glib::ustring, Glib::VariantBase> options;
    options["type"] = Glib::Variant<Glib::ustring>::create("request");

    auto options_variant = Glib::Variant< std::map<Glib::ustring, Glib::VariantBase>>::create(options);
    // Create a Glib::Variant containing the byte array
    auto value_variant = Glib::Variant<std::vector<uint8_t>>::create(packet);

    /*auto params_variant = Glib::Variant<
                                std::tuple<
                                    Glib::Variant<std::vector<uint8_t>>,
                                    std::map<Glib::ustring, Glib::VariantBase>
                                >
                        >::create(std::make_tuple(value_variant, options));*/

      auto params_variant = Glib::VariantContainerBase::create_tuple(std::vector<Glib::VariantBase>({value_variant,options_variant}));
        
  
   try {
        // Create the proxy for the pairing characteristic using its path
        auto characteristic_proxy = Gio::DBus::Proxy::create_sync(
                                    connection,
                                    "org.bluez",
                                    pairing_characteristic,        
                                    "org.bluez.GattCharacteristic1"
                                );
        
        // Write the packet to the pairing characteristic
        characteristic_proxy->call_sync("WriteValue",params_variant);
        
        std::cout << "Pairing data written successfully" << std::endl;

        // wait for response
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        std::map<Glib::ustring, Glib::VariantBase> read_options;
        auto read_options_variant = Glib::Variant< std::map<Glib::ustring, Glib::VariantBase>>::create(read_options);
        
        auto response = characteristic_proxy->call_sync("ReadValue",Glib::VariantContainerBase::create_tuple(read_options_variant));
        std::cout << response.get_type_string() << std::endl << response.print(true) << std::endl;
        auto response_data = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<uint8_t>>>(response.get_child(0));
        auto data2 = response_data.get();
        std::cout << "Pairing response: ";
        for (auto byte : data2) {
            std::cout << std::hex << static_cast<int>(byte) << " ";
        }
        std::cout << std::endl;
                
        // Generate the shared key
        shared_key=crypto::generate_sk("telink_mesh1","123",random_data,std::vector<uint8_t>(data2.begin()+1,data2.begin()+9));

        

    } catch (const Glib::Error& e) {
        throw std::runtime_error("Pairing failed: " + std::string(e.what()));
    }
}

void on_properties_changed(const Gio::DBus::Proxy::MapChangedProperties& properties_changed,
                           const std::vector<Glib::ustring>& invalidated_properties) {

                            std::cout << "Got notification!" << std::endl;
    // Extract the "Value" property from the changed properties
 /*   if (properties_changed.contains("Value")) {
        auto value_variant = properties_changed.get_child("Value");
        auto value = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<uint8_t>>>(value_variant).get();

        // Print the notification data
        std::cout << "Notification received: ";
        for (auto byte : value) {
            std::cout << std::hex << (int)byte << " ";
        }
        std::cout << std::dec << std::endl;
    }*/
}


/*Parameters
sender_name	The sender of the signal or nullptr if the connection is not a bus connection.
signal_name	The name of the signal.
parameters	A Variant tuple with parameters for the signal.*/

void on_notify(const Glib::ustring& sender_name, const Glib::ustring& signal_name, const Glib::VariantContainerBase& parameters)
{
    std::cout << "Got notification!" << std::endl;
}


void subscribeToNotifications(const Glib::ustring& device_path) {
    try {

        std::string notify_characteristic = findCharacteristic(device_path,"00010203-0405-0607-0809-0a0b0c0d1911");        

        auto characteristic_proxy = Gio::DBus::Proxy::create_sync(
                                    connection,
                                    "org.bluez",
                                    notify_characteristic,        
                                    "org.bluez.GattCharacteristic1"
                                );
   
        connection->signal_subscribe(
            sigc::mem_fun(this, &BluetoothManager::on_properties_changed2), // Slot for the callback
            "org.bluez",         // Sender name (can be empty if not filtering by sender)
            "org.freedesktop.DBus.Properties", // Interface name
            "PropertiesChanged",   // Signal member (signal name)
            notify_characteristic,        // Object path (can be empty if not filtering by path)
            {}                  // First argument to filter by (optional, typically empty)            
        );

        // Enable notifications
        characteristic_proxy->call_sync("StartNotify");
        
        // We need to write the GATT char as well!      
        std::map<Glib::ustring, Glib::VariantBase> options;
        options["type"] = Glib::Variant<Glib::ustring>::create("request");
        auto options_variant = Glib::Variant< std::map<Glib::ustring, Glib::VariantBase>>::create(options);
        // Create a Glib::Variant containing the byte array
        auto value_variant = Glib::Variant<std::vector<uint8_t>>::create(std::vector<uint8_t>({0x01}));    
        auto params_variant = Glib::VariantContainerBase::create_tuple(std::vector<Glib::VariantBase>({value_variant,options_variant}));        
        characteristic_proxy->call_sync("WriteValue",params_variant);
    
        std::cout << "Notifications enabled!" << std::endl;
    } catch (const Glib::Error& e) {
        std::cerr << "Failed to enable notifications: " << e.what() << std::endl;
    }
}

// void on_signal(const Glib::RefPtr<Gio::DBus::Connection>& connection,
//                const Glib::ustring& sender_name,
//                const Glib::ustring& object_path,
//                const Glib::ustring& interface_name,
//                const Glib::ustring& signal_name,
//                const Glib::VariantContainerBase& parameters) {
//     std::cout << "Signal received:" << std::endl;
//     std::cout << "  Sender: " << sender_name << std::endl;
//     std::cout << "  Object Path: " << object_path << std::endl;
//     std::cout << "  Interface: " << interface_name << std::endl;
//     std::cout << "  Signal: " << signal_name << std::endl;

//     // Print signal parameters
//     std::cout << "  Parameters: " << parameters.print() << std::endl;
// }

void send_packet(const Glib::ustring device_path,uint16_t target,uint8_t command,std::vector<uint8_t> data)
{
    uint8_t packet_count = 1;
    uint16_t vendor = 0x0211;
    auto packet = std::vector<uint8_t>(10,0x00);
    packet[0] = packet_count & 0xff;
    packet[1] = packet_count >> 8 & 0xff;
    packet[5] = target & 0xff;
    packet[6] = (target >> 8) & 0xff;
    packet[7] = command;
    packet[8] = vendor & 0xff;
    packet[9] = (vendor >> 8) & 0xff;

    packet.insert(packet.end(),data.begin(),data.end());
    packet.resize(20,0x00);
    auto macdata = std::vector<uint8_t>({0xDE, 0x5A, 0xF4, 0x38, 0xC1, 0xA4}); // TODO: use a parameter
    auto enc_packet = crypto::encrypt_packet(shared_key, macdata, packet);


    std::string command_characteristic = findCharacteristic(device_path,"00010203-0405-0607-0809-0a0b0c0d1912");

   // Prepare options dictionary
    std::map<Glib::ustring, Glib::VariantBase> options;
    options["type"] = Glib::Variant<Glib::ustring>::create("request");
    auto options_variant = Glib::Variant< std::map<Glib::ustring, Glib::VariantBase>>::create(options);
    // Create a Glib::Variant containing the byte array
    auto value_variant = Glib::Variant<std::vector<uint8_t>>::create(enc_packet);
    auto params_variant = Glib::VariantContainerBase::create_tuple(std::vector<Glib::VariantBase>({value_variant,options_variant}));
        
    try {
        // Create the proxy for the pairing characteristic using its path
        auto characteristic_proxy = Gio::DBus::Proxy::create_sync(
                                    connection,
                                    "org.bluez",
                                    command_characteristic,        
                                    "org.bluez.GattCharacteristic1"
                                );
        
        // Write the packet to the pairing characteristic
        characteristic_proxy->call_sync("WriteValue",params_variant);
    }
    catch (const Glib::Error& e) {
        std::cerr << "Failed to send packet: " << e.what() << std::endl;
    }
}

void onDeviceAdded(const Glib::RefPtr<Gio::DBus::Connection>& connection,
                   const Glib::ustring& sender_name,
                   const Glib::ustring& object_path,
                   const Glib::ustring& interface_name,
                   const Glib::ustring& signal_name,
                   const Glib::VariantContainerBase& parameters) {
    try {
     

        // Print the type of parameters
       // std::cout << "Type of parameters: " << parameters.get_type().get_string() << std::endl;

        parameters.print(true);
        auto tuple = Glib::VariantBase::cast_dynamic<Glib::Variant<std::tuple<
                        Glib::ustring, // Object path (o)
                        std::map<Glib::ustring, std::map<Glib::ustring, Glib::VariantBase>> // a{sa{sv}}
                        >>>(parameters);

        // Extract the object path and interface map
        auto [object_path, interfaces] = tuple.get();
        

//        std::cout << "Object path: " << object_path << std::endl;

        // Check if the device has the desired "Name" property
        auto device_it = interfaces.find("org.bluez.Device1");
        if (device_it != interfaces.end()) {
            auto properties = device_it->second;

            auto name_it = properties.find("Name");
            if (name_it != properties.end()) {
                auto name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(name_it->second).get();
                 std::cout << "Name: " << name << std::endl;
                if (name == "telink_mesh1") {
                    std::cout << "Found device: " << name << " at " << object_path << std::endl;

                    // Step 1: Stop scanning
                    stopDeviceDiscovery();

                    // Step 2: Connect to the device
                    connectToDevice(object_path);

                    // wait for service info to populate
                    std::this_thread::sleep_for(std::chrono::seconds(2));

                    // Step 3: Pair with the device
                    pairWithDevice(object_path);

                    // Step 4: Subscribe to notifications
                    subscribeToNotifications(object_path);

                    send_packet(object_path,0xFFFF,0xE0,std::vector<uint8_t>({0xFF,0xFF}));
                }
            }
        }

        // Iterate over interfaces
        /*for (const auto& [interface_name, properties] : interfaces) {
            std::cout << "Interface: " << interface_name << std::endl;

            for (const auto& [key, value] : properties) {
                std::cout << "  " << key << ": " << value.print() << std::endl;
            }
        }*/

    } catch (const Glib::Error& e) {
        std::cerr << "Error processing device added: " << e.what() << std::endl;
    }
   
}


void on_properties_changed2(const Glib::RefPtr<Gio::DBus::Connection>& connection,
                   const Glib::ustring& sender_name,
                   const Glib::ustring& object_path,
                   const Glib::ustring& interface_name,
                   const Glib::ustring& signal_name,
                   const Glib::VariantContainerBase& parameters) {
    try {
     

        std::cout << "Type of parameters: " << parameters.get_type().get_string() << std::endl;


        std::cout << object_path << " properties changed:"<< std::endl << parameters.print(true) << std::endl;
        auto tuple = Glib::VariantBase::cast_dynamic<Glib::Variant<std::tuple<
                        Glib::ustring, // Object path (o)
                        std::map<Glib::ustring, Glib::VariantBase> ,
                        std::vector<Glib::ustring>
                        >>>(parameters);

        // Extract the object path and property map
        auto [object_path, properties, extras] = tuple.get();
        
        auto value_it = properties.find("Value");
        if (value_it != properties.end())
        {
            // get the data
            //std::vector<uint8_t> packet = value_it->second.get_dynamic();
            auto packet = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<uint8_t>>>(value_it->second).get();

            auto macdata = std::vector<uint8_t>({0xDE, 0x5A, 0xF4, 0x38, 0xC1, 0xA4}); // TODO: use a parameter
            auto decrypted_packet = crypto::decrypt_packet(shared_key,macdata,packet);

            crypto::print_hex("Decrypted packet rx:",decrypted_packet);
        }
        else
        {
            std::cout << "Not a data value, skipping..." << std::endl;
        }

    } catch (const Glib::Error& e) {
        std::cerr << "Error processing property change " << e.what() << std::endl;
    }

   
}

    
//     // Callback when a new device is added (discovered)
//   void onDeviceAdded(const Glib::RefPtr<Gio::DBus::Connection>& connection,
//                    const Glib::ustring& sender_name,
//                    const Glib::ustring& object_path,
//                    const Glib::ustring& interface_name,
//                    const Glib::ustring& signal_name,
//                    const Glib::VariantContainerBase& parameters) {
//     try {

//         std::cout << "onDeviceAdded " << std::endl;

//         // Create a mutable copy of parameters to call non-const get_child
//         auto mutable_parameters = Glib::VariantContainerBase::cast_dynamic<Glib::VariantContainerBase>(parameters);
//         auto args = Glib::VariantBase::cast_dynamic<Glib::Variant<std::map<std::string, Glib::VariantBase>>>(mutable_parameters.get_child(0));
//         if (args) {
//             auto interfaces = args.get();
//             for (const auto& path : interfaces) {
//                 std::string devicePath = path.first;
//                 std::cout << "Discovered device at path: " << devicePath << std::endl;

//                 // You can now use the devicePath to interact with the device (like getting characteristics)
//                 BluetoothDevice device(devicePath);
//                 auto notificationProxy = device.getCharacteristicProxy("00010203-0405-0607-0809-0a0b0c0d1911");
//             }
//         } else {
//             std::cerr << "Error: Failed to cast argument to expected type!" << std::endl;
//         }
//     } catch (const Glib::Error& e) {
//         std::cerr << "Error processing device added: " << e.what() << std::endl;
//     }
// }




private:
    Glib::RefPtr<Gio::DBus::Connection> connection;
    Glib::RefPtr<Gio::DBus::Proxy> adapterProxy;         // Adapter for managing scanning, etc.
    Glib::RefPtr<Gio::DBus::Proxy> connected_device_proxy; // Currently connected device
    Glib::RefPtr<Gio::DBus::Proxy> notification_proxy;    // GATT characteristic proxy for notifications
    std::vector<uint8_t> shared_key;

};

int main() {

    Gio::init();

    BluetoothManager manager;

    // Create and run the main event loop to handle signals
    auto mainLoop = Glib::MainLoop::create();
    mainLoop->run();  // Start the loop that processes the incoming signals
    

    return 0;
}
