#pragma once

#include "base/Qv2rayBase.hpp"
#include "PolyCoreKernelInterface.hpp"

#include <QObject>
#include <QProcess>
#include <memory>
#include <optional>

namespace PolyCore::core::kernel
{
    enum class KernelType
    {
        V2RAY,
        XRAY, 
        MIHOMO,
        SING_BOX
    };

    struct KernelInfo
    {
        KernelType type;
        QString name;
        QString executablePath;
        QString version;
        QStringList supportedProtocols;
        bool supportsReality = false;
        bool supportsAnylts = false;
        QString configFormat; // "json" or "yaml"
    };

    class PolyCoreKernelManager : public QObject
    {
        Q_OBJECT

    public:
        explicit PolyCoreKernelManager(QObject *parent = nullptr);
        ~PolyCoreKernelManager() override;

        // Kernel management
        void RegisterKernel(KernelType type, const QString &executablePath);
        void UnregisterKernel(KernelType type);
        bool IsKernelAvailable(KernelType type) const;
        QList<KernelInfo> GetAvailableKernels() const;
        
        // Connection management
        std::optional<QString> StartConnection(const ConnectionGroupPair &id, const CONFIGROOT &root, KernelType preferredKernel = KernelType::V2RAY);
        void StopConnection();
        bool IsConnectionRunning() const;
        KernelType GetCurrentKernelType() const;
        
        // Configuration validation
        std::optional<QString> ValidateConfig(const CONFIGROOT &root, KernelType kernelType);
        static std::pair<bool, std::optional<QString>> ValidateKernel(const QString &kernelPath, KernelType type);
        
        // Kernel detection and auto-discovery
        void AutoDetectKernels();
        QString GetKernelVersion(KernelType type) const;
        // Helpers for testing and utilities
        static QString ParseVersionFromOutput(KernelType type, const QString &outputText);
        static KernelType ComputePreferredKernel(KernelType preferred, const QSet<KernelType> &available);

        // Preferred kernel management (for UI integration)
        void SetPreferredKernel(KernelType type) { preferredKernel = type; }
        KernelType GetPreferredKernel() const { return preferredKernel; }
        void SetPreferredKernelByName(const QString &name) { preferredKernel = KernelTypeFromString(name); }
        static KernelType KernelTypeFromString(const QString &name)
        {
            const auto n = name.toLower();
            if (n == "sing-box" || n == "singbox" || n == "sing_box") return KernelType::SING_BOX;
            if (n == "mihomo" || n == "clash" || n == "clash.meta" || n == "clash-meta") return KernelType::MIHOMO;
            if (n == "xray") return KernelType::XRAY;
            return KernelType::V2RAY;
        }
        static QString KernelTypeToString(KernelType t)
        {
            switch (t)
            {
                case KernelType::SING_BOX: return "sing-box";
                case KernelType::MIHOMO: return "mihomo";
                case KernelType::XRAY: return "xray";
                case KernelType::V2RAY: default: return "v2ray";
            }
        }

    signals:
        void OnKernelRegistered(KernelType type);
        void OnKernelUnregistered(KernelType type);
        void OnConnectionStarted(const ConnectionGroupPair &id, KernelType kernelType);
        void OnConnectionStopped(const ConnectionGroupPair &id);
        void OnConnectionError(const ConnectionGroupPair &id, const QString &error);
        void OnStatsDataAvailable(const ConnectionGroupPair &id, const QMap<StatisticsType, QvStatsSpeed> &data);
        void OnKernelLogAvailable(const ConnectionGroupPair &id, const QString &log);

    private slots:
        void OnKernelFinished(int exitCode, QProcess::ExitStatus exitStatus);
        void OnKernelErrorOccurred(QProcess::ProcessError error);
        void OnKernelReadyReadStandardOutput();
        void OnKernelReadyReadStandardError();

    private:
        std::unique_ptr<IPolyCoreKernel> CreateKernelAdapter(KernelType type);
        QString GenerateConfigForKernel(const CONFIGROOT &root, KernelType kernelType);
        void SetupKernelProcess(KernelType type);
        QString GetDefaultKernelPath(KernelType type) const;
        QStringList GetKernelStartArgs(KernelType type, const QString &configPath) const;
        QString QueryKernelVersion(const QString &kernelPath, KernelType type) const;
        KernelType ChooseAvailableKernel(KernelType preferred) const;

    private:
        QMap<KernelType, KernelInfo> availableKernels;
        std::unique_ptr<IPolyCoreKernel> currentKernel;
        QProcess *kernelProcess = nullptr;
        KernelType currentKernelType = KernelType::V2RAY;
        KernelType preferredKernel = KernelType::SING_BOX; // default preference
        ConnectionGroupPair currentConnectionId;
        QString currentConfigPath;
        bool connectionRunning = false;
    };

} // namespace PolyCore::core::kernel

using namespace PolyCore::core::kernel;