#ifndef TELINK_MESH_PROTOCOL_H
#define TELINK_MESH_PROTOCOL_H

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <memory> 
#include <cstring> 
#include <endian.h>

#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "Mesh protocol"

#define MAX_PACKET_SIZE 20

class TelinkMeshProtocol {
public:

 // Enums for commands
    enum __attribute__((packed)) Command : uint8_t
    {
        //Mesh commands
        COMMAND_OTA_UPDATE = 0xC6,
        COMMAND_QUERY_OTA_STATE = 0xC7,
        COMMAND_OTA_STATUS_REPORT = 0xC8,
        COMMAND_GROUP_ID_QUERY = 0xDD,
        COMMAND_GROUP_ID_REPORT = 0xD4,
        COMMAND_GROUP_EDIT = 0xD7,
        COMMAND_ONLINE_STATUS_REPORT = 0xDC,
        COMMAND_ADDRESS_EDIT = 0xE0,
        COMMAND_ADDRESS_REPORT = 0xE1,
        COMMAND_RESET = 0xE3,
        COMMAND_TIME_QUERY = 0xE8,
        COMMAND_TIME_REPORT = 0xE9,
        COMMAND_TIME_SET = 0xE4,
        COMMAND_DEVICE_INFO_QUERY = 0xEA,
        COMMAND_DEVICE_INFO_REPORT = 0xEB,

        // Light commands
        COMMAND_SCENARIO_QUERY = 0xC0,
        COMMAND_SCENARIO_REPORT = 0xC1,
        COMMAND_SCENARIO_LOAD = 0xF2,
        COMMAND_SCENARIO_EDIT = 0xF3,
        COMMAND_STATUS_QUERY = 0xDA,
        COMMAND_STATUS_REPORT = 0xDB,
        COMMAND_ALARM_QUERY = 0xE6,
        COMMAND_ALARM_REPORT = 0xE7,
        COMMAND_ALARM_EDIT = 0xE5,
        COMMAND_LIGHT_ON_OFF = 0xF0,
        COMMAND_LIGHT_ATTRIBUTES_SET = 0xF1
    };

protected:
        // Define payload structs for specific commands
    struct __attribute__((packed)) LightAttributesPayload {
        uint8_t brightness;
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        uint8_t yellow;
        uint8_t white;
        uint8_t music_mode;
        uint8_t control_flag; // TODO: not sure how this works
    };

    struct __attribute__((packed)) DeviceInfoPayload {
        /*uint8_t firmwareVersion;
        uint16_t deviceID;
        uint8_t capabilities;*/
    };

      
    struct __attribute__((packed)) AddressReportPayload {
        uint8_t nodeID;
        uint8_t mac[6];        
    };

    struct __attribute__((packed)) GroupIDReportPayload {        
        uint8_t groups[10];        
    };

    struct __attribute__((packed)) OnlineStatusReportPayload {
        uint8_t nodeId;
        uint8_t reserved;
        uint8_t brightness;
        uint8_t state;
    };

    struct __attribute__((packed)) StatusReportPayload {                
        uint8_t brightness;
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        uint8_t yellow; // ?
        uint8_t white;
        uint8_t music_mode; // ?
        uint8_t control_flag; // ?
    };

    struct __attribute__((packed)) TimeReportPayload {
        uint16_t year;
        uint8_t month;
        uint8_t day;
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
    };

    // Define a union to represent the payload
    union __attribute__((packed)) Payload {
        OnlineStatusReportPayload onlineStatusReport;
        DeviceInfoPayload deviceInfo;
        LightAttributesPayload lightAttributes;
        StatusReportPayload statusReport;
        uint8_t lightOnOff;        
        uint16_t addressEditMode;
        AddressReportPayload addressReport;
        uint16_t groupIDQueryMode;
        GroupIDReportPayload groupIDReport;
        TimeReportPayload timeReport;
        uint8_t statusQueryMode;
        uint8_t raw[MAX_PACKET_SIZE-4]; // Raw data for generic access
    };

