#include "bluezproxy.h"
#include <iostream>
#include <giomm/dbusproxy.h>
#include <giomm/dbusconnection.h>
#include <glib.h>

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "BluezProxy"
#define G_LOG_USE_STRUCTURED 1
BlueZProxy::BlueZProxy()
    {
    setup_dbus_proxy();
}

BlueZProxy::~BlueZProxy() {
    // Cleanup resources if needed
}

void BlueZProxy::setup_dbus_proxy() {
    connection = Gio::DBus::Connection::get_sync(Gio::DBus::BUS_TYPE_SYSTEM);
    if (!connection) {
        throw std::runtime_error("Failed to connect to the system D-Bus.");
    }

    // Create a proxy for the BlueZ adapter interface
    adapter_proxy_ = Gio::DBus::Proxy::create_sync(
        connection,
        "org.bluez",         // BlueZ service name
        "/org/bluez/hci0",   // Default adapter path
        "org.bluez.Adapter1" // BlueZ Adapter interface
    );
    
}

void BlueZProxy::start_scan(sigc::slot<void,const std::string, const std::string> callback)
{    
    
    try {
        // Listen for DeviceAdded signals (new devices discovered) before starting the scan
       connection->signal_subscribe(
            sigc::mem_fun(this, &BlueZProxy::on_interfaces_added), // Slot for the callback
            "org.bluez",         // Sender name (can be empty if not filtering by sender)
            "org.freedesktop.DBus.ObjectManager", // Interface name
            "InterfacesAdded",   // Signal member (signal name)
            {},        // Object path (can be empty if not filtering by path)
            {}                  // First argument to filter by (optional, typically empty)            
        );
        
        auto result = adapter_proxy_->call_sync("StartDiscovery", Glib::VariantContainerBase());

        // Handle the returned value if any
        if (!result.gobj()) { // Check if the result is empty
            g_warning("No response received from StartDiscovery call.");
        } else {
            g_message("Started scanning for devices successfully.");
        }

        sigDeviceFound.connect(callback);

    } catch (const Glib::Error& e) {
        g_warning("Error starting device discovery: %s",e.what().c_str());
    }

}

void BlueZProxy::start_rssi_scan(sigc::slot<void,std::shared_ptr<Device>> callback)
{    
    
    try {
        // Listen for DeviceAdded signals (new devices discovered) before starting the scan
       connection->signal_subscribe(
            sigc::mem_fun(this, &BlueZProxy::on_device_properties_changed), // Slot for the callback
            "org.bluez",         // Sender name (can be empty if not filtering by sender)
            "org.freedesktop.DBus.Properties", // Interface name
            "PropertiesChanged",   // Signal member (signal name)
            {},        // Object path (can be empty if not filtering by path)
            {}                  // First argument to filter by (optional, typically empty)            
        );

        sigDeviceFoundByRSSI.connect(callback);
        
        auto result = adapter_proxy_->call_sync("StartDiscovery", Glib::VariantContainerBase());

        // Handle the returned value if any
        if (!result.gobj()) { // Check if the result is empty
            g_warning("No response received from StartDiscovery call.");
        } else {
            g_message("Started scanning for devices successfully.");
        }        

    } catch (const Glib::Error& e) {
        g_warning("Error starting device discovery: %s",e.what().c_str());   
    }

}

void BlueZProxy::stop_scan()
{
    try
    {
        adapter_proxy_->call_sync("StopDiscovery");
        sigDeviceFound.clear();
        sigDeviceFoundByRSSI.clear();
    } catch (const Glib::Error& e) {
        g_warning("Glib::Error: %s", e.what().c_str());
    } catch (const std::exception& e) {
        g_warning("Standard exception: %s", e.what());
    } catch (...) {
        g_warning("Unknown error");
    }    
}

