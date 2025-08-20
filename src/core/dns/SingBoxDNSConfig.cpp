#include "SingBoxDNSConfig.hpp"
#include <QJsonDocument>
#include <QRegularExpression>
#include <QHostAddress>

namespace PolyCore::core::dns
{
    // DNSServerConfig Implementation
    QJsonObject DNSServerConfig::ToJson() const
    {
        QJsonObject obj;
        obj["tag"] = tag;
        obj["address"] = address;
        if (port != 53) obj["port"] = port;
        if (strategy != "prefer_ipv4") obj["strategy"] = strategy;
        if (!detour.isEmpty()) obj["detour"] = detour;
        if (disable_cache) obj["disable_cache"] = true;
        if (disable_expire) obj["disable_expire"] = true;
        if (client_subnet_enabled) {
            obj["client_subnet"] = client_subnet_enabled;
            if (!client_subnet.isEmpty()) obj["client_subnet"] = client_subnet;
        }
        return obj;
    }

    DNSServerConfig DNSServerConfig::FromJson(const QJsonObject &obj)
    {
        DNSServerConfig config;
        config.tag = obj["tag"].toString();
        config.address = obj["address"].toString();
        config.port = obj["port"].toInt(53);
        config.strategy = obj["strategy"].toString("prefer_ipv4");
        config.detour = obj["detour"].toString();
        config.disable_cache = obj["disable_cache"].toBool();
        config.disable_expire = obj["disable_expire"].toBool();
        config.client_subnet_enabled = obj.contains("client_subnet");
        config.client_subnet = obj["client_subnet"].toString();
        return config;
    }

    // DNSRuleConfig Implementation  
    QJsonObject DNSRuleConfig::ToJson() const
    {
        QJsonObject obj;
        
        // Add matching criteria
        if (!domain.isEmpty()) obj["domain"] = QJsonArray::fromStringList(domain);
        if (!domain_suffix.isEmpty()) obj["domain_suffix"] = QJsonArray::fromStringList(domain_suffix);
        if (!domain_keyword.isEmpty()) obj["domain_keyword"] = QJsonArray::fromStringList(domain_keyword);
        if (!domain_regex.isEmpty()) obj["domain_regex"] = QJsonArray::fromStringList(domain_regex);
        if (!geosite.isEmpty()) obj["geosite"] = QJsonArray::fromStringList(geosite);
        if (!source_geoip.isEmpty()) obj["source_geoip"] = QJsonArray::fromStringList(source_geoip);
        if (!geoip.isEmpty()) obj["geoip"] = QJsonArray::fromStringList(geoip);
        if (!source_ip_cidr.isEmpty()) obj["source_ip_cidr"] = QJsonArray::fromStringList(source_ip_cidr);
        if (!ip_cidr.isEmpty()) obj["ip_cidr"] = QJsonArray::fromStringList(ip_cidr);
        if (!source_port.isEmpty()) obj["source_port"] = QJsonArray::fromStringList(source_port);
        if (!source_port_range.isEmpty()) obj["source_port_range"] = QJsonArray::fromStringList(source_port_range);
        if (!port.isEmpty()) obj["port"] = QJsonArray::fromStringList(port);
        if (!port_range.isEmpty()) obj["port_range"] = QJsonArray::fromStringList(port_range);
        if (!process_name.isEmpty()) obj["process_name"] = QJsonArray::fromStringList(process_name);
        if (!process_path.isEmpty()) obj["process_path"] = QJsonArray::fromStringList(process_path);
        if (!package_name.isEmpty()) obj["package_name"] = QJsonArray::fromStringList(package_name);
        if (!user.isEmpty()) obj["user"] = QJsonArray::fromStringList(user);
        if (!protocol.isEmpty()) obj["protocol"] = QJsonArray::fromStringList(protocol);
        if (!network.isEmpty()) obj["network"] = QJsonArray::fromStringList(network);
        if (!outbound.isEmpty()) obj["outbound"] = QJsonArray::fromStringList(outbound);
        if (!clash_mode.isEmpty()) obj["clash_mode"] = QJsonArray::fromStringList(clash_mode);
        if (!wifi_ssid.isEmpty()) obj["wifi_ssid"] = wifi_ssid;
        if (!wifi_bssid.isEmpty()) obj["wifi_bssid"] = wifi_bssid;
        if (invert) obj["invert"] = true;
        
        // Add actions
        if (!server.isEmpty()) obj["server"] = server;
        if (disable_cache) obj["disable_cache"] = true;
        if (rewrite_ttl > 0) obj["rewrite_ttl"] = rewrite_ttl;
        if (!client_subnet.isEmpty()) obj["client_subnet"] = client_subnet;
        
        return obj;
    }