    // Packet structure
    struct __attribute__((packed)) Packet  {
        union
        {
            struct __attribute__((packed)){
                union {
                    struct __attribute__((packed)){ // TX
            /*[  0-1]*/uint16_t seq; // packet sequence number
            /*[  2-4]*/uint8_t reserved[3];
            /*[  5-6]*/uint16_t dest_node;  // destination mesh node (0=connected node, 0xFFFF=entire mesh)
            /*[    7]*/Command command;
            /*[  8-9]*/uint16_t vendor_code;
                    };
                    struct __attribute__((packed)){ // RX
            /*[  0-2]*/uint8_t rsv_0_2[3];
            /*[    3]*/uint8_t src_node;
            /*[  4-6]*/uint8_t rsv_4_6[3];
            /*[    7]*/Command type;
            /*[  8-9]*/uint8_t rsv_8_9[2];                 
                    };
                };
    /*[10-19]*/Payload payload;
            };
            uint8_t data[MAX_PACKET_SIZE];
        };

    }; 

public:
    class TelinkMeshPacket
    {
        public:

             // Virtual destructor to ensure proper cleanup when derived class is destroyed
            virtual ~TelinkMeshPacket()  {};

            // Inline getter and setter for seq
            uint16_t getSeq() const { return le16toh(packet.seq); }
            void setSeq(uint16_t value) { packet.seq = htole16(value); }
            

            // Inline getter and setter for dest_node
            uint16_t getDestNode() const { return le16toh(packet.dest_node); }
            void setDestNode(uint16_t value) { packet.dest_node = htole16(value); }

            // Inline getter and setter for source node
            uint8_t getSrcNode() const { return le16toh(packet.src_node); }
            void setSrcNode(uint8_t value) { packet.src_node = htole16(value); }

            // Inline getter and setter for vendor_code
            uint16_t getVendorCode() const { return le16toh(packet.vendor_code); }
            void setVendorCode(uint16_t value) { packet.vendor_code = htole16(value); }

            // Inline getter and setter for dest_node
            Command getCommand() const { return packet.command; }

            virtual void debug() const {
                g_debug("TelinkMeshPacket: seq=%u, dest_node=%u, src_node=%u command=0x%02X, vendor_code=%u",
                getSeq(), getDestNode(),getSrcNode(), static_cast<uint8_t>(getCommand()), getVendorCode());
            }


            static std::shared_ptr<TelinkMeshPacket> create(const std::vector<uint8_t>& data)
            {
                if (data.size() != 20) {
                    throw std::invalid_argument("Data must be 20 bytes");
                }

                // Get the command byte from the data 
                Command cmd = static_cast<Command>(data[7]);

                // Create the appropriate subclass based on the command byte
                switch (cmd)
                {
                    case Command::COMMAND_ADDRESS_REPORT:
                        return std::make_shared<TelinkMeshAddressReport>(data);
                    case Command::COMMAND_ONLINE_STATUS_REPORT:
                        return std::make_shared<TelinkMeshOnlineStatusReport>(data);
                    case Command::COMMAND_STATUS_REPORT:
                        return std::make_shared<TelinkLightStatusReport>(data);
                    case Command::COMMAND_GROUP_ID_REPORT:
                        return std::make_shared<TelinkMeshGroupIDReport>(data);
                    case Command::COMMAND_DEVICE_INFO_REPORT:
                        return std::make_shared<TelinkMeshDeviceInfoReport>(data);
                    case Command::COMMAND_TIME_REPORT:
                        return std::make_shared<TelinkMeshTimeReport>(data);                    
                    default:
                        g_warning("Cannot decode mesh command type 0x%02X",cmd);
                        throw std::runtime_error("Unexpected mesh command type");
                }
            }

            std::vector<uint8_t> getData() const {                
                std::vector<uint8_t> data(packet.data,packet.data+sizeof(packet.data));
                return data;
            }


        protected:

            TelinkMeshPacket(TelinkMeshProtocol::Command command)
            {
                packet.command=command;
            };
            
            TelinkMeshPacket(const std::vector<uint8_t>& data) {
                // General constructor to fill the packet with data bytes
                if (data.size() <= MAX_PACKET_SIZE ) {                    
                    std::copy(data.begin(), data.end(), packet.data);
                } else {
                    throw std::runtime_error("Insufficient data provided");
                }
            }            

            Packet packet = {};

    };


    class TelinkLightStatusQuery : public TelinkMeshPacket
    {
        public:
            TelinkLightStatusQuery() : TelinkMeshPacket(Command::COMMAND_STATUS_QUERY){};
            
