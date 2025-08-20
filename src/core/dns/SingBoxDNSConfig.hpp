#pragma once

#include "base/Qv2rayBase.hpp"
#include <QJsonObject>
#include <QJsonArray>
#include <QString>

namespace PolyCore::core::dns
{
    // DNS Server Configuration for sing-box 1.12.0+
    struct DNSServerConfig
    {
        QString tag;                    // Server identifier
        QString address;                // DNS server address  
        int port = 53;                  // DNS server port
        QString strategy = "prefer_ipv4"; // Resolution strategy
        QString detour;                 // Outbound tag for queries
        bool disable_cache = false;     // Disable DNS cache
        bool disable_expire = false;    // Disable cache expiration
        bool client_subnet_enabled = false; // EDNS Client Subnet
        QString client_subnet;          // Client subnet address
        
        QJsonObject ToJson() const;
        static DNSServerConfig FromJson(const QJsonObject &obj);
    };

    // DNS Rule Configuration for sing-box 1.12.0+
    struct DNSRuleConfig  
    {
        // Rule matching criteria
        QStringList domain;             // Domain patterns
        QStringList domain_suffix;      // Domain suffix patterns
        QStringList domain_keyword;     // Domain keyword patterns
        QStringList domain_regex;       // Domain regex patterns
        QStringList geosite;            // Geosite codes
        QStringList source_geoip;       // Source GeoIP codes
        QStringList geoip;              // Destination GeoIP codes
        QStringList source_ip_cidr;     // Source IP CIDR
        QStringList ip_cidr;            // Destination IP CIDR
        QStringList source_port;        // Source ports
        QStringList source_port_range;  // Source port ranges
        QStringList port;               // Destination ports
        QStringList port_range;         // Destination port ranges
        QStringList process_name;       // Process names
        QStringList process_path;       // Process paths
        QStringList package_name;       // Package names (Android)
        QStringList user;               // Users (Linux)
        QStringList protocol;           // Protocols
        QStringList network;            // Network types
        QStringList outbound;           // Outbound tags
        QStringList clash_mode;         // Clash modes
        QString wifi_ssid;              // WiFi SSID
        QString wifi_bssid;             // WiFi BSSID
        bool invert = false;            // Invert rule logic
        
        // Rule actions
        QString server;                 // DNS server tag
        bool disable_cache = false;     // Disable cache for this rule
        int rewrite_ttl = 0;           // Rewrite TTL
        QString client_subnet;          // Override client subnet
        
        QJsonObject ToJson() const;
        static DNSRuleConfig FromJson(const QJsonObject &obj);
    };

    // DNS Fakeip Configuration
    struct DNSFakeipConfig
    {
        bool enabled = false;           // Enable fakeip
        QString inet4_range = "198.18.0.0/15";  // IPv4 range
        QString inet6_range = "fc00::/18";      // IPv6 range
        
        QJsonObject ToJson() const;
        static DNSFakeipConfig FromJson(const QJsonObject &obj);
    };

    // Main DNS Configuration for sing-box 1.12.0+
    struct SingBoxDNSConfig
    {
        QList<DNSServerConfig> servers;     // DNS servers
        QList<DNSRuleConfig> rules;         // DNS rules
        DNSFakeipConfig fakeip;            // Fakeip configuration
        QString strategy = "prefer_ipv4";   // Default resolution strategy
        bool disable_cache = false;        // Global disable cache
        bool disable_expire = false;       // Global disable expire
        bool independent_cache = false;     // Independent cache per server
        bool reverse_mapping = false;      // Reverse DNS mapping
        QString fakeip_range = "198.18.0.0/15"; // Fakeip range (legacy)
        QStringList hijack_dns;            // Hijack DNS addresses
        
        QJsonObject ToJson() const;
        static SingBoxDNSConfig FromJson(const QJsonObject &obj);
        bool IsNewFormat() const { return !servers.isEmpty(); }
    };

    // DNS Configuration Migrator
    class SingBoxDNSMigrator
    {
    public:
        // Migrate from old format to new format (1.12.0+)
        static SingBoxDNSConfig MigrateFromLegacy(const QJsonObject &legacyConfig);
        
        // Convert from other kernel formats
        static SingBoxDNSConfig FromV2RayConfig(const QJsonObject &v2rayDns);
        static SingBoxDNSConfig FromXrayConfig(const QJsonObject &xrayDns);
        static SingBoxDNSConfig FromMihomoConfig(const QJsonObject &clashDns);
        
        // Generate default configurations
        static SingBoxDNSConfig CreateDefaultConfig();
        static SingBoxDNSConfig CreateCloudflareConfig();
        static SingBoxDNSConfig CreateGoogleConfig();
        static SingBoxDNSConfig CreateCleanBrowsingConfig();
        
    private:
        static DNSServerConfig CreateServer(const QString &address, const QString &tag, int port = 53);
        static DNSRuleConfig CreateDomainRule(const QStringList &domains, const QString &server);
        static DNSRuleConfig CreateGeositeRule(const QStringList &geosites, const QString &server);
    };

    // DNS Configuration Validator
    class SingBoxDNSValidator
    {
    public:
        static bool ValidateConfig(const SingBoxDNSConfig &config, QString &errorMessage);
        static bool ValidateServer(const DNSServerConfig &server, QString &errorMessage);
        static bool ValidateRule(const DNSRuleConfig &rule, QString &errorMessage);
        static bool ValidateFakeip(const DNSFakeipConfig &fakeip, QString &errorMessage);
        
    private:
        static bool IsValidDNSAddress(const QString &address);
        static bool IsValidPort(int port);
        static bool IsValidCIDR(const QString &cidr);
        static bool IsValidRegex(const QString &pattern);
    };

} // namespace PolyCore::core::dns