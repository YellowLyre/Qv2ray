#include "PolyCoreKernelInterface.hpp"
#include "base/Qv2rayBase.hpp"
#include "core/connection/Generation.hpp"
#include "utils/QvHelpers.hpp"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>

#define QV_MODULE_NAME "KernelAdapter"

namespace PolyCore::core::kernel
{
    // V2RayKernelAdapter Implementation
    
    V2RayKernelAdapter::V2RayKernelAdapter(bool isXray) : isXray(isXray)
    {
        DEBUG("Created", isXray ? "Xray" : "V2Ray", "kernel adapter");
    }

    QString V2RayKernelAdapter::GenerateConfig(const CONFIGROOT &root)
    {
        try
        {
            QJsonObject config;
            
            // Basic structure
            config["log"] = QJsonObject{
                {"loglevel", "warning"},
                {"access", ""},
                {"error", ""}
            };
            
            // API configuration
            config["api"] = QJsonObject{
                {"tag", "api"},
                {"services", QJsonArray{"HandlerService", "LoggerService", "StatsService"}}
            };
            
            // Statistics
            config["stats"] = QJsonObject{
                {"statsOutboundUplink", true},
                {"statsOutboundDownlink", true}
            };
            
            // Policy
            config["policy"] = QJsonObject{
                {"levels", QJsonObject{
                    {"0", QJsonObject{
                        {"statsUserUplink", true},
                        {"statsUserDownlink", true}
                    }}
                }},
                {"system", QJsonObject{
                    {"statsInboundUplink", true},
                    {"statsInboundDownlink", true}
                }}
            };

            // Generate inbounds
            QJsonArray inbounds;
            for (const auto &inbound : root["inbounds"].toArray())
            {
                QString inboundConfig = GenerateV2RayInbound(INBOUND(inbound.toObject()));
                if (!inboundConfig.isEmpty())
                {
                    inbounds.append(QJsonDocument::fromJson(inboundConfig.toUtf8()).object());
                }
            }
            
            // Add API inbound
            inbounds.append(QJsonObject{
                {"tag", "api"},
                {"port", 15490},
                {"protocol", "dokodemo-door"},
                {"settings", QJsonObject{
                    {"address", "127.0.0.1"}
                }}
            });
            
            config["inbounds"] = inbounds;

            // Generate outbounds  
            QJsonArray outbounds;
            for (const auto &outbound : root["outbounds"].toArray())
            {
                QString outboundConfig = GenerateV2RayOutbound(OUTBOUND(outbound.toObject()));
                if (!outboundConfig.isEmpty())
                {
                    outbounds.append(QJsonDocument::fromJson(outboundConfig.toUtf8()).object());
                }
            }
            
            config["outbounds"] = outbounds;

            // Generate routing
            if (root.contains("routing"))
            {
                QString routingConfig = GenerateV2RayRouting(root["routing"].toObject());
                if (!routingConfig.isEmpty())
                {
                    config["routing"] = QJsonDocument::fromJson(routingConfig.toUtf8()).object();
                }
            }

            // DNS configuration
            if (root.contains("dns"))
            {
                config["dns"] = root["dns"].toObject();
            }

            return JsonToString(config);
        }
        catch (const std::exception &e)
        {
            LOG("Error generating V2Ray config:", e.what());
            return QString();
        }
    }

    QStringList V2RayKernelAdapter::GetStartupArguments(const QString &configPath)
    {
        return QStringList() << "run" << "-c" << configPath;
    }

    bool V2RayKernelAdapter::ValidateConfig(const QString &configPath)
    {
        // Use kernel's built-in validation
        QProcess process;
        QStringList args = QStringList() << "test" << "-c" << configPath;
        
        process.start("", args); // Path will be filled by the manager
        process.waitForFinished(5000);
        
        return process.exitCode() == 0;
    }

    QString V2RayKernelAdapter::GetVersion() const
    {
        // This will be called after the executable path is known
        // For now, return a placeholder
        return "Unknown";
    }

    QStringList V2RayKernelAdapter::GetSupportedProtocols() const
    {
        QStringList protocols = {"vmess", "vless", "shadowsocks", "trojan", "socks", "http"};
        
        if (isXray)
        {
            protocols << "xtls";
        }
        
        return protocols;
    }

    bool V2RayKernelAdapter::SupportsProtocol(const QString &protocol) const
    {
        return GetSupportedProtocols().contains(protocol.toLower());
    }

    QString V2RayKernelAdapter::ParseKernelLog(const QString &rawLog)
    {
        // Parse V2Ray/Xray log format: [timestamp] [level] [module] message
        QStringList lines = rawLog.split('\n', Qt::SkipEmptyParts);
        QStringList parsedLines;
        
        for (const QString &line : lines)
        {
            // Remove ANSI color codes
            QString cleanLine = line;
            cleanLine.remove(QRegularExpression("\\x1B\\[[0-9;]*m"));
            
            // Parse log level and message
            QRegularExpression logRegex(R"(^\[([^\]]+)\]\s+\[([^\]]+)\]\s+\[([^\]]+)\]\s+(.+)$)");
            QRegularExpressionMatch match = logRegex.match(cleanLine);
            
            if (match.hasMatch())
            {
                QString timestamp = match.captured(1);
                QString level = match.captured(2);
                QString module = match.captured(3);
                QString message = match.captured(4);
                
                parsedLines.append(QString("[%1] %2: %3").arg(level, module, message));
            }
            else
            {
                parsedLines.append(cleanLine);
            }
        }
        
        return parsedLines.join('\n');
    }

