#include "PolyCoreKernelInterface.hpp"
#include "base/Qv2rayBase.hpp"
#include "utils/QvHelpers.hpp"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QRegularExpression>
#include <QYamlDocument>

#define QV_MODULE_NAME "MihomoAdapter"

namespace PolyCore::core::kernel
{
    // MihomoKernelAdapter Implementation
    
    MihomoKernelAdapter::MihomoKernelAdapter()
    {
        DEBUG("Created Mihomo (Clash) kernel adapter");
    }

    QString MihomoKernelAdapter::GenerateConfig(const CONFIGROOT &root)
    {
        try
        {
            QJsonObject config;
            
            // Basic Clash configuration
            config["mixed-port"] = 7890;
            config["allow-lan"] = false;
            config["bind-address"] = "*";
            config["mode"] = "rule";
            config["log-level"] = "info";
            config["external-controller"] = "127.0.0.1:9090";
            config["secret"] = "";
            
            // Clash specific settings
            config["unified-delay"] = false;
            config["tcp-concurrent"] = true;
            config["skip-auth-prefixes"] = QJsonArray{"127.0.0.1/32", "::1/128"};
            config["lan-allowed-ips"] = QJsonArray{"0.0.0.0/0", "::/0"};
            config["lan-disallowed-ips"] = QJsonArray{"192.168.0.0/16", "fc00::/7", "fe80::/10", "ff00::/8"};
            
            // Profile settings
            config["profile"] = QJsonObject{
                {"store-selected", true},
                {"store-fake-ip", false}
            };
            
            // IPv6 support
            config["ipv6"] = true;
            
            // Interface binding
            if (root.contains("interface"))
            {
                config["interface-name"] = root["interface"].toString();
            }
            
            // Generate proxy configuration from outbounds
            QJsonArray proxies;
            QStringList proxyNames;
            
            for (const auto &outbound : root["outbounds"].toArray())
            {
                QString proxyConfig = GenerateClashProxy(OUTBOUND(outbound.toObject()));
                if (!proxyConfig.isEmpty())
                {
                    QJsonObject proxy = QJsonDocument::fromJson(proxyConfig.toUtf8()).object();
                    proxies.append(proxy);
                    proxyNames.append(proxy["name"].toString());
                }
            }
            
            config["proxies"] = proxies;

            // Generate proxy groups from routing configuration
            if (root.contains("routing"))
            {
                QString proxyGroupsConfig = GenerateClashProxyGroup(root["routing"].toObject());
                if (!proxyGroupsConfig.isEmpty())
                {
                    config["proxy-groups"] = QJsonDocument::fromJson(proxyGroupsConfig.toUtf8()).array();
                }
            }
            else
            {
                // Default proxy group
                config["proxy-groups"] = QJsonArray{
                    QJsonObject{
                        {"name", "Proxy"},
                        {"type", "select"},
                        {"proxies", QJsonArray(proxyNames)}
                    }
                };
            }

            // Generate rules from routing configuration
            if (root.contains("routing"))
            {
                QString rulesConfig = GenerateClashRules(root["routing"].toObject());
                if (!rulesConfig.isEmpty())
                {
                    config["rules"] = QJsonDocument::fromJson(rulesConfig.toUtf8()).array();
                }
            }
            else
            {
                // Default rules
                config["rules"] = QJsonArray{
                    "GEOIP,LAN,DIRECT",
                    "GEOIP,CN,DIRECT",
                    "MATCH,Proxy"
                };
            }

            // DNS configuration
            config["dns"] = QJsonObject{
                {"enable", true},
                {"listen", "0.0.0.0:53"},
                {"default-nameserver", QJsonArray{"114.114.114.114", "8.8.8.8"}},
                {"enhanced-mode", "fake-ip"},
                {"fake-ip-range", "198.18.0.1/16"},
                {"use-hosts", true},
                {"nameserver", QJsonArray{"https://doh.pub/dns-query", "https://dns.alidns.com/dns-query"}},
                {"fallback", QJsonArray{"https://1.1.1.1/dns-query", "tls://8.8.8.8:853"}},
                {"fallback-filter", QJsonObject{
                    {"geoip", true},
                    {"geoip-code", "CN"},
                    {"ipcidr", QJsonArray{"240.0.0.0/4"}}
                }}
            };

            // TUN configuration (if supported)
            config["tun"] = QJsonObject{
                {"enable", false},
                {"stack", "system"},
                {"dns-hijack", QJsonArray{"8.8.8.8:53", "8.8.4.4:53"}},
                {"auto-route", true},
                {"auto-detect-interface", true}
            };

            return ConvertToYaml(config);
        }
        catch (const std::exception &e)
        {
            LOG("Error generating Mihomo config:", e.what());
            return QString();
        }
    }

