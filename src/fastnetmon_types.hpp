#ifndef FASTNETMON_TYPES_H
#define FASTNETMON_TYPES_H

#include <netinet/in.h> // struct in6_addr
#include <stdint.h> // uint32_t
#include <sys/time.h> // struct timeval

#include <map>
#include <string>
#include <unordered_map>
#include <utility> // std::pair
#include <vector>

#include "packet_storage.hpp"

#include "fastnetmon_simple_packet.hpp"

#include "subnet_counter.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "fastnetmon_networks.hpp"

enum attack_severity_t { ATTACK_SEVERITY_LOW, ATTACK_SEVERITY_MIDDLE, ATTACK_SEVERITY_HIGH };


// Kafka traffic export formats
enum class kafka_traffic_export_format_t : uint32_t { Unknown = 0, JSON = 1, Protobuf = 2 };

typedef std::vector<subnet_counter_t> vector_of_counters_t;

typedef std::map<std::string, std::string> configuration_map_t;
typedef std::map<std::string, uint64_t> graphite_data_t;

// Enum with available sort by field
enum sort_type_t { PACKETS, BYTES, FLOWS };

// Source of attack detection
enum class attack_detection_source_t : uint32_t { Automatic = 1, Manual = 2, Other = 255 };

// Which direction of traffic triggered attack
enum class attack_detection_direction_type_t {
    unknown,
    incoming,
    outgoing,
};


// How we have detected this attack?
enum class attack_detection_threshold_type_t {
    unknown,

    packets_per_second,
    bytes_per_second,
    flows_per_second,

    tcp_packets_per_second,
    udp_packets_per_second,
    icmp_packets_per_second,

    tcp_bytes_per_second,
    udp_bytes_per_second,
    icmp_bytes_per_second,

    tcp_syn_packets_per_second,
    tcp_syn_bytes_per_second,
};

// Types of metrics as in Prometheus:
// https://prometheus.io/docs/concepts/metric_types/
enum class metric_type_t { counter, gauge };

// Here we store different counters
class system_counter_t {
    public:
    system_counter_t(const std::string& counter_name, uint64_t counter_value, const metric_type_t& metric_type, const std::string& description) {
        this->counter_name        = counter_name;
        this->counter_value       = counter_value;
        this->counter_type        = metric_type;
        this->counter_description = description;
    }

    std::string counter_name;
    uint64_t counter_value     = 0;
    metric_type_t counter_type = metric_type_t::counter;
    std::string counter_description;
};


/* Class for custom comparison fields by different fields */
template <typename T> class TrafficComparatorClass {
    private:
    sort_type_t sort_field;
    direction_t sort_direction;

    public:
    TrafficComparatorClass(direction_t sort_direction, sort_type_t sort_field) {
        this->sort_field     = sort_field;
        this->sort_direction = sort_direction;
    }

    bool operator()(T a, T b) {
        if (sort_field == FLOWS) {
            if (sort_direction == INCOMING) {
                return a.second.in_flows > b.second.in_flows;
            } else if (sort_direction == OUTGOING) {
                return a.second.out_flows > b.second.out_flows;
            } else {
                return false;
            }
        } else if (sort_field == PACKETS) {
            if (sort_direction == INCOMING) {
                return a.second.total.in_packets > b.second.total.in_packets;
            } else if (sort_direction == OUTGOING) {
                return a.second.total.out_packets > b.second.total.out_packets;
            } else {
                return false;
            }
        } else if (sort_field == BYTES) {
            if (sort_direction == INCOMING) {
                return a.second.total.in_bytes > b.second.total.in_bytes;
            } else if (sort_direction == OUTGOING) {
                return a.second.total.out_bytes > b.second.total.out_bytes;
            } else {
                return false;
            }
        } else {
            return false;
        }
    }
};


class logging_configuration_t {
    public:
    logging_configuration_t()
    : filesystem_logging(true), local_syslog_logging(false), remote_syslog_logging(false), remote_syslog_port(0) {
    }
    bool filesystem_logging;
    std::string filesystem_logging_path;

    bool local_syslog_logging;

    bool remote_syslog_logging;
    std::string remote_syslog_server;
    unsigned int remote_syslog_port;
    std::string logging_level = "info";
};