            uint8_t getMode() const { return packet.payload.statusQueryMode; }
            void setMode(uint8_t value) { packet.payload.statusQueryMode = value; }

            void debug() const override {
                TelinkMeshPacket::debug();
                g_debug("TelinkMeshStatusQuery: mode=%u", packet.payload.statusQueryMode);
            }

    };

    class TelinkMeshAddressEdit : public TelinkMeshPacket
    {
        public:
            TelinkMeshAddressEdit() : TelinkMeshPacket(Command::COMMAND_ADDRESS_EDIT){};
            
            uint16_t getMode() const { return le16toh(packet.payload.addressEditMode); }
            void setMode(uint16_t value) { packet.payload.addressEditMode = htole16(value); }

            void debug() const override {
                TelinkMeshPacket::debug();
                g_debug("TelinkMeshAddressEdit: mode=%u", le16toh(packet.payload.addressEditMode));
            }

    };

    class TelinkMeshAddressReport : public TelinkMeshPacket
    {
        public:
            TelinkMeshAddressReport() : TelinkMeshPacket(Command::COMMAND_ADDRESS_REPORT){};
            
            uint8_t getNodeID() const { return le16toh(packet.payload.addressReport.nodeID); }
            void setNodeID(uint8_t value) { packet.payload.addressReport.nodeID = htole16(value); }

     
            std::vector<uint8_t> getMAC() const {
                return std::vector<uint8_t>(packet.payload.addressReport.mac, packet.payload.addressReport.mac + 6);
            }
    
            void setMAC(const std::vector<uint8_t>& mac) {
                if (mac.size() == 6) {
                    std::memcpy(packet.payload.addressReport.mac, mac.data(), 6);
                } else {
                    // Handle error, MAC address must be 6 bytes
                    std::cerr << "Invalid MAC address size." << std::endl;
                }
            }

            void debug() const override {
                TelinkMeshPacket::debug();
                g_debug("TelinkMeshAddressReport: nodeID=%u, mac=%02X:%02X:%02X:%02X:%02X:%02X",
                        getNodeID(),
                        packet.payload.addressReport.mac[0],
                        packet.payload.addressReport.mac[1],
                        packet.payload.addressReport.mac[2],
                        packet.payload.addressReport.mac[3],
                        packet.payload.addressReport.mac[4],
                        packet.payload.addressReport.mac[5]);
            }

        
            TelinkMeshAddressReport(const std::vector<uint8_t>& data): TelinkMeshPacket(data) {}
    };

    class TelinkLightSetAttributes : public TelinkMeshPacket
    {
        public:
            TelinkLightSetAttributes() : TelinkMeshPacket(Command::COMMAND_LIGHT_ATTRIBUTES_SET){};

            uint8_t get_brightness() const { return packet.payload.lightAttributes.brightness; }
            void set_brightness(uint8_t value) { packet.payload.lightAttributes.brightness = value; }

            uint8_t get_red() const { return packet.payload.lightAttributes.red; }
            void set_red(uint8_t value) { packet.payload.lightAttributes.red = value; }

            uint8_t get_green() const { return packet.payload.lightAttributes.green; }
            void set_green(uint8_t value) { packet.payload.lightAttributes.green = value; }

            uint8_t get_blue() const { return packet.payload.lightAttributes.blue; }
            void set_blue(uint8_t value) { packet.payload.lightAttributes.blue = value; }

            uint8_t get_yellow() const { return packet.payload.lightAttributes.yellow; }
            void set_yellow(uint8_t value) { packet.payload.lightAttributes.yellow = value; }

            uint8_t get_white() const { return packet.payload.lightAttributes.white; }
            void set_white(uint8_t value) { packet.payload.lightAttributes.white = value; }

            uint8_t get_music_mode() const { return packet.payload.lightAttributes.music_mode; }
            void set_music_mode(uint8_t value) { packet.payload.lightAttributes.music_mode = value; }

            uint8_t get_control_flag() const { return packet.payload.lightAttributes.control_flag; }
            void set_control_flag(uint8_t value) { packet.payload.lightAttributes.control_flag = value; }

            void debug() const override {
                TelinkMeshPacket::debug();
                g_debug("TelinkLightSetAttributes: brightness=%u, red=%u, green=%u, blue=%u, "
                "yellow=%u, white=%u, music_mode=%u, control_flag=%u",
                get_brightness(), get_red(), get_green(), get_blue(),
                get_yellow(), get_white(), get_music_mode(), get_control_flag());
            }

    };

