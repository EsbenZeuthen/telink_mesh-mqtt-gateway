#ifndef _TELINK_GATT_PROFILE_H_
#define _TELINK_GATT_PROFILE_H_

#include <giomm.h>
#include <glibmm.h>
#include <iostream>

const std::string BLUEZ_BUS_NAME = "org.bluez";
const std::string BLUEZ_PROFILE_PATH = "/custom/gatt/profile";
const std::string BLUEZ_PROFILE_INTERFACE = "org.bluez.GattProfile1";

// UUID of the Telink service
const std::string TELINK_SERVICE_UUID = "00010203-0405-0607-0809-0a0b0c0d1910";

// UUID of the characteristics that need to be handled
const std::string TELINK_CHAR_UUID = "00010203-0405-0607-0809-0a0b0c0d1911";

class GattProfile : public Glib::ObjectBase {
public:
    GattProfile(const Glib::RefPtr<Gio::DBus::Connection>& connection)
        : conn(connection) {
        register_profile();
    }

private:
    Glib::RefPtr<Gio::DBus::Connection> conn;

    void register_profile() {
        try {
            // Define the interface vtable
         /*  Gio::DBus::InterfaceVTable vtable(
                sigc::mem_fun(*this, &GattProfile::on_method_call),  // Method handler
                sigc::slot<Gio::DBus::InterfaceVTable::SlotInterfaceGetProperty>(),  // No property getter
                sigc::slot<Gio::DBus::InterfaceVTable::SlotInterfaceSetProperty>()   // No property setter
            );

          
            // Register the object
             conn->register_object(BLUEZ_PROFILE_PATH, 
            { 
                {BLUEZ_PROFILE_INTERFACE, vtable} 
            },
            Gio::DBus::Connection::Flags()

        );*/

            // Call BlueZ to register the profile
            auto proxy = Gio::DBus::Proxy::create_sync(
                conn,
                BLUEZ_BUS_NAME,
                "/org/bluez",
                "org.bluez.GattManager1"
            );

            Glib::VariantContainerBase options = Glib::VariantDict()
                .insert_value("Service", Glib::Variant<std::string>(TELINK_SERVICE_UUID))
                .insert_value("Characteristic", Glib::Variant<std::string>(TELINK_CHAR_UUID))
                .insert_value("Name", Glib::Variant<std::string>("Telink Custom Profile"))
                .insert_value("Role", Glib::Variant<std::string>("client"))
                .create_tuple();

            proxy->call_sync("RegisterProfile", Glib::Variant<std::tuple<std::string, std::string, Glib::VariantContainerBase>>(
                std::make_tuple(BLUEZ_PROFILE_PATH, TELINK_SERVICE_UUID, options)
            ));

            std::cout << "Custom GATT profile registered successfully!\n";
        } catch (const Glib::Error& e) {
            std::cerr << "Failed to register profile: " << e.what() << std::endl;
        }
    }

    void on_method_call(
        const Glib::RefPtr<Gio::DBus::Connection>& connection,
        const Glib::ustring& sender,
        const Glib::ustring& object_path,
        const Glib::ustring& interface_name,
        const Glib::ustring& method_name,
        const Glib::VariantContainerBase& parameters,
        Glib::RefPtr<Gio::DBus::MethodInvocation>& invocation
    ) {
        if (method_name == "Release") {
            release();
            invocation->return_value();
        } else if (method_name == "NewConnection") {
            new_connection(parameters);
            invocation->return_value();
        } else if (method_name == "RequestDisconnection") {
            request_disconnection();
            invocation->return_value();
        }
    }


    void release() {
        std::cout << "GattProfile released by BlueZ" << std::endl;
    }

    void new_connection(const Glib::VariantContainerBase&) {
        std::cout << "New GATT connection established!" << std::endl;
    }

    void request_disconnection() {
        std::cout << "GATT connection disconnected" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    auto app = Glib::MainLoop::create();
    auto connection = Gio::DBus::Connection::get_sync(Gio::DBus::BusType::SYSTEM);
    
    if (!connection) {
        std::cerr << "Failed to connect to system D-Bus" << std::endl;
        return 1;
    }

    auto profile = std::make_shared<GattProfile>(connection);
    app->run();

    return 0;
}

#endif