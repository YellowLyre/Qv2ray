#pragma once

#include "base/Qv2rayBase.hpp"
#include <QJsonObject>
#include <QString>

namespace PolyCore::core::protocols
{
    // Reality Protocol Support
    struct RealityConfig
    {
        QString dest;           // Destination address
        int destPort;           // Destination port
        QString serverName;     // SNI server name
        QString publicKey;      // Reality public key
        QString shortId;        // Short ID
        QString spiderX;        // Spider X parameter
        QString fingerprint;    // TLS fingerprint
        
        QJsonObject ToJson() const;
        static RealityConfig FromJson(const QJsonObject &obj);
        bool IsValid() const;
    };

    // Anylts Protocol Support (sing-box specific)
    struct AnyltsConfig
    {
        QString type = "anylts";
        QString server;         // Server address
        int port;              // Server port
        QString password;       // Authentication password
        QString method;         // Encryption method
        bool udp = false;       // UDP relay support
        
        // Anylts specific settings
        QString obfs;           // Obfuscation method
        QString obfsPassword;   // Obfuscation password
        QString plugin;         // Plugin configuration
        QJsonObject pluginOpts; // Plugin options
        
        QJsonObject ToJson() const;
        static AnyltsConfig FromJson(const QJsonObject &obj);
        bool IsValid() const;
    };

    // Protocol Configuration Generator
    class ProtocolConfigGenerator
    {
    public:
        // Reality configuration generators
        static QJsonObject GenerateXrayRealityInbound(const RealityConfig &config, const QString &protocol = "vless");
        static QJsonObject GenerateXrayRealityOutbound(const RealityConfig &config, const QString &protocol = "vless");
        static QJsonObject GenerateMihomoRealityProxy(const RealityConfig &config);
        static QJsonObject GenerateSingBoxRealityInbound(const RealityConfig &config);
        static QJsonObject GenerateSingBoxRealityOutbound(const RealityConfig &config);
        
        // Anylts configuration generators (sing-box only)
        static QJsonObject GenerateSingBoxAnyltsInbound(const AnyltsConfig &config);
        static QJsonObject GenerateSingBoxAnyltsOutbound(const AnyltsConfig &config);
        
        // Protocol detection and validation
        static bool IsRealitySupported(const QString &kernelType, const QString &protocol);
        static bool IsAnyltsSupported(const QString &kernelType);
        static QStringList GetSupportedRealityProtocols(const QString &kernelType);
        
        // Configuration migration helpers
        static RealityConfig MigrateFromV2RayReality(const QJsonObject &v2rayConfig);
        static QJsonObject MigrateToTargetKernel(const QJsonObject &sourceConfig, 
                                                  const QString &sourceKernel, 
                                                  const QString &targetKernel);
    };

    // Protocol Validator
    class ProtocolValidator
    {
    public:
        static bool ValidateRealityConfig(const RealityConfig &config);
        static bool ValidateAnyltsConfig(const AnyltsConfig &config);
        static QString GetValidationError(const QString &protocol, const QJsonObject &config);
        static bool IsDestinationReachable(const QString &dest, int port);
        static bool IsPublicKeyValid(const QString &publicKey);
        static bool IsShortIdValid(const QString &shortId);
    };

} // namespace PolyCore::core::protocols

// Convenience macros for protocol detection
#define SUPPORTS_REALITY(kernel) \
    (kernel == "xray" || kernel == "mihomo" || kernel == "sing-box")

#define SUPPORTS_ANYLTS(kernel) \
    (kernel == "sing-box")

#define REALITY_PROTOCOLS \
    QStringList{"vless", "vmess", "trojan"}

#define ANYLTS_PROTOCOLS \
    QStringList{"anylts"}