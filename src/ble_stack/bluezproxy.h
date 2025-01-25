#ifndef BLUEZ_PROXY_H
#define BLUEZ_PROXY_H

#include <giomm.h>
#include <string>
#include <sigc++/sigc++.h>
#include <vector>

class BlueZProxy {
public:

    struct Device
    {
        std::string Address;
        std::string AddressType;
        std::string Name;
        std::string Alias;
        bool Paired;
        bool Bonded;
        bool Trusted;
        bool Blocked;
        bool LegacyPairing;
        int16_t RSSI;
        bool Connected;        
    };

    BlueZProxy();
    ~BlueZProxy();

    
    void start_scan(sigc::slot<void,const std::string, const std::string> callback);

    void start_rssi_scan(sigc::slot<void,std::shared_ptr<Device>> callback);

    void stop_scan();
    
    bool connect(const std::string& device_address);

    void disconnect_by_name(const std::string& device_name);

    bool write(const std::string& device_address,const std::string& write_char_uuid,const std::vector<uint8_t>& payload);

    std::vector<uint8_t> read(const std::string& device_address,const std::string& read_char_uuid);

    void start_notify(const std::string& device_address,const std::string& notify_char_uuid,sigc::slot<void,const std::vector<uint8_t>> callback);
    
protected:

    void on_interfaces_added(const Glib::RefPtr<Gio::DBus::Connection>& connection,
                             const Glib::ustring& sender_name,
                             const Glib::ustring& object_path,
                             const Glib::ustring& interface_name,
                             const Glib::ustring& signal_name,
                             const Glib::VariantContainerBase& parameters);

    void on_device_properties_changed(const Glib::RefPtr<Gio::DBus::Connection>& connection,
                             const Glib::ustring& sender_name,
                             const Glib::ustring& object_path,
                             const Glib::ustring& interface_name,
                             const Glib::ustring& signal_name,
                             const Glib::VariantContainerBase& parameters);

    void on_data(const Glib::RefPtr<Gio::DBus::Connection>& connection,
                 const Glib::ustring& sender_name,
                 const Glib::ustring& object_path,
                 const Glib::ustring& interface_name,
                 const Glib::ustring& signal_name,
                 const Glib::VariantContainerBase& parameters);
    
    // Helper methods
    Glib::RefPtr<Gio::DBus::Proxy> get_device_proxy(const Glib::DBusObjectPathString& device_path);
    Glib::RefPtr<Gio::DBus::Proxy> get_device_proxy(const std::string& device_address);
    std::shared_ptr<Device>  get_device_info(const Glib::DBusObjectPathString &device_path);
    Glib::DBusObjectPathString get_device_path(const std::string& device_address);
    Glib::RefPtr<Gio::DBus::Proxy> get_char_proxy(const Glib::ustring& device_path,const std::string& char_uuid);
    Glib::ustring get_char_path(const Glib::ustring& device_path,const std::string& target_uuid);
    void setup_dbus_proxy();    

    // Internal state
    Glib::RefPtr<Gio::DBus::Connection> connection;
    Glib::RefPtr<Gio::DBus::Proxy> adapter_proxy_;  

    // callback signals
    sigc::signal<void,std::shared_ptr<Device>> sigDeviceFoundByRSSI;
    sigc::signal<void,const std::string,const std::string> sigDeviceFound;
    sigc::signal<void,const std::vector<uint8_t>&> sigDataRx;

};

#endif // BLUEZ_PROXY_H