    class TelinkLightStatusReport : public TelinkMeshPacket
    {
        public:
            TelinkLightStatusReport() : TelinkMeshPacket(Command::COMMAND_STATUS_REPORT){};

            uint8_t get_brightness() const { return packet.payload.statusReport.brightness; }
            void set_brightness(uint8_t value) { packet.payload.statusReport.brightness = value; }

            uint8_t get_red() const { return packet.payload.statusReport.red; }
            void set_red(uint8_t value) { packet.payload.statusReport.red = value; }

            uint8_t get_green() const { return packet.payload.statusReport.green; }
            void set_green(uint8_t value) { packet.payload.statusReport.green = value; }

            uint8_t get_blue() const { return packet.payload.statusReport.blue; }
            void set_blue(uint8_t value) { packet.payload.statusReport.blue = value; }

            uint8_t get_yellow() const { return packet.payload.statusReport.yellow; }
            void set_yellow(uint8_t value) { packet.payload.statusReport.yellow = value; }

            uint8_t get_white() const { return packet.payload.statusReport.white; }
            void set_white(uint8_t value) { packet.payload.statusReport.white = value; }

            uint8_t get_music_mode() const { return packet.payload.statusReport.music_mode; }
            void set_music_mode(uint8_t value) { packet.payload.statusReport.music_mode = value; }

            uint8_t get_control_flag() const { return packet.payload.statusReport.control_flag; }
            void set_control_flag(uint8_t value) { packet.payload.statusReport.control_flag = value; }

            void debug() const override {
                TelinkMeshPacket::debug();
                g_debug("TelinkLightStatusReport: brightness=%u, red=%u, green=%u, blue=%u, "
                        "yellow=%u, white=%u, music_mode=%u, control_flag=%u",
                        packet.payload.statusReport.brightness,
                        packet.payload.statusReport.red,
                        packet.payload.statusReport.green,
                        packet.payload.statusReport.blue,
                        packet.payload.statusReport.yellow,
                        packet.payload.statusReport.white,
                        packet.payload.statusReport.music_mode,
                        packet.payload.statusReport.control_flag);
            }

        
            TelinkLightStatusReport(const std::vector<uint8_t>& data): TelinkMeshPacket(data) {}
    };

    class TelinkLightOnOff : public TelinkMeshPacket {
    public:
        TelinkLightOnOff() : TelinkMeshPacket(Command::COMMAND_LIGHT_ON_OFF) {}

        uint8_t get_on_off() const { return packet.payload.lightOnOff; }
        void set_on_off(uint8_t value) { packet.payload.lightOnOff = value; }
    };

    class TelinkMeshOnlineStatusReport : public TelinkMeshPacket
    {
        public:
            TelinkMeshOnlineStatusReport() : TelinkMeshPacket(Command::COMMAND_ONLINE_STATUS_REPORT) {};

            uint8_t getNodeID() const { return packet.payload.onlineStatusReport.nodeId; }
            void setNodeID(uint8_t value) { packet.payload.onlineStatusReport.nodeId = value; }

            uint8_t getReserved() const { return packet.payload.onlineStatusReport.reserved; }
            void setReserved(uint8_t value) { packet.payload.onlineStatusReport.reserved = value; }

            uint8_t getBrightness() const { return packet.payload.onlineStatusReport.brightness; }
            void setBrightness(uint8_t value) { packet.payload.onlineStatusReport.brightness = value; }

            uint8_t getState() const { return packet.payload.onlineStatusReport.state; }
            void setState(uint8_t value) { packet.payload.onlineStatusReport.state = value; }

            bool isLightOn() const { return !(getState() & 0x01); }

            void debug() const override {
                TelinkMeshPacket::debug();
                g_debug("TelinkMeshOnlineStatusReport: nodeID=%u, reserved=%u, brightness=%u, state=%u",
                        packet.payload.onlineStatusReport.nodeId,
                        packet.payload.onlineStatusReport.reserved,
                        packet.payload.onlineStatusReport.brightness,
                        packet.payload.onlineStatusReport.state);
            }

        
            TelinkMeshOnlineStatusReport(const std::vector<uint8_t>& data): TelinkMeshPacket(data) {}
    };

