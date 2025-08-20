#pragma once

#include "base/Qv2rayBase.hpp"
#include <QObject>
#include <memory>

namespace PolyCore::core::kernel
{
    // Abstract interface for all kernel adapters
    class IPolyCoreKernel
    {
    public:
        virtual ~IPolyCoreKernel() = default;
        
        // Core functionality
        virtual QString GenerateConfig(const CONFIGROOT &root) = 0;
        virtual QStringList GetStartupArguments(const QString &configPath) = 0;
        virtual bool ValidateConfig(const QString &configPath) = 0;
        
        // Kernel information  
        virtual QString GetKernelName() const = 0;
        virtual QString GetVersion() const = 0;
        virtual QStringList GetSupportedProtocols() const = 0;
        virtual QString GetConfigFormat() const = 0; // "json" or "yaml"
        
        // Protocol support detection
        virtual bool SupportsProtocol(const QString &protocol) const = 0;
        virtual bool SupportsReality() const = 0;
        virtual bool SupportsAnylts() const = 0;
        
        // Log parsing
        virtual QString ParseKernelLog(const QString &rawLog) = 0;
        virtual bool IsKernelReady(const QString &log) = 0;
        virtual QString ExtractErrorMessage(const QString &log) = 0;
        
        // Stats extraction (if supported)
        virtual bool SupportsStats() const = 0;
        virtual QMap<StatisticsType, QvStatsSpeed> ParseStatsOutput(const QString &output) = 0;
    };

    // V2Ray/Xray kernel adapter
    class V2RayKernelAdapter : public IPolyCoreKernel
    {
    public:
        explicit V2RayKernelAdapter(bool isXray = false);
        
        QString GenerateConfig(const CONFIGROOT &root) override;
        QStringList GetStartupArguments(const QString &configPath) override;
        bool ValidateConfig(const QString &configPath) override;
        
        QString GetKernelName() const override { return isXray ? "Xray" : "V2Ray"; }
        QString GetVersion() const override;
        QStringList GetSupportedProtocols() const override;
        QString GetConfigFormat() const override { return "json"; }
        
        bool SupportsProtocol(const QString &protocol) const override;
        bool SupportsReality() const override { return isXray; }
        bool SupportsAnylts() const override { return false; }
        
        QString ParseKernelLog(const QString &rawLog) override;
        bool IsKernelReady(const QString &log) override;
        QString ExtractErrorMessage(const QString &log) override;
        
        bool SupportsStats() const override { return true; }
        QMap<StatisticsType, QvStatsSpeed> ParseStatsOutput(const QString &output) override;
        
    private:
        bool isXray;
        QString GenerateV2RayInbound(const INBOUND &inbound);
        QString GenerateV2RayOutbound(const OUTBOUND &outbound);
        QString GenerateV2RayRouting(const QJsonObject &routing);
        QString GenerateRealityConfig(const QJsonObject &reality);
    };

    // Mihomo (Clash) kernel adapter  
    class MihomoKernelAdapter : public IPolyCoreKernel
    {
    public:
        MihomoKernelAdapter();
        
        QString GenerateConfig(const CONFIGROOT &root) override;
        QStringList GetStartupArguments(const QString &configPath) override;
        bool ValidateConfig(const QString &configPath) override;
        
        QString GetKernelName() const override { return "Mihomo"; }
        QString GetVersion() const override;
        QStringList GetSupportedProtocols() const override;
        QString GetConfigFormat() const override { return "yaml"; }
        
        bool SupportsProtocol(const QString &protocol) const override;
        bool SupportsReality() const override { return true; }
        bool SupportsAnylts() const override { return false; }
        
        QString ParseKernelLog(const QString &rawLog) override;
        bool IsKernelReady(const QString &log) override;
        QString ExtractErrorMessage(const QString &log) override;
        
        bool SupportsStats() const override { return true; }
        QMap<StatisticsType, QvStatsSpeed> ParseStatsOutput(const QString &output) override;
        
    private:
        QString GenerateClashProxy(const OUTBOUND &outbound);
        QString GenerateClashProxyGroup(const QJsonObject &routing);
        QString GenerateClashRules(const QJsonObject &routing);
        QString ConvertToYaml(const QJsonObject &json);
    };

    // sing-box kernel adapter
    class SingBoxKernelAdapter : public IPolyCoreKernel
    {
    public:
        SingBoxKernelAdapter();
        
        QString GenerateConfig(const CONFIGROOT &root) override;
        QStringList GetStartupArguments(const QString &configPath) override;
        bool ValidateConfig(const QString &configPath) override;
        
        QString GetKernelName() const override { return "sing-box"; }
        QString GetVersion() const override;
        QStringList GetSupportedProtocols() const override;
        QString GetConfigFormat() const override { return "json"; }
        
        bool SupportsProtocol(const QString &protocol) const override;
        bool SupportsReality() const override { return true; }
        bool SupportsAnylts() const override { return true; }
        
        QString ParseKernelLog(const QString &rawLog) override;
        bool IsKernelReady(const QString &log) override;
        QString ExtractErrorMessage(const QString &log) override;
        
        bool SupportsStats() const override { return true; }
        QMap<StatisticsType, QvStatsSpeed> ParseStatsOutput(const QString &output) override;
        
    private:
        QString GenerateSingBoxInbound(const INBOUND &inbound);
        QString GenerateSingBoxOutbound(const OUTBOUND &outbound);
        QString GenerateSingBoxRoute(const QJsonObject &routing);
        QString GenerateSingBoxDNS(const QJsonObject &dns); // New 1.12.0+ format
        QString GenerateRealityConfig(const QJsonObject &reality);
        QString GenerateAnyltsConfig(const QJsonObject &anylts);
        bool isNewDNSFormat = true; // For sing-box 1.12.0+
    };

} // namespace PolyCore::core::kernel