bool BlueZProxy::connect(const std::string& device_address) {
    try {
        auto device_proxy_ = get_device_proxy(device_address);
        
        if (!device_proxy_) {
            g_warning("Failed to create proxy for device: %s", device_address.c_str());
            return false;
        }

        // Call BlueZ Device1's `Connect` method
        auto method_call = device_proxy_->call_sync("Connect");

        if (!method_call) {
            g_warning("Failed to connect to device: %s", device_address.c_str());
            return false;
        }

        g_message("Successfully connected to device: %s", device_address.c_str());
        return true;
    } catch (const Glib::Error& e) {
        g_warning("Glib::Error occurred while connecting to device %s: %s", device_address.c_str(), e.what().c_str());
    } catch (const std::exception& e) {
        g_warning("Standard exception occurred while connecting to device %s: %s", device_address.c_str(), e.what());
    } catch (...) {
        g_warning("Unknown error occurred while connecting to device: %s", device_address.c_str());
    }

    return false;
}

void BlueZProxy::disconnect(const std::string& device_address)
{
    try {
        auto device_proxy_ = get_device_proxy(device_address);
        
        if (!device_proxy_) {
            g_warning("Failed to create proxy for device: %s", device_address.c_str());
            return;
        }

        // Call BlueZ Device1's `Connect` method
        auto method_call = device_proxy_->call_sync("Disconnect");

        if (!method_call) {
            g_warning("Error while diconnecting %s", device_address.c_str());
            return;
        }

        g_message("Successfully disconnected device: %s", device_address.c_str());        
    } catch (const Glib::Error& e) {
        g_warning("Glib::Error occurred while disconnecting device %s: %s", device_address.c_str(), e.what().c_str());
    } catch (const std::exception& e) {
        g_warning("Standard exception occurred while disconnecting device %s: %s", device_address.c_str(), e.what());
    } catch (...) {
        g_warning("Unknown error occurred while disconnecting device: %s", device_address.c_str());
    }    
}

bool BlueZProxy::write(const std::string& device_address,const std::string& write_char_uuid,const std::vector<uint8_t>& payload)
{
    try {
        auto device_path = get_device_path(device_address);
        auto write_char_proxy = get_char_proxy(device_path,write_char_uuid);

    // Prepare options dictionary
        std::map<Glib::ustring, Glib::VariantBase> options;
        options["type"] = Glib::Variant<Glib::ustring>::create("request");
        auto options_variant = Glib::Variant< std::map<Glib::ustring, Glib::VariantBase>>::create(options);
        // Create a Glib::Variant containing the byte array
        auto value_variant = Glib::Variant<std::vector<uint8_t>>::create(payload);
        auto params_variant = Glib::VariantContainerBase::create_tuple(std::vector<Glib::VariantBase>({value_variant,options_variant}));
            
        // Write the payload to the characteristic
        write_char_proxy->call_sync("WriteValue",params_variant);
        return true;
    } catch (const Glib::Error& e) {
        g_warning("Glib::Error occurred while writing to device %s: %s", device_address.c_str(), e.what().c_str());
    } catch (const std::exception& e) {
        g_warning("Standard exception occurred while writing to device %s: %s", device_address.c_str(), e.what());
    } catch (...) {
        g_warning("Unknown error occurred while writing to device: %s", device_address.c_str());
    }
    return false;
}

std::vector<uint8_t> BlueZProxy::read(const std::string& device_address,const std::string& read_char_uuid)
{
        
    auto device_path = get_device_path(device_address);
    auto read_char_proxy = get_char_proxy(device_path,read_char_uuid);

    std::map<Glib::ustring, Glib::VariantBase> read_options;
    auto read_options_variant = Glib::Variant< std::map<Glib::ustring, Glib::VariantBase>>::create(read_options);
    
    auto response_variant = read_char_proxy->call_sync("ReadValue",Glib::VariantContainerBase::create_tuple(read_options_variant));    
    auto response_data_variant = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<uint8_t>>>(response_variant.get_child(0));
    return response_data_variant.get();   
}

