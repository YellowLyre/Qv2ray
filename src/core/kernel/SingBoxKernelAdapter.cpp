#include "PolyCoreKernelInterface.hpp"
#include "base/Qv2rayBase.hpp"
#include "core/dns/SingBoxDNSConfig.hpp"
#include "core/protocols/ProtocolSupport.hpp"
#include "utils/QvHelpers.hpp"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QRegularExpression>

#define QV_MODULE_NAME "SingBoxAdapter"

namespace PolyCore::core::kernel
{
    // SingBoxKernelAdapter Implementation
    
    SingBoxKernelAdapter::SingBoxKernelAdapter()
    {
        DEBUG("Created sing-box kernel adapter");
    }

    QString SingBoxKernelAdapter::GenerateConfig(const CONFIGROOT &root)
    {
        try
        {
            QJsonObject config;
            
            // Basic sing-box configuration structure
            config["log"] = QJsonObject{
                {"level", "warn"},
                {"timestamp", true}
            };

            // Experimental features
            config["experimental"] = QJsonObject{
                {"clash_api", QJsonObject{
                    {"external_controller", "127.0.0.1:9090"},
                    {"external_ui", ""},
                    {"external_ui_download_url", ""},
                    {"external_ui_download_detour", ""},
                    {"secret", ""},
                    {"default_mode", "rule"}
                }},
                {"cache_file", QJsonObject{
                    {"enabled", true},
                    {"path", "cache.db"},
                    {"cache_id", "polycore"},
                    {"store_fakeip", false}
                }}
            };

            // Generate DNS configuration (sing-box 1.12.0+ format)
            if (root.contains("dns"))
            {
                QString dnsConfig = GenerateSingBoxDNS(root["dns"].toObject());
                if (!dnsConfig.isEmpty())
                {
                    config["dns"] = QJsonDocument::fromJson(dnsConfig.toUtf8()).object();
                }
            }
            else
            {
                // Default DNS configuration using new 1.12.0+ format
                using namespace PolyCore::core::dns;
                SingBoxDNSConfig defaultDns = SingBoxDNSMigrator::CreateDefaultConfig();
                config["dns"] = defaultDns.ToJson();
            }

            // Generate inbounds
            QJsonArray inbounds;
            for (const auto &inbound : root["inbounds"].toArray())
            {
                QString inboundConfig = GenerateSingBoxInbound(INBOUND(inbound.toObject()));
                if (!inboundConfig.isEmpty())
                {
                    inbounds.append(QJsonDocument::fromJson(inboundConfig.toUtf8()).object());
                }
            }
            config["inbounds"] = inbounds;

            // Generate outbounds
            QJsonArray outbounds;
            for (const auto &outbound : root["outbounds"].toArray())
            {
                QString outboundConfig = GenerateSingBoxOutbound(OUTBOUND(outbound.toObject()));
                if (!outboundConfig.isEmpty())
                {
                    outbounds.append(QJsonDocument::fromJson(outboundConfig.toUtf8()).object());
                }
            }
            
            // Add default direct and block outbounds
            outbounds.append(QJsonObject{
                {"tag", "direct"},
                {"type", "direct"}
            });
            
            outbounds.append(QJsonObject{
                {"tag", "block"},
                {"type", "block"}
            });
            
            config["outbounds"] = outbounds;

            // Generate route configuration
            if (root.contains("routing"))
            {
                QString routeConfig = GenerateSingBoxRoute(root["routing"].toObject());
                if (!routeConfig.isEmpty())
                {
                    config["route"] = QJsonDocument::fromJson(routeConfig.toUtf8()).object();
                }
            }
            else
            {
                // Default routing
                config["route"] = QJsonObject{
                    {"auto_detect_interface", true},
                    {"final", "proxy"},
                    {"rules", QJsonArray{
                        QJsonObject{
                            {"geosite", QJsonArray{"private"}},
                            {"outbound", "direct"}
                        },
                        QJsonObject{
                            {"geosite", QJsonArray{"cn"}},
                            {"outbound", "direct"}
                        },
                        QJsonObject{
                            {"geoip", QJsonArray{"private", "cn"}},
                            {"outbound", "direct"}
                        }
                    }}
                };
            }

            return JsonToString(config);
        }
        catch (const std::exception &e)
        {
            LOG("Error generating sing-box config:", e.what());
            return QString();
        }
    }