    QStringList MihomoKernelAdapter::GetStartupArguments(const QString &configPath)
    {
        return QStringList() << "-f" << configPath;
    }

    bool MihomoKernelAdapter::ValidateConfig(const QString &configPath)
    {
        // Mihomo/Clash typically doesn't have a built-in config test command
        // Perform basic structural checks to avoid obvious invalid files
        QFile file(configPath);
        if (!file.open(QIODevice::ReadOnly))
        {
            return false;
        }

        const QString content = QString::fromUtf8(file.readAll());
        file.close();

        if (content.trimmed().isEmpty()) return false;
        // must contain basic required top-level keys
        const bool hasProxies = content.contains(QRegularExpression("^proxies:\s*$", QRegularExpression::MultilineOption));
        const bool hasRules = content.contains(QRegularExpression("^rules:\s*$", QRegularExpression::MultilineOption));
        const bool hasProxyGroups = content.contains(QRegularExpression("^proxy-groups:\s*$", QRegularExpression::MultilineOption));
        if (!(hasProxies || hasProxyGroups)) return false;
        if (!hasRules) return false;
        return true;
    }

    QString MihomoKernelAdapter::GetVersion() const
    {
        return "Unknown";
    }

    QStringList MihomoKernelAdapter::GetSupportedProtocols() const
    {
        return {"ss", "ssr", "vmess", "vless", "trojan", "snell", "socks5", "http", "hysteria", "hysteria2", "tuic", "wireguard"};
    }

    bool MihomoKernelAdapter::SupportsProtocol(const QString &protocol) const
    {
        return GetSupportedProtocols().contains(protocol.toLower());
    }

    QString MihomoKernelAdapter::ParseKernelLog(const QString &rawLog)
    {
        // Parse Mihomo/Clash log format: time level message
        QStringList lines = rawLog.split('\n', Qt::SkipEmptyParts);
        QStringList parsedLines;
        
        for (const QString &line : lines)
        {
            // Remove ANSI color codes
            QString cleanLine = line;
            cleanLine.remove(QRegularExpression("\\x1B\\[[0-9;]*m"));
            
            // Parse timestamp, level and message
            QRegularExpression logRegex(R"(^(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{3}[+-]\d{2}:\d{2})\s+(\w+)\s+(.+)$)");
            QRegularExpressionMatch match = logRegex.match(cleanLine);
            
            if (match.hasMatch())
            {
                QString timestamp = match.captured(1);
                QString level = match.captured(2);
                QString message = match.captured(3);
                
                parsedLines.append(QString("[%1] %2").arg(level.toUpper(), message));
            }
            else
            {
                parsedLines.append(cleanLine);
            }
        }
        
        return parsedLines.join('\n');
    }

    bool MihomoKernelAdapter::IsKernelReady(const QString &log)
    {
        return log.contains("HTTP proxy listening", Qt::CaseInsensitive) ||
               log.contains("SOCKS proxy listening", Qt::CaseInsensitive) ||
               log.contains("Mixed proxy listening", Qt::CaseInsensitive) ||
               log.contains("Start initial provider", Qt::CaseInsensitive);
    }

    QString MihomoKernelAdapter::ExtractErrorMessage(const QString &log)
    {
        QStringList lines = log.split('\n', Qt::SkipEmptyParts);
        
        for (const QString &line : lines)
        {
            if (line.contains("ERROR", Qt::CaseInsensitive) || 
                line.contains("FATAL", Qt::CaseInsensitive) ||
                line.contains("failed", Qt::CaseInsensitive))
            {
                return line.trimmed();
            }
        }
        
        return QString();
    }

