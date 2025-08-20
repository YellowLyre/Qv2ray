#include "ProtocolSupport.hpp"
#include <QHostInfo>
#include <QTcpSocket>
#include <QTimer>

namespace PolyCore::core::protocols
{
    // Reality Configuration Implementation
    QJsonObject RealityConfig::ToJson() const
    {
        QJsonObject obj;
        obj["dest"] = dest;
        obj["destPort"] = destPort;
        obj["serverName"] = serverName;
        obj["publicKey"] = publicKey;
        obj["shortId"] = shortId;
        obj["spiderX"] = spiderX;
        obj["fingerprint"] = fingerprint;
        return obj;
    }

    RealityConfig RealityConfig::FromJson(const QJsonObject &obj)
    {
        RealityConfig config;
        config.dest = obj["dest"].toString();
        config.destPort = obj["destPort"].toInt();
        config.serverName = obj["serverName"].toString();
        config.publicKey = obj["publicKey"].toString();
        config.shortId = obj["shortId"].toString();
        config.spiderX = obj["spiderX"].toString();
        config.fingerprint = obj["fingerprint"].toString();
        return config;
    }

    bool RealityConfig::IsValid() const
    {
        return !dest.isEmpty() && destPort > 0 && destPort < 65536 &&
               !serverName.isEmpty() && !publicKey.isEmpty();
    }

    // Anylts Configuration Implementation
    QJsonObject AnyltsConfig::ToJson() const
    {
        QJsonObject obj;
        obj["type"] = type;
        obj["server"] = server;
        obj["port"] = port;
        obj["password"] = password;
        obj["method"] = method;
        obj["udp"] = udp;
        if (!obfs.isEmpty()) obj["obfs"] = obfs;
        if (!obfsPassword.isEmpty()) obj["obfs-password"] = obfsPassword;
        if (!plugin.isEmpty()) obj["plugin"] = plugin;
        if (!pluginOpts.isEmpty()) obj["plugin-opts"] = pluginOpts;
        return obj;
    }

    AnyltsConfig AnyltsConfig::FromJson(const QJsonObject &obj)
    {
        AnyltsConfig config;
        config.type = obj["type"].toString("anylts");
        config.server = obj["server"].toString();
        config.port = obj["port"].toInt();
        config.password = obj["password"].toString();
        config.method = obj["method"].toString();
        config.udp = obj["udp"].toBool();
        config.obfs = obj["obfs"].toString();
        config.obfsPassword = obj["obfs-password"].toString();
        config.plugin = obj["plugin"].toString();
        config.pluginOpts = obj["plugin-opts"].toObject();
        return config;
    }

    bool AnyltsConfig::IsValid() const
    {
        return !server.isEmpty() && port > 0 && port < 65536 && 
               !password.isEmpty() && !method.isEmpty();
    }

    // Protocol Configuration Generator Implementation
    QJsonObject ProtocolConfigGenerator::GenerateXrayRealityInbound(const RealityConfig &config, const QString &protocol)
    {
        QJsonObject inbound;
        inbound["protocol"] = protocol;
        inbound["port"] = config.destPort;
        
        QJsonObject settings;
        if (protocol == "vless") {
            QJsonObject client;
            client["id"] = ""; // Will be filled by user
            client["flow"] = "xtls-rprx-vision";
            settings["clients"] = QJsonArray{client};
        }
        inbound["settings"] = settings;
        
        QJsonObject streamSettings;
        streamSettings["network"] = "tcp";
        
        QJsonObject realitySettings;
        realitySettings["show"] = false;
        realitySettings["dest"] = QString("%1:%2").arg(config.dest).arg(config.destPort);
        realitySettings["xver"] = 0;
        realitySettings["serverNames"] = QJsonArray{config.serverName};
        realitySettings["privateKey"] = ""; // Server private key
        realitySettings["shortIds"] = QJsonArray{config.shortId};
        
        streamSettings["realitySettings"] = realitySettings;
        streamSettings["security"] = "reality";
        inbound["streamSettings"] = streamSettings;
        
        return inbound;
    }

    QJsonObject ProtocolConfigGenerator::GenerateXrayRealityOutbound(const RealityConfig &config, const QString &protocol)
    {
        QJsonObject outbound;
        outbound["protocol"] = protocol;
        
        QJsonObject settings;
        if (protocol == "vless") {
            QJsonObject vnext;
            vnext["address"] = config.dest;
            vnext["port"] = config.destPort;
            
            QJsonObject user;
            user["id"] = ""; // Will be filled by user
            user["flow"] = "xtls-rprx-vision";
            vnext["users"] = QJsonArray{user};
            
            settings["vnext"] = QJsonArray{vnext};
        }
        outbound["settings"] = settings;
        
        QJsonObject streamSettings;
        streamSettings["network"] = "tcp";
        streamSettings["security"] = "reality";
        
        QJsonObject realitySettings;
        realitySettings["serverName"] = config.serverName;
        realitySettings["fingerprint"] = config.fingerprint;
        realitySettings["publicKey"] = config.publicKey;
        realitySettings["shortId"] = config.shortId;
        realitySettings["spiderX"] = config.spiderX;
        
        streamSettings["realitySettings"] = realitySettings;
        outbound["streamSettings"] = streamSettings;
        
        return outbound;
    }