    bool V2RayKernelAdapter::IsKernelReady(const QString &log)
    {
        return log.contains("started", Qt::CaseInsensitive) && 
               (log.contains("listening", Qt::CaseInsensitive) || 
                log.contains("serving", Qt::CaseInsensitive));
    }

    QString V2RayKernelAdapter::ExtractErrorMessage(const QString &log)
    {
        QStringList lines = log.split('\n', Qt::SkipEmptyParts);
        
        for (const QString &line : lines)
        {
            if (line.contains("error", Qt::CaseInsensitive) || 
                line.contains("failed", Qt::CaseInsensitive) ||
                line.contains("panic", Qt::CaseInsensitive))
            {
                return line.trimmed();
            }
        }
        
        return QString();
    }

    QMap<StatisticsType, QvStatsSpeed> V2RayKernelAdapter::ParseStatsOutput(const QString &output)
    {
        QMap<StatisticsType, QvStatsSpeed> stats;
        
        // Parse V2Ray stats format
        QRegularExpression statsRegex(R"(stat:\s*<\s*name:\s*"([^"]+)"\s*value:\s*(\d+)\s*>)");
        QRegularExpressionMatchIterator i = statsRegex.globalMatch(output);
        
        while (i.hasNext())
        {
            QRegularExpressionMatch match = i.next();
            QString statName = match.captured(1);
            qint64 value = match.captured(2).toLongLong();
            
            if (statName.contains("uplink"))
            {
                QvStatsSpeed speed;
                speed.first = value; // Total bytes
                stats[StatisticsType::ALL_PROXY] = speed;
            }
            else if (statName.contains("downlink"))
            {
                if (stats.contains(StatisticsType::ALL_PROXY))
                {
                    stats[StatisticsType::ALL_PROXY].second = value;
                }
            }
        }
        
        return stats;
    }

    // Private helper methods

    QString V2RayKernelAdapter::GenerateV2RayInbound(const INBOUND &inbound)
    {
        QJsonObject config;
        
        config["tag"] = inbound["tag"].toString();
        config["port"] = inbound["port"].toInt();
        config["protocol"] = inbound["protocol"].toString();
        config["listen"] = inbound["listen"].toString("127.0.0.1");
        
        if (inbound.contains("settings"))
        {
            config["settings"] = inbound["settings"].toObject();
        }
        
        if (inbound.contains("streamSettings"))
        {
            config["streamSettings"] = inbound["streamSettings"].toObject();
        }
        
        if (inbound.contains("sniffing"))
        {
            config["sniffing"] = inbound["sniffing"].toObject();
        }
        
        return JsonToString(config);
    }

    QString V2RayKernelAdapter::GenerateV2RayOutbound(const OUTBOUND &outbound)
    {
        QJsonObject config;
        
        config["tag"] = outbound["tag"].toString();
        config["protocol"] = outbound["protocol"].toString();
        
        if (outbound.contains("settings"))
        {
            config["settings"] = outbound["settings"].toObject();
        }
        
        if (outbound.contains("streamSettings"))
        {
            QJsonObject streamSettings = outbound["streamSettings"].toObject();
            
            // Handle Reality configuration for Xray
            if (isXray && streamSettings.contains("realitySettings"))
            {
                QString realityConfig = GenerateRealityConfig(streamSettings["realitySettings"].toObject());
                if (!realityConfig.isEmpty())
                {
                    streamSettings["realitySettings"] = QJsonDocument::fromJson(realityConfig.toUtf8()).object();
                }
            }
            
            config["streamSettings"] = streamSettings;
        }
        
        if (outbound.contains("proxySettings"))
        {
            config["proxySettings"] = outbound["proxySettings"].toObject();
        }
        
        return JsonToString(config);
    }

    QString V2RayKernelAdapter::GenerateV2RayRouting(const QJsonObject &routing)
    {
        QJsonObject config;
        
        config["domainStrategy"] = routing.value("domainStrategy").toString("IPIfNonMatch");
        
        if (routing.contains("rules"))
        {
            config["rules"] = routing["rules"].toArray();
        }
        
        if (routing.contains("balancers"))
        {
            config["balancers"] = routing["balancers"].toArray();
        }
        
        return JsonToString(config);
    }

    QString V2RayKernelAdapter::GenerateRealityConfig(const QJsonObject &reality)
    {
        if (!isXray)
        {
            return QString(); // Reality only supported by Xray
        }
        
        QJsonObject config;
        
        config["show"] = reality.value("show").toBool(false);
        config["dest"] = reality.value("dest").toString();
        config["xver"] = reality.value("xver").toInt(0);
        config["serverNames"] = reality.value("serverNames").toArray();
        config["privateKey"] = reality.value("privateKey").toString();
        config["minClientVer"] = reality.value("minClientVer").toString("");
        config["maxClientVer"] = reality.value("maxClientVer").toString("");
        config["maxTimeDiff"] = reality.value("maxTimeDiff").toInt(0);
        config["shortIds"] = reality.value("shortIds").toArray();
        
        return JsonToString(config);
    }

} // namespace PolyCore::core::kernel