    QMap<StatisticsType, QvStatsSpeed> MihomoKernelAdapter::ParseStatsOutput(const QString &output)
    {
        QMap<StatisticsType, QvStatsSpeed> stats;
        
        // Mihomo/Clash usually provides stats through REST API, not stdout
        // This would typically be queried from http://127.0.0.1:9090/traffic
        Q_UNUSED(output)
        
        return stats;
    }

    // Private helper methods

    QString MihomoKernelAdapter::GenerateClashProxy(const OUTBOUND &outbound)
    {
        QJsonObject proxy;
        QString protocol = outbound["protocol"].toString();
        
        proxy["name"] = outbound["tag"].toString();
        proxy["type"] = protocol;
        
        if (protocol == "vmess")
        {
            auto settings = outbound["settings"].toObject();
            auto vnext = settings["vnext"].toArray();
            if (!vnext.isEmpty())
            {
                auto server = vnext[0].toObject();
                proxy["server"] = server["address"].toString();
                proxy["port"] = server["port"].toInt();
                
                auto users = server["users"].toArray();
                if (!users.isEmpty())
                {
                    auto user = users[0].toObject();
                    proxy["uuid"] = user["id"].toString();
                    proxy["alterId"] = user["alterId"].toInt(0);
                    proxy["cipher"] = user["security"].toString("auto");
                }
            }
        }
        else if (protocol == "vless")
        {
            auto settings = outbound["settings"].toObject();
            auto vnext = settings["vnext"].toArray();
            if (!vnext.isEmpty())
            {
                auto server = vnext[0].toObject();
                proxy["server"] = server["address"].toString();
                proxy["port"] = server["port"].toInt();
                
                auto users = server["users"].toArray();
                if (!users.isEmpty())
                {
                    auto user = users[0].toObject();
                    proxy["uuid"] = user["id"].toString();
                    proxy["flow"] = user["flow"].toString();
                }
            }
        }
        else if (protocol == "shadowsocks")
        {
            auto settings = outbound["settings"].toObject();
            auto servers = settings["servers"].toArray();
            if (!servers.isEmpty())
            {
                auto server = servers[0].toObject();
                proxy["server"] = server["address"].toString();
                proxy["port"] = server["port"].toInt();
                proxy["cipher"] = server["method"].toString();
                proxy["password"] = server["password"].toString();
            }
        }
        else if (protocol == "trojan")
        {
            auto settings = outbound["settings"].toObject();
            auto servers = settings["servers"].toArray();
            if (!servers.isEmpty())
            {
                auto server = servers[0].toObject();
                proxy["server"] = server["address"].toString();
                proxy["port"] = server["port"].toInt();
                proxy["password"] = server["password"].toString();
            }
        }
        else if (protocol == "socks")
        {
            proxy["type"] = "socks5";
            auto settings = outbound["settings"].toObject();
            auto servers = settings["servers"].toArray();
            if (!servers.isEmpty())
            {
                auto server = servers[0].toObject();
                proxy["server"] = server["address"].toString();
                proxy["port"] = server["port"].toInt();
                proxy["username"] = server["users"].toArray()[0].toObject()["user"].toString();
                proxy["password"] = server["users"].toArray()[0].toObject()["pass"].toString();
            }
        }
        else if (protocol == "http")
        {
            auto settings = outbound["settings"].toObject();
            auto servers = settings["servers"].toArray();
            if (!servers.isEmpty())
            {
                auto server = servers[0].toObject();
                proxy["server"] = server["address"].toString();
                proxy["port"] = server["port"].toInt();
                proxy["username"] = server["users"].toArray()[0].toObject()["username"].toString();
                proxy["password"] = server["users"].toArray()[0].toObject()["password"].toString();
            }
        }

        // Handle stream settings
        if (outbound.contains("streamSettings"))
        {
            auto streamSettings = outbound["streamSettings"].toObject();
            QString network = streamSettings["network"].toString("tcp");
            proxy["network"] = network;
            
            if (network == "ws")
            {
                auto wsSettings = streamSettings["wsSettings"].toObject();
                proxy["ws-opts"] = QJsonObject{
                    {"path", wsSettings["path"].toString()},
                    {"headers", wsSettings["headers"].toObject()}
                };
            }
            else if (network == "grpc")
            {
                auto grpcSettings = streamSettings["grpcSettings"].toObject();
                proxy["grpc-opts"] = QJsonObject{
                    {"grpc-service-name", grpcSettings["serviceName"].toString()}
                };
            }
            else if (network == "h2")
            {
                auto httpSettings = streamSettings["httpSettings"].toObject();
                proxy["h2-opts"] = QJsonObject{
                    {"host", httpSettings["host"].toArray()},
                    {"path", httpSettings["path"].toString()}
                };
            }
            
            // TLS settings
            if (streamSettings.contains("security"))
            {
                QString security = streamSettings["security"].toString();
                if (security == "tls")
                {
                    proxy["tls"] = true;
                    if (streamSettings.contains("tlsSettings"))
                    {
                        auto tlsSettings = streamSettings["tlsSettings"].toObject();
                        proxy["servername"] = tlsSettings["serverName"].toString();
                        proxy["skip-cert-verify"] = tlsSettings["allowInsecure"].toBool(false);
                    }
                }
                else if (security == "reality")
                {
                    proxy["tls"] = true;
                    proxy["reality-opts"] = QJsonObject{
                        {"public-key", streamSettings["realitySettings"].toObject()["publicKey"].toString()},
                        {"short-id", streamSettings["realitySettings"].toObject()["shortId"].toString()}
                    };
                }
            }
        }
        
        return JsonToString(proxy);
    }