    QStringList SingBoxKernelAdapter::GetStartupArguments(const QString &configPath)
    {
        return QStringList() << "run" << "-c" << configPath;
    }

    bool SingBoxKernelAdapter::ValidateConfig(const QString &configPath)
    {
        // Use sing-box's built-in validation
        QProcess process;
        QStringList args = QStringList() << "check" << "-c" << configPath;
        
        process.start("", args); // Path will be filled by the manager
        process.waitForFinished(5000);
        
        return process.exitCode() == 0;
    }

    QString SingBoxKernelAdapter::GetVersion() const
    {
        return "Unknown";
    }

    QStringList SingBoxKernelAdapter::GetSupportedProtocols() const
    {
        return {"direct", "block", "socks", "http", "shadowsocks", "vmess", "vless", "trojan", 
                "wireguard", "hysteria", "hysteria2", "tuic", "ssh", "shadowtls", "anylts"};
    }

    bool SingBoxKernelAdapter::SupportsProtocol(const QString &protocol) const
    {
        return GetSupportedProtocols().contains(protocol.toLower());
    }

    QString SingBoxKernelAdapter::ParseKernelLog(const QString &rawLog)
    {
        // Parse sing-box log format: timestamp [level] message
        QStringList lines = rawLog.split('\n', Qt::SkipEmptyParts);
        QStringList parsedLines;
        
        for (const QString &line : lines)
        {
            // Remove ANSI color codes
            QString cleanLine = line;
            cleanLine.remove(QRegularExpression("\\x1B\\[[0-9;]*m"));
            
            // Parse timestamp and level
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

    bool SingBoxKernelAdapter::IsKernelReady(const QString &log)
    {
        return log.contains("started", Qt::CaseInsensitive) || 
               log.contains("listening", Qt::CaseInsensitive) ||
               log.contains("sing-box started", Qt::CaseInsensitive);
    }

    QString SingBoxKernelAdapter::ExtractErrorMessage(const QString &log)
    {
        QStringList lines = log.split('\n', Qt::SkipEmptyParts);
        
        for (const QString &line : lines)
        {
            if (line.contains("ERROR", Qt::CaseInsensitive) || 
                line.contains("FATAL", Qt::CaseInsensitive) ||
                line.contains("panic", Qt::CaseInsensitive) ||
                line.contains("failed", Qt::CaseInsensitive))
            {
                return line.trimmed();
            }
        }
        
        return QString();
    }

    QMap<StatisticsType, QvStatsSpeed> SingBoxKernelAdapter::ParseStatsOutput(const QString &output)
    {
        QMap<StatisticsType, QvStatsSpeed> stats;
        
        // sing-box provides stats through Clash API
        // This would typically be queried from the API endpoint
        Q_UNUSED(output)
        
        return stats;
    }

    // Private helper methods

    QString SingBoxKernelAdapter::GenerateSingBoxInbound(const INBOUND &inbound)
    {
        QJsonObject config;
        QString protocol = inbound["protocol"].toString();
        
        config["tag"] = inbound["tag"].toString();
        config["type"] = protocol;
        config["listen"] = inbound["listen"].toString("127.0.0.1");
        config["listen_port"] = inbound["port"].toInt();
        
        if (protocol == "mixed")
        {
            // Mixed proxy (HTTP + SOCKS)
            config["users"] = QJsonArray();
        }
        else if (protocol == "socks")
        {
            if (inbound.contains("settings"))
            {
                auto settings = inbound["settings"].toObject();
                if (settings.contains("auth") && settings["auth"].toString() == "password")
                {
                    config["users"] = QJsonArray{
                        QJsonObject{
                            {"username", settings["accounts"].toArray()[0].toObject()["user"].toString()},
                            {"password", settings["accounts"].toArray()[0].toObject()["pass"].toString()}
                        }
                    };
                }
            }
        }
        else if (protocol == "http")
        {
            if (inbound.contains("settings"))
            {
                auto settings = inbound["settings"].toObject();
                if (settings.contains("accounts"))
                {
                    config["users"] = QJsonArray{
                        QJsonObject{
                            {"username", settings["accounts"].toArray()[0].toObject()["user"].toString()},
                            {"password", settings["accounts"].toArray()[0].toObject()["pass"].toString()}
                        }
                    };
                }
            }
        }
        else if (protocol == "shadowsocks")
        {
            auto settings = inbound["settings"].toObject();
            config["method"] = settings["method"].toString();
            config["password"] = settings["password"].toString();
        }
        else if (protocol == "vmess")
        {
            auto settings = inbound["settings"].toObject();
            QJsonArray users;
            for (const auto &client : settings["clients"].toArray())
            {
                auto clientObj = client.toObject();
                users.append(QJsonObject{
                    {"uuid", clientObj["id"].toString()},
                    {"alter_id", clientObj["alterId"].toInt(0)}
                });
            }
            config["users"] = users;
        }
        else if (protocol == "vless")
        {
            auto settings = inbound["settings"].toObject();
            QJsonArray users;
            for (const auto &client : settings["clients"].toArray())
            {
                auto clientObj = client.toObject();
                users.append(QJsonObject{
                    {"uuid", clientObj["id"].toString()},
                    {"flow", clientObj["flow"].toString()}
                });
            }
            config["users"] = users;
        }
        else if (protocol == "trojan")
        {
            auto settings = inbound["settings"].toObject();
            QJsonArray users;
            for (const auto &client : settings["clients"].toArray())
            {
                auto clientObj = client.toObject();
                users.append(QJsonObject{
                    {"password", clientObj["password"].toString()}
                });
            }
            config["users"] = users;
        }

        // Handle transport (stream settings)
        if (inbound.contains("streamSettings"))
        {
            auto streamSettings = inbound["streamSettings"].toObject();
            QString network = streamSettings["network"].toString("tcp");
            
            if (network == "tcp")
            {
                config["transport"] = QJsonObject{{"type", "tcp"}};
            }
            else if (network == "ws")
            {
                auto wsSettings = streamSettings["wsSettings"].toObject();
                config["transport"] = QJsonObject{
                    {"type", "ws"},
                    {"path", wsSettings["path"].toString("/")},
                    {"headers", wsSettings["headers"].toObject()}
                };
            }
            else if (network == "h2")
            {
                auto httpSettings = streamSettings["httpSettings"].toObject();
                config["transport"] = QJsonObject{
                    {"type", "http"},
                    {"host", httpSettings["host"].toArray()},
                    {"path", httpSettings["path"].toString("/")}
                };
            }
            else if (network == "grpc")
            {
                auto grpcSettings = streamSettings["grpcSettings"].toObject();
                config["transport"] = QJsonObject{
                    {"type", "grpc"},
                    {"service_name", grpcSettings["serviceName"].toString()}
                };
            }

            // TLS settings
            if (streamSettings.contains("security"))
            {
                QString security = streamSettings["security"].toString();
                if (security == "tls")
                {
                    auto tlsSettings = streamSettings["tlsSettings"].toObject();
                    config["tls"] = QJsonObject{
                        {"enabled", true},
                        {"server_name", tlsSettings["serverName"].toString()},
                        {"insecure", tlsSettings["allowInsecure"].toBool(false)}
                    };
                    
                    if (tlsSettings.contains("certificates"))
                    {
                        auto certs = tlsSettings["certificates"].toArray();
                        if (!certs.isEmpty())
                        {
                            auto cert = certs[0].toObject();
                            config["tls"].toObject()["certificate_path"] = cert["certificateFile"].toString();
                            config["tls"].toObject()["key_path"] = cert["keyFile"].toString();
                        }
                    }
                }
                else if (security == "reality")
                {
                    auto realitySettings = streamSettings["realitySettings"].toObject();
                    QString realityConfig = GenerateRealityConfig(realitySettings);
                    if (!realityConfig.isEmpty())
                    {
                        config["tls"] = QJsonDocument::fromJson(realityConfig.toUtf8()).object();
                    }
                }
            }
        }
        
        return JsonToString(config);
    }

    QString SingBoxKernelAdapter::GenerateSingBoxOutbound(const OUTBOUND &outbound)
    {
        QJsonObject config;
        QString protocol = outbound["protocol"].toString();
        
        config["tag"] = outbound["tag"].toString();
        config["type"] = protocol;
        
        if (protocol == "direct")
        {
            // Direct connection
            if (outbound.contains("settings"))
            {
                auto settings = outbound["settings"].toObject();
                if (settings.contains("domainStrategy"))
                {
                    config["domain_strategy"] = settings["domainStrategy"].toString();
                }
            }
        }
        else if (protocol == "block")
        {
            // Block connection - no additional settings needed
        }
        else if (protocol == "shadowsocks")
        {
            auto settings = outbound["settings"].toObject();
            auto servers = settings["servers"].toArray();
            if (!servers.isEmpty())
            {
                auto server = servers[0].toObject();
                config["server"] = server["address"].toString();
                config["server_port"] = server["port"].toInt();
                config["method"] = server["method"].toString();
                config["password"] = server["password"].toString();
            }
        }
        else if (protocol == "vmess")
        {
            auto settings = outbound["settings"].toObject();
            auto vnext = settings["vnext"].toArray();
            if (!vnext.isEmpty())
            {
                auto server = vnext[0].toObject();
                config["server"] = server["address"].toString();
                config["server_port"] = server["port"].toInt();
                
                auto users = server["users"].toArray();
                if (!users.isEmpty())
                {
                    auto user = users[0].toObject();
                    config["uuid"] = user["id"].toString();
                    config["alter_id"] = user["alterId"].toInt(0);
                    config["security"] = user["security"].toString("auto");
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
                config["server"] = server["address"].toString();
                config["server_port"] = server["port"].toInt();
                
                auto users = server["users"].toArray();
                if (!users.isEmpty())
                {
                    auto user = users[0].toObject();
                    config["uuid"] = user["id"].toString();
                    config["flow"] = user["flow"].toString();
                }
            }
        }
        else if (protocol == "trojan")
        {
            auto settings = outbound["settings"].toObject();
            auto servers = settings["servers"].toArray();
            if (!servers.isEmpty())
            {
                auto server = servers[0].toObject();
                config["server"] = server["address"].toString();
                config["server_port"] = server["port"].toInt();
                config["password"] = server["password"].toString();
            }
        }
        else if (protocol == "socks")
        {
            auto settings = outbound["settings"].toObject();
            auto servers = settings["servers"].toArray();
            if (!servers.isEmpty())
            {
                auto server = servers[0].toObject();
                config["server"] = server["address"].toString();
                config["server_port"] = server["port"].toInt();
                config["version"] = "5";
                
                if (server.contains("users"))
                {
                    auto users = server["users"].toArray();
                    if (!users.isEmpty())
                    {
                        auto user = users[0].toObject();
                        config["username"] = user["user"].toString();
                        config["password"] = user["pass"].toString();
                    }
                }
            }
        }
        else if (protocol == "http")
        {
            auto settings = outbound["settings"].toObject();
            auto servers = settings["servers"].toArray();
            if (!servers.isEmpty())
            {
                auto server = servers[0].toObject();
                config["server"] = server["address"].toString();
                config["server_port"] = server["port"].toInt();
                
                if (server.contains("users"))
                {
                    auto users = server["users"].toArray();
                    if (!users.isEmpty())
                    {
                        auto user = users[0].toObject();
                        config["username"] = user["username"].toString();
                        config["password"] = user["password"].toString();
                    }
                }
            }
        }
        else if (protocol == "anylts")
        {
            // ANYLTS protocol (sing-box specific)
            auto settings = outbound["settings"].toObject();
            QString anyltsConfig = GenerateAnyltsConfig(settings);
            if (!anyltsConfig.isEmpty())
            {
                QJsonObject anyltsObj = QJsonDocument::fromJson(anyltsConfig.toUtf8()).object();
                for (auto it = anyltsObj.begin(); it != anyltsObj.end(); ++it)
                {
                    config[it.key()] = it.value();
                }
            }
        }

        // Handle transport (stream settings)
        if (outbound.contains("streamSettings"))
        {
            auto streamSettings = outbound["streamSettings"].toObject();
            QString network = streamSettings["network"].toString("tcp");
            
            if (network != "tcp")
            {
                QJsonObject transport;
                transport["type"] = network;
                
                if (network == "ws")
                {
                    auto wsSettings = streamSettings["wsSettings"].toObject();
                    transport["path"] = wsSettings["path"].toString("/");
                    transport["headers"] = wsSettings["headers"].toObject();
                    if (wsSettings.contains("maxEarlyData"))
                    {
                        transport["max_early_data"] = wsSettings["maxEarlyData"].toInt();
                    }
                }
                else if (network == "h2")
                {
                    auto httpSettings = streamSettings["httpSettings"].toObject();
                    transport["host"] = httpSettings["host"].toArray();
                    transport["path"] = httpSettings["path"].toString("/");
                }
                else if (network == "grpc")
                {
                    auto grpcSettings = streamSettings["grpcSettings"].toObject();
                    transport["service_name"] = grpcSettings["serviceName"].toString();
                }
                
                config["transport"] = transport;
            }

            // TLS settings
            if (streamSettings.contains("security"))
            {
                QString security = streamSettings["security"].toString();
                if (security == "tls")
                {
                    auto tlsSettings = streamSettings["tlsSettings"].toObject();
                    config["tls"] = QJsonObject{
                        {"enabled", true},
                        {"server_name", tlsSettings["serverName"].toString()},
                        {"insecure", tlsSettings["allowInsecure"].toBool(false)}
                    };
                    
                    if (tlsSettings.contains("alpn"))
                    {
                        config["tls"].toObject()["alpn"] = tlsSettings["alpn"].toArray();
                    }
                }
                else if (security == "reality")
                {
                    auto realitySettings = streamSettings["realitySettings"].toObject();
                    QString realityConfig = GenerateRealityConfig(realitySettings);
                    if (!realityConfig.isEmpty())
                    {
                        config["tls"] = QJsonDocument::fromJson(realityConfig.toUtf8()).object();
                    }
                }
            }
        }
        
        return JsonToString(config);
    }

    QString SingBoxKernelAdapter::GenerateSingBoxRoute(const QJsonObject &routing)
    {
        QJsonObject config;
        
        config["auto_detect_interface"] = true;
        config["final"] = routing.value("defaultOutbound").toString("proxy");
        
        QJsonArray rules;
        
        if (routing.contains("rules"))
        {
            auto routingRules = routing["rules"].toArray();
            for (const auto &rule : routingRules)
            {
                auto ruleObj = rule.toObject();
                QString type = ruleObj["type"].toString();
                QString outboundTag = ruleObj["outboundTag"].toString("proxy");
                
                if (type == "field")
                {
                    QJsonObject singboxRule;
                    singboxRule["outbound"] = outboundTag;
                    
                    if (ruleObj.contains("domain"))
                    {
                        QJsonArray domains;
                        QJsonArray domainSuffix;
                        QJsonArray domainKeyword;
                        QJsonArray geosite;
                        
                        for (const auto &domain : ruleObj["domain"].toArray())
                        {
                            QString domainStr = domain.toString();
                            if (domainStr.startsWith("geosite:"))
                            {
                                geosite.append(domainStr.mid(8));
                            }
                            else if (domainStr.startsWith("domain:"))
                            {
                                domainSuffix.append(domainStr.mid(7));
                            }
                            else if (domainStr.startsWith("keyword:"))
                            {
                                domainKeyword.append(domainStr.mid(8));
                            }
                            else
                            {
                                domains.append(domainStr);
                            }
                        }
                        
                        if (!domains.isEmpty()) singboxRule["domain"] = domains;
                        if (!domainSuffix.isEmpty()) singboxRule["domain_suffix"] = domainSuffix;
                        if (!domainKeyword.isEmpty()) singboxRule["domain_keyword"] = domainKeyword;
                        if (!geosite.isEmpty()) singboxRule["geosite"] = geosite;
                    }
                    
                    if (ruleObj.contains("ip"))
                    {
                        QJsonArray ipCidr;
                        QJsonArray geoip;
                        
                        for (const auto &ip : ruleObj["ip"].toArray())
                        {
                            QString ipStr = ip.toString();
                            if (ipStr.startsWith("geoip:"))
                            {
                                geoip.append(ipStr.mid(6));
                            }
                            else
                            {
                                ipCidr.append(ipStr);
                            }
                        }
                        
                        if (!ipCidr.isEmpty()) singboxRule["ip_cidr"] = ipCidr;
                        if (!geoip.isEmpty()) singboxRule["geoip"] = geoip;
                    }
                    
                    if (ruleObj.contains("port"))
                    {
                        singboxRule["port"] = ruleObj["port"].toArray();
                    }
                    
                    if (ruleObj.contains("sourcePort"))
                    {
                        singboxRule["source_port"] = ruleObj["sourcePort"].toArray();
                    }
                    
                    if (ruleObj.contains("network"))
                    {
                        singboxRule["network"] = ruleObj["network"].toString();
                    }
                    
                    if (ruleObj.contains("protocol"))
                    {
                        singboxRule["protocol"] = ruleObj["protocol"].toArray();
                    }
                    
                    if (ruleObj.contains("inboundTag"))
                    {
                        singboxRule["inbound"] = ruleObj["inboundTag"].toArray();
                    }
                    
                    rules.append(singboxRule);
                }
            }
        }
        
        config["rules"] = rules;
        
        return JsonToString(config);
    }

    QString SingBoxKernelAdapter::GenerateSingBoxDNS(const QJsonObject &dns)
    {
        using namespace PolyCore::core::dns;
        
        // Check if it's already in new format
        if (dns.contains("servers") && dns["servers"].isArray())
        {
            // Already in sing-box 1.12.0+ format
            return JsonToString(dns);
        }
        
        // Migrate from legacy format
        SingBoxDNSConfig newDns = SingBoxDNSMigrator::MigrateFromLegacy(dns);
        return JsonToString(newDns.ToJson());
    }

    QString SingBoxKernelAdapter::GenerateRealityConfig(const QJsonObject &reality)
    {
        QJsonObject config;
        
        config["enabled"] = true;
        config["server_name"] = reality.value("serverName").toString();
        config["public_key"] = reality.value("publicKey").toString();
        config["short_id"] = reality.value("shortId").toString();
        
        if (reality.contains("spiderX"))
        {
            config["reality"] = QJsonObject{
                {"enabled", true},
                {"handshake", QJsonObject{
                    {"server", reality.value("dest").toString()},
                    {"server_port", reality.value("destPort").toInt(443)}
                }}
            };
        }
        
        return JsonToString(config);
    }

    QString SingBoxKernelAdapter::GenerateAnyltsConfig(const QJsonObject &anylts)
    {
        using namespace PolyCore::core::protocols;
        
        AnyltsConfig config;
        config.server = anylts["server"].toString();
        config.port = anylts["port"].toInt();
        config.password = anylts["password"].toString();
        config.method = anylts["method"].toString("aes-256-gcm");
        config.udp = anylts["udp"].toBool(false);
        config.obfs = anylts["obfs"].toString();
        config.obfsPassword = anylts["obfs-password"].toString();
        config.plugin = anylts["plugin"].toString();
        config.pluginOpts = anylts["plugin-opts"].toObject();
        
        return JsonToString(config.ToJson());
    }

} // namespace PolyCore::core::kernel