void BlueZProxy::start_notify(const std::string& device_address,const std::string& notify_char_uuid,sigc::slot<void,const std::vector<uint8_t>> callback)
{
    auto device_path = get_device_path(device_address);
    auto notify_char_proxy = get_char_proxy(device_path,notify_char_uuid);
    auto notify_char_path = get_char_path(device_path,notify_char_uuid);
   
    connection->signal_subscribe(
        sigc::mem_fun(this, &BlueZProxy::on_data), // Slot for the callback
        "org.bluez",         // Sender name (can be empty if not filtering by sender)
        "org.freedesktop.DBus.Properties", // Interface name
        "PropertiesChanged",   // Signal member (signal name)
        notify_char_path,        // Object path (can be empty if not filtering by path)
        {}                  // First argument to filter by (optional, typically empty)            
    );

    sigDataRx.connect(callback);

    // Enable notifications
    notify_char_proxy->call_sync("StartNotify");
               
}

void BlueZProxy::disconnect_by_name(const std::string& device_name) {
    try {        
        auto om_proxy = Gio::DBus::Proxy::create_sync(
            connection,
            "org.bluez",                 // BlueZ service
            "/",                 // Specific device path
            "org.freedesktop.DBus.ObjectManager" // Interface for managed objects
        );
        // Get managed objects
        auto result = om_proxy->call_sync("GetManagedObjects");

        //std::cout << result.get_type_string() << std::endl;
        // Unpack the variant containing the managed objects
        auto objects = Glib::VariantBase::cast_dynamic<
                                Glib::Variant<
                                std::map<Glib::DBusObjectPathString,
                                    std::map<Glib::ustring,
                                        std::map<Glib::ustring,
                                                Glib::VariantBase>>>>>
                            (result.get_child(0));

        // Iterate through all objects managed by the adapter
        for (const auto& [object_path, interfaces] : objects.get()) {
            if (interfaces.find("org.bluez.Device1") != interfaces.end()) {
                auto device_proxy = Gio::DBus::Proxy::create_sync(
                    connection,
                    "org.bluez",
                    object_path,
                    "org.bluez.Device1"
                );

                Glib::Variant<Glib::ustring> name_variant;
                Glib::Variant<bool> connected_variant;
                device_proxy->get_cached_property(name_variant,"Name");                
                device_proxy->get_cached_property(connected_variant,"Connected");
                if (name_variant && Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(name_variant).get()==device_name
                    && connected_variant && Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(connected_variant).get()) {
                    // Call Disconnect method
                    device_proxy->call_sync("Disconnect");
                    g_message("Disconnected device: %s", object_path.c_str());
                }
            }
        }
    } catch (const Glib::Error& e) {
        g_warning("Error while disconnecting devices: %s", e.what().c_str());
    } catch (const std::exception& e) {
        g_warning("Standard exception while disconnecting devices: %s", e.what());
    } catch (...) {
        g_warning("Unknown error while disconnecting devices");
    }
}


void BlueZProxy::on_interfaces_added(const Glib::RefPtr<Gio::DBus::Connection>& connection,
                   const Glib::ustring& sender_name,
                   const Glib::ustring& object_path,
                   const Glib::ustring& interface_name,
                   const Glib::ustring& signal_name,
                   const Glib::VariantContainerBase& parameters) {
    
         
    auto tuple = Glib::VariantBase::cast_dynamic<Glib::Variant<std::tuple<
                    Glib::ustring, // Object path (o)
                    std::map<Glib::ustring,
                    std::map<Glib::ustring,
                    Glib::VariantBase>> // a{sa{sv}}
                    >>>(parameters);

    // Extract the object path and interface map
    auto [obj_path, interfaces] = tuple.get();

    g_debug("Interfaces added on %s",obj_path.c_str());
        
    // Check if the device has the desired "Name" property
    auto device_it = interfaces.find("org.bluez.Device1");
    if (device_it != interfaces.end()) {
        auto properties = device_it->second;

        Glib::ustring name,address;

        auto name_it = properties.find("Name");
        if (name_it != properties.end()) {
            name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(name_it->second).get();          
        }

        auto address_it = properties.find("Address");
        if (address_it != properties.end()) {
            address = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(address_it->second).get();          
        }

        sigDeviceFound.emit(address,name);
    }
}