    QString MihomoKernelAdapter::GenerateClashProxyGroup(const QJsonObject &routing)
    {
        QJsonArray proxyGroups;
        
        // Default proxy group
        proxyGroups.append(QJsonObject{
            {"name", "Proxy"},
            {"type", "select"},
            {"proxies", QJsonArray{"DIRECT"}} // Will be populated with actual proxy names
        });
        
        // Auto proxy group
        proxyGroups.append(QJsonObject{
            {"name", "Auto"},
            {"type", "url-test"},
            {"proxies", QJsonArray()}, // Will be populated with actual proxy names
            {"url", "http://www.gstatic.com/generate_204"},
            {"interval", 300}
        });
        
        // Load balance group
        proxyGroups.append(QJsonObject{
            {"name", "LoadBalance"},
            {"type", "load-balance"},
            {"proxies", QJsonArray()}, // Will be populated with actual proxy names
            {"url", "http://www.gstatic.com/generate_204"},
            {"interval", 300}
        });
        
        return JsonToString(proxyGroups);
    }

    QString MihomoKernelAdapter::GenerateClashRules(const QJsonObject &routing)
    {
        QJsonArray rules;
        
        if (routing.contains("rules"))
        {
            auto routingRules = routing["rules"].toArray();
            for (const auto &rule : routingRules)
            {
                auto ruleObj = rule.toObject();
                QString type = ruleObj["type"].toString();
                QString outboundTag = ruleObj["outboundTag"].toString("Proxy");
                
                if (type == "field")
                {
                    if (ruleObj.contains("domain"))
                    {
                        auto domains = ruleObj["domain"].toArray();
                        for (const auto &domain : domains)
                        {
                            if (domain.toString().startsWith("geosite:"))
                            {
                                QString geosite = domain.toString().mid(8).toUpper();
                                rules.append("GEOSITE," + geosite + "," + outboundTag);
                            }
                            else if (domain.toString().startsWith("domain:"))
                            {
                                QString domainName = domain.toString().mid(7);
                                rules.append("DOMAIN-SUFFIX," + domainName + "," + outboundTag);
                            }
                            else if (domain.toString().startsWith("keyword:"))
                            {
                                QString keyword = domain.toString().mid(8);
                                rules.append("DOMAIN-KEYWORD," + keyword + "," + outboundTag);
                            }
                            else
                            {
                                rules.append("DOMAIN," + domain.toString() + "," + outboundTag);
                            }
                        }
                    }
                    
                    if (ruleObj.contains("ip"))
                    {
                        auto ips = ruleObj["ip"].toArray();
                        for (const auto &ip : ips)
                        {
                            if (ip.toString().startsWith("geoip:"))
                            {
                                QString geoip = ip.toString().mid(6).toUpper();
                                rules.append("GEOIP," + geoip + "," + outboundTag);
                            }
                            else
                            {
                                rules.append("IP-CIDR," + ip.toString() + "," + outboundTag);
                            }
                        }
                    }
                    
                    if (ruleObj.contains("port"))
                    {
                        auto ports = ruleObj["port"].toArray();
                        for (const auto &port : ports)
                        {
                            rules.append("DST-PORT," + port.toString() + "," + outboundTag);
                        }
                    }
                }
            }
        }
        
        // Default rules
        if (rules.isEmpty())
        {
            rules.append("GEOIP,LAN,DIRECT");
            rules.append("GEOIP,CN,DIRECT");
            rules.append("MATCH,Proxy");
        }
        else
        {
            rules.append("MATCH,Proxy");
        }
        
        return JsonToString(rules);
    }