    QJsonObject ProtocolConfigGenerator::GenerateSingBoxRealityOutbound(const RealityConfig &config)
    {
        QJsonObject outbound;
        outbound["type"] = "vless";
        outbound["server"] = config.dest;
        outbound["server_port"] = config.destPort;
        outbound["uuid"] = ""; // Will be filled by user
        outbound["flow"] = "xtls-rprx-vision";
        
        QJsonObject tls;
        tls["enabled"] = true;
        tls["server_name"] = config.serverName;
        tls["utls"] = QJsonObject{{"enabled", true}, {"fingerprint", config.fingerprint}};
        
        QJsonObject reality;
        reality["enabled"] = true;
        reality["public_key"] = config.publicKey;
        reality["short_id"] = config.shortId;
        
        tls["reality"] = reality;
        outbound["tls"] = tls;
        
        return outbound;
    }

    QJsonObject ProtocolConfigGenerator::GenerateSingBoxAnyltsOutbound(const AnyltsConfig &config)
    {
        QJsonObject outbound;
        outbound["type"] = "anylts";
        outbound["server"] = config.server;
        outbound["server_port"] = config.port;
        outbound["password"] = config.password;
        outbound["method"] = config.method;
        
        if (config.udp) {
            outbound["udp_relay"] = true;
        }
        
        if (!config.obfs.isEmpty()) {
            QJsonObject obfsSettings;
            obfsSettings["type"] = config.obfs;
            if (!config.obfsPassword.isEmpty()) {
                obfsSettings["password"] = config.obfsPassword;
            }
            outbound["obfs"] = obfsSettings;
        }
        
        if (!config.plugin.isEmpty()) {
            outbound["plugin"] = config.plugin;
            if (!config.pluginOpts.isEmpty()) {
                outbound["plugin_opts"] = config.pluginOpts;
            }
        }
        
        return outbound;
    }

    bool ProtocolConfigGenerator::IsRealitySupported(const QString &kernelType, const QString &protocol)
    {
        if (kernelType == "v2ray") return false; // V2Ray doesn't support Reality
        if (kernelType == "xray" && REALITY_PROTOCOLS.contains(protocol)) return true;
        if (kernelType == "mihomo" && protocol == "vless") return true;
        if (kernelType == "sing-box" && REALITY_PROTOCOLS.contains(protocol)) return true;
        return false;
    }

    bool ProtocolConfigGenerator::IsAnyltsSupported(const QString &kernelType)
    {
        return kernelType == "sing-box";
    }

    QStringList ProtocolConfigGenerator::GetSupportedRealityProtocols(const QString &kernelType)
    {
        if (kernelType == "xray" || kernelType == "sing-box") {
            return REALITY_PROTOCOLS;
        } else if (kernelType == "mihomo") {
            return {"vless"};
        }
        return {};
    }

    // Protocol Validator Implementation
    bool ProtocolValidator::ValidateRealityConfig(const RealityConfig &config)
    {
        if (!config.IsValid()) return false;
        
        // Validate public key format (base64)
        if (!IsPublicKeyValid(config.publicKey)) return false;
        
        // Validate short ID format (hex string, 0-16 chars)
        if (!IsShortIdValid(config.shortId)) return false;
        
        // Validate fingerprint
        QStringList validFingerprints = {"chrome", "firefox", "safari", "edge", "qq", "random", "randomized"};
        if (!config.fingerprint.isEmpty() && !validFingerprints.contains(config.fingerprint)) {
            return false;
        }
        
        return true;
    }

    bool ProtocolValidator::ValidateAnyltsConfig(const AnyltsConfig &config)
    {
        if (!config.IsValid()) return false;
        
        // Validate encryption methods
        QStringList validMethods = {"aes-128-gcm", "aes-256-gcm", "chacha20-poly1305", "none"};
        if (!validMethods.contains(config.method)) return false;
        
        // Validate obfuscation methods if specified
        if (!config.obfs.isEmpty()) {
            QStringList validObfs = {"http", "tls", "random", "none"};
            if (!validObfs.contains(config.obfs)) return false;
        }
        
        return true;
    }

    bool ProtocolValidator::IsPublicKeyValid(const QString &publicKey)
    {
        if (publicKey.length() != 44) return false; // Base64 encoded 32-byte key
        
        QRegularExpression base64Regex("^[A-Za-z0-9+/]{43}=$");
        return base64Regex.match(publicKey).hasMatch();
    }

    bool ProtocolValidator::IsShortIdValid(const QString &shortId)
    {
        if (shortId.length() > 16) return false;
        
        QRegularExpression hexRegex("^[0-9a-fA-F]*$");
        return hexRegex.match(shortId).hasMatch();
    }

    bool ProtocolValidator::IsDestinationReachable(const QString &dest, int port)
    {
        QTcpSocket socket;
        socket.connectToHost(dest, port);
        return socket.waitForConnected(3000); // 3 second timeout
    }

} // namespace PolyCore::core::protocols