void BlueZProxy::on_device_properties_changed(const Glib::RefPtr<Gio::DBus::Connection>& connection,
                             const Glib::ustring& sender_name,
                             const Glib::ustring& object_path,
                             const Glib::ustring& interface_name,
                             const Glib::ustring& signal_name,
                             const Glib::VariantContainerBase& parameters)
{   
    auto tuple = Glib::VariantBase::cast_dynamic<Glib::Variant<std::tuple<
                        Glib::ustring, // Object path (o)
                        std::map<Glib::ustring, Glib::VariantBase> ,
                        std::vector<Glib::ustring>
                        >>>(parameters);

    auto [o, properties, extras] = tuple.get();

    auto value_it = properties.find("RSSI");
    if (value_it != properties.end())
    {
        // get the data            
        auto data = Glib::VariantBase::cast_dynamic<Glib::Variant<std::int16_t>>(value_it->second).get();            
        Glib::DBusObjectPathString obj_path(object_path);
        auto device_info = get_device_info(obj_path);
        sigDeviceFoundByRSSI.emit(device_info);
    }
     // Signal format: a{sv} (Dictionary of property names to values)
    /*Glib::Variant<std::map<Glib::ustring, Glib::VariantBase>> props;
    Glib::Variant<std::vector<Glib::ustring>> invalidated_props;

    parameters.get_child(props, 0);  // First argument: changed properties
    parameters.get_child(invalidated_props, 1);  // Second argument: invalidated properties

    auto changed_props = props.get();

    if (changed_props.find("RSSI") != changed_props.end()) {
        auto rssi_value = changed_props.at("RSSI");
        //Glib::ustring device_path = proxy->get_object_path();
        //rssi_values[device_path] = rssi_value;
        std::cout << "RSSI for " << object_path << ": " << rssi_value << std::endl;    
    }*/
}


void BlueZProxy::on_data(const Glib::RefPtr<Gio::DBus::Connection>& connection,
             const Glib::ustring& sender_name,
             const Glib::ustring& object_path,
             const Glib::ustring& interface_name,
             const Glib::ustring& signal_name,
             const Glib::VariantContainerBase& parameters) {
    try {
             
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
            auto data = Glib::VariantBase::cast_dynamic<Glib::Variant<std::vector<uint8_t>>>(value_it->second).get();

            sigDataRx.emit(data);          
        }
        

    } catch (const Glib::Error& e) {
        std::cerr << "Error processing property change " << e.what() << std::endl;
    }

   
}

Glib::RefPtr<Gio::DBus::Proxy> BlueZProxy::get_device_proxy(const Glib::DBusObjectPathString& device_path)
{
    auto device_proxy = Gio::DBus::Proxy::create_sync(
        connection,
        "org.bluez",        // BlueZ service name
        device_path,        // Device object path
        "org.bluez.Device1" // BlueZ Device interface
    );

    return device_proxy;
}

Glib::RefPtr<Gio::DBus::Proxy> BlueZProxy::get_device_proxy(const std::string& device_address)
{   
    return get_device_proxy(get_device_path(device_address));
}

void get_device_info(const Glib::DBusObjectPathString &device_path);

Glib::DBusObjectPathString BlueZProxy::get_device_path(const std::string& device_address)
{
    auto device_path = "/org/bluez/hci0/dev_" + device_address;
    std::replace(device_path.begin(), device_path.end(), ':', '_'); // Format device path
    return device_path;
}