    QString MihomoKernelAdapter::ConvertToYaml(const QJsonObject &json)
    {
        // Simple JSON to YAML conversion
        // In a real implementation, you'd use a proper YAML library
        QString yaml;
        QTextStream stream(&yaml);
        
        for (auto it = json.begin(); it != json.end(); ++it)
        {
            QString key = it.key();
            QJsonValue value = it.value();
            
            if (value.isObject())
            {
                stream << key << ":\n";
                ConvertObjectToYaml(value.toObject(), stream, 2);
            }
            else if (value.isArray())
            {
                stream << key << ":\n";
                ConvertArrayToYaml(value.toArray(), stream, 2);
            }
            else if (value.isString())
            {
                stream << key << ": " << value.toString() << "\n";
            }
            else if (value.isDouble())
            {
                stream << key << ": " << value.toDouble() << "\n";
            }
            else if (value.isBool())
            {
                stream << key << ": " << (value.toBool() ? "true" : "false") << "\n";
            }
        }
        
        return yaml;
    }

    void MihomoKernelAdapter::ConvertObjectToYaml(const QJsonObject &obj, QTextStream &stream, int indent)
    {
        QString indentStr = QString(indent, ' ');
        
        for (auto it = obj.begin(); it != obj.end(); ++it)
        {
            QString key = it.key();
            QJsonValue value = it.value();
            
            if (value.isObject())
            {
                stream << indentStr << key << ":\n";
                ConvertObjectToYaml(value.toObject(), stream, indent + 2);
            }
            else if (value.isArray())
            {
                stream << indentStr << key << ":\n";
                ConvertArrayToYaml(value.toArray(), stream, indent + 2);
            }
            else if (value.isString())
            {
                stream << indentStr << key << ": " << value.toString() << "\n";
            }
            else if (value.isDouble())
            {
                stream << indentStr << key << ": " << value.toDouble() << "\n";
            }
            else if (value.isBool())
            {
                stream << indentStr << key << ": " << (value.toBool() ? "true" : "false") << "\n";
            }
        }
    }

    void MihomoKernelAdapter::ConvertArrayToYaml(const QJsonArray &arr, QTextStream &stream, int indent)
    {
        QString indentStr = QString(indent, ' ');
        
        for (const auto &value : arr)
        {
            if (value.isObject())
            {
                stream << indentStr << "-\n";
                ConvertObjectToYaml(value.toObject(), stream, indent + 2);
            }
            else if (value.isArray())
            {
                stream << indentStr << "-\n";
                ConvertArrayToYaml(value.toArray(), stream, indent + 2);
            }
            else if (value.isString())
            {
                stream << indentStr << "- " << value.toString() << "\n";
            }
            else if (value.isDouble())
            {
                stream << indentStr << "- " << value.toDouble() << "\n";
            }
            else if (value.isBool())
            {
                stream << indentStr << "- " << (value.toBool() ? "true" : "false") << "\n";
            }
        }
    }

} // namespace PolyCore::core::kernel