    DNSRuleConfig DNSRuleConfig::FromJson(const QJsonObject &obj)
    {
        DNSRuleConfig config;
        
        // Parse matching criteria
        auto arrayToStringList = [](const QJsonValue &val) {
            QStringList list;
            if (val.isArray()) {
                for (const auto &item : val.toArray()) {
                    list << item.toString();
                }
            } else if (val.isString()) {
                list << val.toString();
            }
            return list;
        };
        
        config.domain = arrayToStringList(obj["domain"]);
        config.domain_suffix = arrayToStringList(obj["domain_suffix"]);
        config.domain_keyword = arrayToStringList(obj["domain_keyword"]);
        config.domain_regex = arrayToStringList(obj["domain_regex"]);
        config.geosite = arrayToStringList(obj["geosite"]);
        config.source_geoip = arrayToStringList(obj["source_geoip"]);
        config.geoip = arrayToStringList(obj["geoip"]);
        config.source_ip_cidr = arrayToStringList(obj["source_ip_cidr"]);
        config.ip_cidr = arrayToStringList(obj["ip_cidr"]);
        config.source_port = arrayToStringList(obj["source_port"]);
        config.source_port_range = arrayToStringList(obj["source_port_range"]);
        config.port = arrayToStringList(obj["port"]);
        config.port_range = arrayToStringList(obj["port_range"]);
        config.process_name = arrayToStringList(obj["process_name"]);
        config.process_path = arrayToStringList(obj["process_path"]);
        config.package_name = arrayToStringList(obj["package_name"]);
        config.user = arrayToStringList(obj["user"]);
        config.protocol = arrayToStringList(obj["protocol"]);
        config.network = arrayToStringList(obj["network"]);
        config.outbound = arrayToStringList(obj["outbound"]);
        config.clash_mode = arrayToStringList(obj["clash_mode"]);
        config.wifi_ssid = obj["wifi_ssid"].toString();
        config.wifi_bssid = obj["wifi_bssid"].toString();
        config.invert = obj["invert"].toBool();
        
        // Parse actions
        config.server = obj["server"].toString();
        config.disable_cache = obj["disable_cache"].toBool();
        config.rewrite_ttl = obj["rewrite_ttl"].toInt();
        config.client_subnet = obj["client_subnet"].toString();
        
        return config;
    }

    // DNSFakeipConfig Implementation
    QJsonObject DNSFakeipConfig::ToJson() const
    {
        QJsonObject obj;
        obj["enabled"] = enabled;
        if (enabled) {
            obj["inet4_range"] = inet4_range;
            obj["inet6_range"] = inet6_range;
        }
        return obj;
    }

    DNSFakeipConfig DNSFakeipConfig::FromJson(const QJsonObject &obj)
    {
        DNSFakeipConfig config;
        config.enabled = obj["enabled"].toBool();
        config.inet4_range = obj["inet4_range"].toString("198.18.0.0/15");
        config.inet6_range = obj["inet6_range"].toString("fc00::/18");
        return config;
    }

    // SingBoxDNSConfig Implementation
    QJsonObject SingBoxDNSConfig::ToJson() const
    {
        QJsonObject obj;
        
        // Add servers
        if (!servers.isEmpty()) {
            QJsonArray serverArray;
            for (const auto &server : servers) {
                serverArray.append(server.ToJson());
            }
            obj["servers"] = serverArray;
        }
        
        // Add rules
        if (!rules.isEmpty()) {
            QJsonArray ruleArray;
            for (const auto &rule : rules) {
                ruleArray.append(rule.ToJson());
            }
            obj["rules"] = ruleArray;
        }
        
        // Add fakeip
        if (fakeip.enabled) {
            obj["fakeip"] = fakeip.ToJson();
        }
        
        // Add global settings
        if (strategy != "prefer_ipv4") obj["strategy"] = strategy;
        if (disable_cache) obj["disable_cache"] = true;
        if (disable_expire) obj["disable_expire"] = true;
        if (independent_cache) obj["independent_cache"] = true;
        if (reverse_mapping) obj["reverse_mapping"] = true;
        
        // Legacy settings for compatibility
        if (!fakeip_range.isEmpty() && fakeip_range != "198.18.0.0/15") {
            obj["fakeip_range"] = fakeip_range;
        }
        if (!hijack_dns.isEmpty()) {
            obj["hijack_dns"] = QJsonArray::fromStringList(hijack_dns);
        }
        
        return obj;
    }