    class TelinkMeshTimeReport : public TelinkMeshPacket
    {
        public:
            TelinkMeshTimeReport() : TelinkMeshPacket(Command::COMMAND_TIME_REPORT) {};

            uint16_t getYear() const { return le16toh(packet.payload.timeReport.year); }
            void setYear(uint16_t value) { packet.payload.timeReport.year = htole16(value); }

            uint8_t getMonth() const { return packet.payload.timeReport.month; }
            void setMonth(uint8_t value) { packet.payload.timeReport.month = value; }

            uint8_t getDay() const { return packet.payload.timeReport.day; }
            void setDay(uint8_t value) { packet.payload.timeReport.day = value; }

            uint8_t getHour() const { return packet.payload.timeReport.hour; }
            void setHour(uint8_t value) { packet.payload.timeReport.hour = value; }

            uint8_t getMinute() const { return packet.payload.timeReport.minute; }
            void setMinute(uint8_t value) { packet.payload.timeReport.minute = value; }

            uint8_t getSecond() const { return packet.payload.timeReport.second; }
            void setSecond(uint8_t value) { packet.payload.timeReport.second = value; }

            void debug() const override {
                TelinkMeshPacket::debug();
                g_debug("TelinkMeshTimeReport: year=%u, month=%u, day=%u, hour=%u, minute=%u, second=%u",
                        le16toh(packet.payload.timeReport.year),
                        packet.payload.timeReport.month,
                        packet.payload.timeReport.day,
                        packet.payload.timeReport.hour,
                        packet.payload.timeReport.minute,
                        packet.payload.timeReport.second);
            }


            TelinkMeshTimeReport(const std::vector<uint8_t>& data): TelinkMeshPacket(data) {}
    };

    class TelinkMeshDeviceInfoReport : public TelinkMeshPacket
    {
        public:
            TelinkMeshDeviceInfoReport() : TelinkMeshPacket(Command::COMMAND_DEVICE_INFO_REPORT) {};        
            TelinkMeshDeviceInfoReport(const std::vector<uint8_t>& data): TelinkMeshPacket(data) {}
            
    };

    class TelinkMeshGroupIDQuery : public TelinkMeshPacket
    {
        public:
            TelinkMeshGroupIDQuery() : TelinkMeshPacket(Command::COMMAND_GROUP_ID_QUERY){};
            
            uint16_t getMode() const { return le16toh(packet.payload.groupIDQueryMode); }
            void setMode(uint16_t value) { packet.payload.groupIDQueryMode = htole16(value); }

            void debug() const override {
                TelinkMeshPacket::debug();
                g_debug("TelinkMeshGroupIDQuery: mode=%u", le16toh(packet.payload.groupIDQueryMode));
            }

    };

    class TelinkMeshGroupIDReport : public TelinkMeshPacket
    {
        public:
            TelinkMeshGroupIDReport() : TelinkMeshPacket(Command::COMMAND_GROUP_ID_REPORT) {};

            std::vector<uint8_t> getGroups() const {
                return std::vector<uint8_t>(packet.payload.groupIDReport.groups, 
                                             packet.payload.groupIDReport.groups + 10);
            }

            void setGroups(const std::vector<uint8_t>& groups) {
                if (groups.size() == 10) {
                    std::memcpy(packet.payload.groupIDReport.groups, groups.data(), 10);
                } else {
                    std::cerr << "Invalid group size." << std::endl;
                }
            }

            void debug() const override {
                TelinkMeshPacket::debug();
                g_debug("TelinkMeshGroupIDReport: groups=[%u, %u, %u, %u, %u, %u, %u, %u, %u, %u]",
                        packet.payload.groupIDReport.groups[0],
                        packet.payload.groupIDReport.groups[1],
                        packet.payload.groupIDReport.groups[2],
                        packet.payload.groupIDReport.groups[3],
                        packet.payload.groupIDReport.groups[4],
                        packet.payload.groupIDReport.groups[5],
                        packet.payload.groupIDReport.groups[6],
                        packet.payload.groupIDReport.groups[7],
                        packet.payload.groupIDReport.groups[8],
                        packet.payload.groupIDReport.groups[9]);
            }

        
            TelinkMeshGroupIDReport(const std::vector<uint8_t>& data): TelinkMeshPacket(data) {}
    };

    
   

    
};

#endif