typedef std::vector<subnet_cidr_mask_t> subnet_vector_t;

typedef std::map<subnet_cidr_mask_t, std::string> subnet_to_host_group_map_t;
typedef std::map<std::string, subnet_vector_t> host_group_map_t;

typedef void (*process_packet_pointer)(simple_packet_t&);

// Attack types
enum attack_type_t {
    ATTACK_UNKNOWN                = 1,
    ATTACK_SYN_FLOOD              = 2,
    ATTACK_ICMP_FLOOD             = 3,
    ATTACK_UDP_FLOOD              = 4,
    ATTACK_IP_FRAGMENTATION_FLOOD = 5,
};

// Amplification types
enum amplification_attack_type_t {
    AMPLIFICATION_ATTACK_UNKNOWN = 1,
    AMPLIFICATION_ATTACK_DNS     = 2,
    AMPLIFICATION_ATTACK_NTP     = 3,
    AMPLIFICATION_ATTACK_SSDP    = 4,
    AMPLIFICATION_ATTACK_SNMP    = 5,
    AMPLIFICATION_ATTACK_CHARGEN = 6,
};

class total_counter_element_t {
    public:
    uint64_t bytes;
    uint64_t packets;
    uint64_t flows;

    void zeroify() {
        bytes   = 0;
        packets = 0;
        flows   = 0;
    }
};

// Set of structures for calculating total traffic counters
class total_speed_counters_t {
    public:
    total_counter_element_t total_counters[4];
    total_counter_element_t total_speed_counters[4];
    total_counter_element_t total_speed_average_counters[4];

    template <class Archive> void serialize(Archive& ar, [[maybe_unused]] const unsigned int version) {
        ar& BOOST_SERIALIZATION_NVP(total_counters);
        ar& BOOST_SERIALIZATION_NVP(total_speed_counters);
        ar& BOOST_SERIALIZATION_NVP(total_speed_average_counters);
    }
};


// structure with attack details
class attack_details_t : public subnet_counter_t {
    public:
    // This operation is very heavy, it may crash in case of entropy shortage and it actually happened to our customer
    bool generate_uuid() {
        boost::uuids::random_generator gen;

        try {
            attack_uuid = gen();
        } catch (...) {
            return false;
        }

        return true;
    }

    std::string get_protocol_name() const {
        if (ipv6) {
            return "IPv6";
        } else {
            return "IPv4";
        }
    }

    // Host group for this attack
    std::string host_group;

    // Parent hostgroup for host's host group
    std::string parent_host_group;

    direction_t attack_direction = OTHER;

    // first attackpower detected
    uint64_t attack_power = 0;

    // max attack power
    uint64_t max_attack_power    = 0;
    unsigned int attack_protocol = 0;

    // Average counters
    uint64_t average_in_bytes    = 0;
    uint64_t average_out_bytes   = 0;
    uint64_t average_in_packets  = 0;
    uint64_t average_out_packets = 0;
    uint64_t average_in_flows    = 0;
    uint64_t average_out_flows   = 0;

    // Time when we ban this IP
    time_t ban_timestamp = 0;
    bool unban_enabled   = true;
    int ban_time         = 0; // seconds of the ban

    // If this attack was detected for IPv6 protocol
    bool ipv6 = false;

    subnet_cidr_mask_t customer_network;

    attack_detection_source_t attack_detection_source = attack_detection_source_t::Automatic;
    boost::uuids::uuid attack_uuid{};
    attack_severity_t attack_severity = ATTACK_SEVERITY_MIDDLE;

    // Threshold used to trigger this attack
    attack_detection_threshold_type_t attack_detection_threshold = attack_detection_threshold_type_t::unknown;

    packet_storage_t pcap_attack_dump;

    // Direction of threshold used to trigger this attack
    attack_detection_direction_type_t attack_detection_direction = attack_detection_direction_type_t::unknown;

    std::string get_attack_uuid_as_string() const {
        return boost::uuids::to_string(attack_uuid);
    }
};


typedef attack_details_t banlist_item_t;