    SingBoxDNSConfig SingBoxDNSConfig::FromJson(const QJsonObject &obj)
    {
        SingBoxDNSConfig config;
        
        // Parse servers
        if (obj.contains("servers") && obj["servers"].isArray()) {
            for (const auto &serverVal : obj["servers"].toArray()) {
                if (serverVal.isObject()) {
                    config.servers.append(DNSServerConfig::FromJson(serverVal.toObject()));
                }
            }
        }
        
        // Parse rules
        if (obj.contains("rules") && obj["rules"].isArray()) {
            for (const auto &ruleVal : obj["rules"].toArray()) {
                if (ruleVal.isObject()) {
                    config.rules.append(DNSRuleConfig::FromJson(ruleVal.toObject()));
                }
            }
        }
        
        // Parse fakeip
        if (obj.contains("fakeip")) {
            config.fakeip = DNSFakeipConfig::FromJson(obj["fakeip"].toObject());
        }
        
        // Parse global settings
        config.strategy = obj["strategy"].toString("prefer_ipv4");
        config.disable_cache = obj["disable_cache"].toBool();
        config.disable_expire = obj["disable_expire"].toBool();
        config.independent_cache = obj["independent_cache"].toBool();
        config.reverse_mapping = obj["reverse_mapping"].toBool();
        config.fakeip_range = obj["fakeip_range"].toString("198.18.0.0/15");
        
        // Parse hijack_dns
        if (obj.contains("hijack_dns") && obj["hijack_dns"].isArray()) {
            for (const auto &dns : obj["hijack_dns"].toArray()) {
                config.hijack_dns << dns.toString();
            }
        }
        
        return config;
    }

    // SingBoxDNSMigrator Implementation
    SingBoxDNSConfig SingBoxDNSMigrator::MigrateFromLegacy(const QJsonObject &legacyConfig)
    {
        SingBoxDNSConfig newConfig;
        
        // Migrate basic settings
        newConfig.strategy = legacyConfig["strategy"].toString("prefer_ipv4");
        newConfig.disable_cache = legacyConfig["disable_cache"].toBool();
        newConfig.disable_expire = legacyConfig["disable_expire"].toBool();
        newConfig.fakeip_range = legacyConfig["fakeip_range"].toString("198.18.0.0/15");
        
        // Migrate servers from legacy format
        if (legacyConfig.contains("servers")) {
            const auto &servers = legacyConfig["servers"];
            if (servers.isArray()) {
                int index = 0;
                for (const auto &serverVal : servers.toArray()) {
                    DNSServerConfig server;
                    if (serverVal.isString()) {
                        // Simple string format: "1.1.1.1"
                        server.address = serverVal.toString();
                        server.tag = QString("dns_%1").arg(index++);
                    } else if (serverVal.isObject()) {
                        // Object format with more settings
                        const auto &serverObj = serverVal.toObject();
                        server.address = serverObj["address"].toString();
                        server.tag = serverObj["tag"].toString(QString("dns_%1").arg(index++));
                        server.port = serverObj["port"].toInt(53);
                        server.detour = serverObj["detour"].toString();
                    }
                    if (!server.address.isEmpty()) {
                        newConfig.servers.append(server);
                    }
                }
            }
        }
        
        // Migrate rules from legacy format
        if (legacyConfig.contains("rules")) {
            const auto &rules = legacyConfig["rules"];
            if (rules.isArray()) {
                for (const auto &ruleVal : rules.toArray()) {
                    if (ruleVal.isObject()) {
                        DNSRuleConfig rule = DNSRuleConfig::FromJson(ruleVal.toObject());
                        newConfig.rules.append(rule);
                    }
                }
            }
        }
        
        // Migrate fakeip settings
        if (legacyConfig.contains("fakeip")) {
            const auto &fakeipObj = legacyConfig["fakeip"].toObject();
            newConfig.fakeip.enabled = fakeipObj["enabled"].toBool();
            newConfig.fakeip.inet4_range = fakeipObj["inet4_range"].toString("198.18.0.0/15");
            newConfig.fakeip.inet6_range = fakeipObj["inet6_range"].toString("fc00::/18");
        } else if (legacyConfig.contains("fakeip_range")) {
            // Legacy single fakeip range
            newConfig.fakeip.enabled = true;
            newConfig.fakeip.inet4_range = legacyConfig["fakeip_range"].toString("198.18.0.0/15");
        }
        
        return newConfig;
    }