Glib::RefPtr<Gio::DBus::Proxy> BlueZProxy::get_char_proxy(const Glib::ustring& device_path,const std::string& char_uuid)
{
    auto char_path = get_char_path(device_path,char_uuid);

    if (char_path.empty()) {
        throw std::invalid_argument("Characteristic not found");
    }

     // Create the proxy for the pairing characteristic using its path
    auto char_proxy = Gio::DBus::Proxy::create_sync(
                            connection,
                            "org.bluez",
                            char_path,        
                            "org.bluez.GattCharacteristic1"
    );

    return char_proxy;
}

Glib::ustring BlueZProxy::get_char_path(const Glib::ustring& device_path,const std::string& target_uuid) {


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
                            g_debug("Found GATT characteristic path: %s",path.c_str());
                            return path;
                        }
                    }
                }
            }
        }

    // If not found, throw an exception
    g_warning("Characteristic with UUID %s not found", target_uuid.c_str());
    return Glib::ustring ();
}


std::shared_ptr<BlueZProxy::Device> BlueZProxy::get_device_info(const Glib::DBusObjectPathString &device_path) {
    
    auto proxy = Gio::DBus::Proxy::create_sync(
        connection,
        "org.bluez",                 // BlueZ service
        device_path,                 // Specific device path
        "org.freedesktop.DBus.Properties" // Interface for managed objects
    );

    auto interface_variant = Glib::Variant<Glib::ustring>::create("org.bluez.Device1");
    auto result_variant = proxy->call_sync("GetAll",Glib::VariantContainerBase::create_tuple({interface_variant}));


    auto tuple = Glib::VariantBase::cast_dynamic<Glib::Variant<std::tuple<
                                                std::map<Glib::ustring, Glib::VariantBase>>>>(result_variant);

    auto [properties] = tuple.get();
    

    // Create the Device structure
    auto device_info = std::make_shared<BlueZProxy::Device>();

    // Extract and map each property into the Device structure
    try {
                // Check and extract each property
        if (properties.find("Address") != properties.end()) {
            device_info->Address = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(properties.at("Address")).get();
        }

        if (properties.find("AddressType") != properties.end()) {
            device_info->AddressType = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(properties.at("AddressType")).get();
        }

        if (properties.find("Name") != properties.end()) {
            device_info->Name = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(properties.at("Name")).get();
        }

        if (properties.find("Alias") != properties.end()) {
            device_info->Alias = Glib::VariantBase::cast_dynamic<Glib::Variant<Glib::ustring>>(properties.at("Alias")).get();
        }

        if (properties.find("Paired") != properties.end()) {
            device_info->Paired = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(properties.at("Paired")).get();
        }

        if (properties.find("Bonded") != properties.end()) {
            device_info->Bonded = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(properties.at("Bonded")).get();
        }

        if (properties.find("Trusted") != properties.end()) {
            device_info->Trusted = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(properties.at("Trusted")).get();
        }

        if (properties.find("Blocked") != properties.end()) {
            device_info->Blocked = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(properties.at("Blocked")).get();
        }

        if (properties.find("LegacyPairing") != properties.end()) {
            device_info->LegacyPairing = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(properties.at("LegacyPairing")).get();
        }

        if (properties.find("RSSI") != properties.end()) {
            device_info->RSSI = Glib::VariantBase::cast_dynamic<Glib::Variant<int16_t>>(properties.at("RSSI")).get();
        }

        if (properties.find("Connected") != properties.end()) {
            device_info->Connected = Glib::VariantBase::cast_dynamic<Glib::Variant<bool>>(properties.at("Connected")).get();
        }   
    } catch (const Glib::Error &e) {
        std::cerr << "Error while extracting device properties: " << e.what() << std::endl;
        return nullptr;  // Return nullptr if any error occurs
    }

    // Return the populated device info
    return device_info;
    
}