// struct for save per direction and per protocol details for flow
class conntrack_key_struct_t {
    public:
    uint64_t bytes   = 0;
    uint64_t packets = 0;
    // will be used for Garbage Collection
    time_t last_update_time = 0;
};


typedef uint64_t packed_session;
// Main mega structure for storing conntracks
// We should use class instead struct for correct std::map allocation
typedef std::map<packed_session, conntrack_key_struct_t> contrack_map_type;

class conntrack_main_struct_t {
    public:
    contrack_map_type in_tcp;
    contrack_map_type in_udp;
    contrack_map_type in_icmp;
    contrack_map_type in_other;

    contrack_map_type out_tcp;
    contrack_map_type out_udp;
    contrack_map_type out_icmp;
    contrack_map_type out_other;
};

typedef std::map<uint32_t, subnet_counter_t> map_for_counters;
typedef std::vector<subnet_counter_t> vector_of_counters;

typedef std::map<subnet_cidr_mask_t, vector_of_counters> map_of_vector_counters_t;

// Flow tracking structures
typedef std::vector<conntrack_main_struct_t> vector_of_flow_counters_t;
typedef std::map<subnet_cidr_mask_t, vector_of_flow_counters_t> map_of_vector_counters_for_flow_t;


typedef subnet_counter_t subnet_counter_t;
typedef std::pair<subnet_cidr_mask_t, subnet_counter_t> pair_of_map_for_subnet_counters_elements_t;
typedef std::map<subnet_cidr_mask_t, subnet_counter_t> map_for_subnet_counters_t;

// IPv6 per subnet counters
typedef std::pair<subnet_ipv6_cidr_mask_t, subnet_counter_t> pair_of_map_for_ipv6_subnet_counters_elements_t;
typedef std::unordered_map<subnet_ipv6_cidr_mask_t, subnet_counter_t> map_for_ipv6_subnet_counters_t;


class packed_conntrack_hash_t {
    public:
    packed_conntrack_hash_t() : opposite_ip(0), src_port(0), dst_port(0) {
    }
    // src or dst IP
    uint32_t opposite_ip;
    uint16_t src_port;
    uint16_t dst_port;
};

// This class consists of all configuration of global or per subnet ban thresholds
class ban_settings_t {
    public:
    ban_settings_t()
    : enable_ban(false), enable_ban_ipv6(false), enable_ban_for_pps(false), enable_ban_for_bandwidth(false),
      enable_ban_for_flows_per_second(false), enable_ban_for_tcp_pps(false), enable_ban_for_tcp_bandwidth(false),
      enable_ban_for_udp_pps(false), enable_ban_for_udp_bandwidth(false), enable_ban_for_icmp_pps(false),
      enable_ban_for_icmp_bandwidth(false), ban_threshold_tcp_mbps(0), ban_threshold_tcp_pps(0),
      ban_threshold_udp_mbps(0), ban_threshold_udp_pps(0), ban_threshold_icmp_mbps(0), ban_threshold_icmp_pps(0),
      ban_threshold_mbps(0), ban_threshold_flows(0), ban_threshold_pps(0) {
    }
    bool enable_ban;
    bool enable_ban_ipv6;

    bool enable_ban_for_pps;
    bool enable_ban_for_bandwidth;
    bool enable_ban_for_flows_per_second;

    bool enable_ban_for_tcp_pps;
    bool enable_ban_for_tcp_bandwidth;

    bool enable_ban_for_udp_pps;
    bool enable_ban_for_udp_bandwidth;

    bool enable_ban_for_icmp_pps;
    bool enable_ban_for_icmp_bandwidth;

    unsigned int ban_threshold_tcp_mbps;
    unsigned int ban_threshold_tcp_pps;

    unsigned int ban_threshold_udp_mbps;
    unsigned int ban_threshold_udp_pps;

    unsigned int ban_threshold_icmp_mbps;
    unsigned int ban_threshold_icmp_pps;

    unsigned int ban_threshold_mbps;
    unsigned int ban_threshold_flows;
    unsigned int ban_threshold_pps;
};


typedef std::map<std::string, ban_settings_t> host_group_ban_settings_map_t;

// data structure for storing data in Vector
typedef std::pair<uint32_t, subnet_counter_t> pair_of_map_elements;


#endif