    SingBoxDNSConfig SingBoxDNSMigrator::CreateDefaultConfig()
    {
        SingBoxDNSConfig config;
        
        // Add default DNS servers
        config.servers.append(CreateServer("https://1.1.1.1/dns-query", "cloudflare"));
        config.servers.append(CreateServer("8.8.8.8", "google"));
        config.servers.append(CreateServer("223.5.5.5", "alidns"));
        
        // Add default rules
        DNSRuleConfig cnRule;
        cnRule.geosite = {"cn", "private"};
        cnRule.server = "alidns";
        config.rules.append(cnRule);
        
        DNSRuleConfig defaultRule;
        defaultRule.server = "cloudflare";
        config.rules.append(defaultRule);
        
        return config;
    }

    DNSServerConfig SingBoxDNSMigrator::CreateServer(const QString &address, const QString &tag, int port)
    {
        DNSServerConfig server;
        server.address = address;
        server.tag = tag;
        server.port = port;
        return server;
    }

    // SingBoxDNSValidator Implementation
    bool SingBoxDNSValidator::ValidateConfig(const SingBoxDNSConfig &config, QString &errorMessage)
    {
        // Validate servers
        if (config.servers.isEmpty()) {
            errorMessage = "At least one DNS server must be configured";
            return false;
        }
        
        for (const auto &server : config.servers) {
            if (!ValidateServer(server, errorMessage)) {
                return false;
            }
        }
        
        // Validate rules
        for (const auto &rule : config.rules) {
            if (!ValidateRule(rule, errorMessage)) {
                return false;
            }
        }
        
        // Validate fakeip
        if (config.fakeip.enabled && !ValidateFakeip(config.fakeip, errorMessage)) {
            return false;
        }
        
        return true;
    }

    bool SingBoxDNSValidator::ValidateServer(const DNSServerConfig &server, QString &errorMessage)
    {
        if (server.tag.isEmpty()) {
            errorMessage = "DNS server tag cannot be empty";
            return false;
        }
        
        if (server.address.isEmpty()) {
            errorMessage = "DNS server address cannot be empty";
            return false;
        }
        
        if (!IsValidDNSAddress(server.address)) {
            errorMessage = QString("Invalid DNS server address: %1").arg(server.address);
            return false;
        }
        
        if (!IsValidPort(server.port)) {
            errorMessage = QString("Invalid DNS server port: %1").arg(server.port);
            return false;
        }
        
        return true;
    }

    bool SingBoxDNSValidator::ValidateRule(const DNSRuleConfig &rule, QString &errorMessage)
    {
        if (rule.server.isEmpty()) {
            errorMessage = "DNS rule must specify a server";
            return false;
        }
        
        // Validate regex patterns
        for (const auto &pattern : rule.domain_regex) {
            if (!IsValidRegex(pattern)) {
                errorMessage = QString("Invalid domain regex pattern: %1").arg(pattern);
                return false;
            }
        }
        
        // Validate CIDR ranges
        for (const auto &cidr : rule.source_ip_cidr + rule.ip_cidr) {
            if (!IsValidCIDR(cidr)) {
                errorMessage = QString("Invalid CIDR range: %1").arg(cidr);
                return false;
            }
        }
        
        return true;
    }

    bool SingBoxDNSValidator::IsValidDNSAddress(const QString &address)
    {
        // Check for various DNS address formats
        if (address.startsWith("https://") || address.startsWith("tls://") || 
            address.startsWith("quic://") || address.startsWith("rcode://")) {
            return true; // Assume URL formats are valid
        }
        
        // Check for IP address
        QHostAddress hostAddr(address);
        if (!hostAddr.isNull()) {
            return true;
        }
        
        // Check for hostname
        QRegularExpression hostnameRegex("^[a-zA-Z0-9]([a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])?(\\.[a-zA-Z0-9]([a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])?)*$");
        return hostnameRegex.match(address).hasMatch();
    }

    bool SingBoxDNSValidator::IsValidPort(int port)
    {
        return port > 0 && port < 65536;
    }

    bool SingBoxDNSValidator::IsValidCIDR(const QString &cidr)
    {
        QStringList parts = cidr.split("/");
        if (parts.size() != 2) return false;
        
        QHostAddress addr(parts[0]);
        if (addr.isNull()) return false;
        
        bool ok;
        int prefix = parts[1].toInt(&ok);
        if (!ok) return false;
        
        if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
            return prefix >= 0 && prefix <= 32;
        } else if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
            return prefix >= 0 && prefix <= 128;
        }
        
        return false;
    }

    bool SingBoxDNSValidator::IsValidRegex(const QString &pattern)
    {
        QRegularExpression regex(pattern);
        return regex.isValid();
    }

} // namespace PolyCore::core::dns