#include "PolyCoreKernelManager.hpp"
#include "PolyCoreKernelInterface.hpp"
#include "base/Qv2rayBase.hpp"
#include "utils/QvHelpers.hpp"

#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTimer>
#include <QRegularExpression>

#define QV_MODULE_NAME "KernelManager"

namespace PolyCore::core::kernel
{
    PolyCoreKernelManager::PolyCoreKernelManager(QObject *parent)
        : QObject(parent)
        , currentKernel(nullptr)
        , kernelProcess(nullptr)
        , currentKernelType(KernelType::V2RAY)
        , connectionRunning(false)
    {
        DEBUG("PolyCoreKernelManager initialized");
    }

    PolyCoreKernelManager::~PolyCoreKernelManager()
    {
        if (connectionRunning)
        {
            StopConnection();
        }
        
        for (auto &kernelInfo : availableKernels)
        {
            DEBUG("Unregistering kernel:", kernelInfo.name);
        }
    }

    void PolyCoreKernelManager::RegisterKernel(KernelType type, const QString &executablePath)
    {
        if (!QFileInfo(executablePath).isExecutable())
        {
            LOG("Kernel executable not found or not executable:", executablePath);
            return;
        }

        auto [isValid, errorMessage] = ValidateKernel(executablePath, type);
        if (!isValid)
        {
            LOG("Kernel validation failed:", errorMessage.value_or("Unknown error"));
            return;
        }

        KernelInfo info;
        info.type = type;
        info.executablePath = executablePath;
        
        auto adapter = CreateKernelAdapter(type);
        if (adapter)
        {
            info.name = adapter->GetKernelName();
            info.version = QueryKernelVersion(executablePath, type);
            info.supportedProtocols = adapter->GetSupportedProtocols();
            info.supportsReality = adapter->SupportsReality();
            info.supportsAnylts = adapter->SupportsAnylts();
            info.configFormat = adapter->GetConfigFormat();
        }

        availableKernels[type] = info;
        
        DEBUG("Registered kernel:", info.name, "version:", info.version);
        DEBUG("Supported protocols:", info.supportedProtocols.join(", "));
        DEBUG("Reality support:", info.supportsReality ? "Yes" : "No");
        DEBUG("Anylts support:", info.supportsAnylts ? "Yes" : "No");
        
        emit OnKernelRegistered(type);
    }

    void PolyCoreKernelManager::UnregisterKernel(KernelType type)
    {
        if (availableKernels.contains(type))
        {
            // Stop connection if using this kernel
            if (connectionRunning && currentKernelType == type)
            {
                StopConnection();
            }
            
            DEBUG("Unregistering kernel:", availableKernels[type].name);
            availableKernels.remove(type);
            emit OnKernelUnregistered(type);
        }
    }

    bool PolyCoreKernelManager::IsKernelAvailable(KernelType type) const
    {
        return availableKernels.contains(type);
    }

    QList<KernelInfo> PolyCoreKernelManager::GetAvailableKernels() const
    {
        return availableKernels.values();
    }

    std::optional<QString> PolyCoreKernelManager::StartConnection(const ConnectionGroupPair &id, const CONFIGROOT &root, KernelType preferredKernel)
    {
        if (connectionRunning)
        {
            return "A connection is already running. Stop it first.";
        }

        // Choose kernel by preference and availability
        preferredKernel = ChooseAvailableKernel(preferredKernel);
        if (!IsKernelAvailable(preferredKernel))
        {
            return "No kernels are available. Please register at least one kernel.";
        }

        // Generate config for the selected kernel
        QString config = GenerateConfigForKernel(root, preferredKernel);
        if (config.isEmpty())
        {
            return "Failed to generate configuration for kernel: " + GetKernelName(preferredKernel);
        }

        // Save config to temporary file
        QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QString configExtension = availableKernels[preferredKernel].configFormat == "yaml" ? ".yaml" : ".json";
        currentConfigPath = tempDir + "/polycore_config_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + configExtension;
        
        if (!StringToFile(config, currentConfigPath))
        {
            return "Failed to write configuration file: " + currentConfigPath;
        }

        // Setup kernel process
        currentKernelType = preferredKernel;
        currentConnectionId = id;
        currentKernel = CreateKernelAdapter(preferredKernel);
        
        SetupKernelProcess(preferredKernel);
        
        // Start the kernel
        QStringList args = GetKernelStartArgs(preferredKernel, currentConfigPath);
        QString kernelPath = availableKernels[preferredKernel].executablePath;
        
        DEBUG("Starting kernel:", kernelPath, "with args:", args.join(" "));
        DEBUG("Config path:", currentConfigPath);
        
        kernelProcess->start(kernelPath, args);
        
        if (!kernelProcess->waitForStarted(5000))
        {
            QString error = "Failed to start kernel: " + kernelProcess->errorString();
            CleanupConnection();
            return error;
        }

        connectionRunning = true;
        DEBUG("Kernel started successfully for connection:", id.connectionId);
        emit OnConnectionStarted(id, preferredKernel);
        
        return std::nullopt;
    }

    void PolyCoreKernelManager::StopConnection()
    {
        if (!connectionRunning)
        {
            return;
        }

        DEBUG("Stopping connection:", currentConnectionId.connectionId);
        
        if (kernelProcess)
        {
            kernelProcess->terminate();
            if (!kernelProcess->waitForFinished(3000))
            {
                LOG("Kernel did not terminate gracefully, killing it");
                kernelProcess->kill();
                kernelProcess->waitForFinished(1000);
            }
        }

        CleanupConnection();
        emit OnConnectionStopped(currentConnectionId);
    }

    bool PolyCoreKernelManager::IsConnectionRunning() const
    {
        return connectionRunning;
    }

    KernelType PolyCoreKernelManager::GetCurrentKernelType() const
    {
        return currentKernelType;
    }

    std::optional<QString> PolyCoreKernelManager::ValidateConfig(const CONFIGROOT &root, KernelType kernelType)
    {
        if (!IsKernelAvailable(kernelType))
        {
            return "Kernel not available: " + GetKernelName(kernelType);
        }

        QString config = GenerateConfigForKernel(root, kernelType);
        if (config.isEmpty())
        {
            return "Failed to generate configuration";
        }

        // Save to temporary file for validation
        QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QString configExtension = availableKernels[kernelType].configFormat == "yaml" ? ".yaml" : ".json";
        QString tempPath = tempDir + "/polycore_validate_" + QString::number(QDateTime::currentMSecsSinceEpoch()) + configExtension;
        
        if (!StringToFile(config, tempPath))
        {
            return "Failed to write temporary config file";
        }

        auto adapter = CreateKernelAdapter(kernelType);
        bool isValid = adapter->ValidateConfig(tempPath);
        
        // Cleanup temp file
        QFile::remove(tempPath);
        
        if (!isValid)
        {
            return "Configuration validation failed";
        }

        return std::nullopt;
    }

    std::pair<bool, std::optional<QString>> PolyCoreKernelManager::ValidateKernel(const QString &kernelPath, KernelType type)
    {
        QFileInfo fileInfo(kernelPath);
        if (!fileInfo.exists())
        {
            return {false, "Kernel executable does not exist: " + kernelPath};
        }

        if (!fileInfo.isExecutable())
        {
            return {false, "File is not executable: " + kernelPath};
        }

        // Try to get version information
        QProcess process;
        QStringList args;
        
        switch (type)
        {
            case KernelType::V2RAY:
            case KernelType::XRAY:
                args << "version";
                break;
            case KernelType::MIHOMO:
                args << "-v";
                break;
            case KernelType::SING_BOX:
                args << "version";
                break;
        }

        process.start(kernelPath, args);
        if (!process.waitForFinished(3000))
        {
            return {false, "Kernel did not respond to version query"};
        }

        if (process.exitCode() != 0)
        {
            return {false, "Kernel version query failed with exit code: " + QString::number(process.exitCode())};
        }

        return {true, std::nullopt};
    }

    void PolyCoreKernelManager::AutoDetectKernels()
    {
        DEBUG("Auto-detecting kernels...");
        
        // Common installation paths for different platforms
        QStringList searchPaths;
        
#ifdef Q_OS_WIN
        searchPaths << QCoreApplication::applicationDirPath()
                   << QCoreApplication::applicationDirPath() + "/core"
                   << "C:/Program Files/v2ray"
                   << "C:/Program Files/xray"
                   << "C:/Program Files/mihomo" 
                   << "C:/Program Files/sing-box";
#elif defined(Q_OS_LINUX)
        searchPaths << "/usr/bin"
                   << "/usr/local/bin"
                   << "/opt/v2ray"
                   << "/opt/xray"
                   << "/opt/mihomo"
                   << "/opt/sing-box"
                   << QDir::homePath() + "/.local/bin"
                   << QDir::homePath() + "/.local/share/qv2ray/bin";
#elif defined(Q_OS_MACOS)
        searchPaths << "/usr/local/bin"
                   << "/opt/homebrew/bin"
                   << QCoreApplication::applicationDirPath()
                   << QDir::homePath() + "/.local/bin"
                   << QDir::homePath() + "/.local/share/qv2ray/bin";
#endif

        // Kernel executable names
        QMap<KernelType, QStringList> kernelNames = {
            {KernelType::V2RAY, {"v2ray", "v2ray.exe"}},
            {KernelType::XRAY, {"xray", "xray.exe"}},
            {KernelType::MIHOMO, {"mihomo", "mihomo.exe", "clash", "clash.exe", "mihomo-windows-amd64.exe", "mihomo-windows-amd64-v2.exe"}},
            {KernelType::SING_BOX, {"sing-box", "sing-box.exe"}}
        };

        for (auto kernelType : kernelNames.keys())
        {
            if (IsKernelAvailable(kernelType))
            {
                continue; // Already registered
            }

            for (const QString &searchPath : searchPaths)
            {
                QDir dir(searchPath);
                if (!dir.exists()) continue;

                for (const QString &kernelName : kernelNames[kernelType])
                {
                    QString fullPath = dir.absoluteFilePath(kernelName);
                    if (QFileInfo(fullPath).isExecutable())
                    {
                        DEBUG("Found kernel:", fullPath);
                        RegisterKernel(kernelType, fullPath);
                        goto next_kernel; // Found this kernel type, move to next
                    }
                }
            }
            next_kernel:;
        }

        DEBUG("Auto-detection completed. Found", availableKernels.size(), "kernels");
    }

    QString PolyCoreKernelManager::GetKernelVersion(KernelType type) const
    {
        if (availableKernels.contains(type))
        {
            return availableKernels[type].version;
        }
        return "Unknown";
    }

    // Private methods implementation

    std::unique_ptr<IPolyCoreKernel> PolyCoreKernelManager::CreateKernelAdapter(KernelType type)
    {
        switch (type)
        {
            case KernelType::V2RAY:
                return std::make_unique<V2RayKernelAdapter>(false);
            case KernelType::XRAY:
                return std::make_unique<V2RayKernelAdapter>(true);
            case KernelType::MIHOMO:
                return std::make_unique<MihomoKernelAdapter>();
            case KernelType::SING_BOX:
                return std::make_unique<SingBoxKernelAdapter>();
            default:
                return nullptr;
        }
    }

    QString PolyCoreKernelManager::GenerateConfigForKernel(const CONFIGROOT &root, KernelType kernelType)
    {
        auto adapter = CreateKernelAdapter(kernelType);
        if (!adapter)
        {
            return QString();
        }

        return adapter->GenerateConfig(root);
    }

    void PolyCoreKernelManager::SetupKernelProcess(KernelType type)
    {
        if (kernelProcess)
        {
            kernelProcess->deleteLater();
        }

        kernelProcess = new QProcess(this);
        
        connect(kernelProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &PolyCoreKernelManager::OnKernelFinished);
        connect(kernelProcess, &QProcess::errorOccurred,
                this, &PolyCoreKernelManager::OnKernelErrorOccurred);
        connect(kernelProcess, &QProcess::readyReadStandardOutput,
                this, &PolyCoreKernelManager::OnKernelReadyReadStandardOutput);
        connect(kernelProcess, &QProcess::readyReadStandardError,
                this, &PolyCoreKernelManager::OnKernelReadyReadStandardError);
    }

    QString PolyCoreKernelManager::GetDefaultKernelPath(KernelType type) const
    {
        Q_UNUSED(type)
        // This could be implemented to return default installation paths
        return QString();
    }

    QStringList PolyCoreKernelManager::GetKernelStartArgs(KernelType type, const QString &configPath) const
    {
        auto adapter = CreateKernelAdapter(type);
        if (!adapter)
        {
            return QStringList();
        }

        return adapter->GetStartupArguments(configPath);
    }

    QString PolyCoreKernelManager::GetKernelName(KernelType type) const
    {
        if (availableKernels.contains(type))
        {
            return availableKernels[type].name;
        }

        switch (type)
        {
            case KernelType::V2RAY: return "V2Ray";
            case KernelType::XRAY: return "Xray";
            case KernelType::MIHOMO: return "Mihomo";
            case KernelType::SING_BOX: return "sing-box";
            default: return "Unknown";
        }
    }

    void PolyCoreKernelManager::CleanupConnection()
    {
        connectionRunning = false;
        
        if (kernelProcess)
        {
            kernelProcess->deleteLater();
            kernelProcess = nullptr;
        }

        if (!currentConfigPath.isEmpty())
        {
            QFile::remove(currentConfigPath);
            currentConfigPath.clear();
        }

        currentKernel.reset();
        currentConnectionId = ConnectionGroupPair();
    }

    QString PolyCoreKernelManager::QueryKernelVersion(const QString &kernelPath, KernelType type) const
    {
        QProcess process;
        QStringList args;
        switch (type)
        {
            case KernelType::V2RAY:
            case KernelType::XRAY:
                args << "version";
                break;
            case KernelType::MIHOMO:
                args << "-v";
                break;
            case KernelType::SING_BOX:
                args << "version";
                break;
        }
        process.start(kernelPath, args);
        if (!process.waitForFinished(3000) || process.exitCode() != 0)
        {
            return QStringLiteral("Unknown");
        }
        const QString out = QString::fromUtf8(process.readAllStandardOutput() + process.readAllStandardError());
        return ParseVersionFromOutput(type, out);
    }

    KernelType PolyCoreKernelManager::ChooseAvailableKernel(KernelType preferred) const
    {
        // If explicitly available, use it
        if (IsKernelAvailable(preferred)) return preferred;
        // Try persisted preferred
        if (IsKernelAvailable(preferredKernel)) return preferredKernel;
        // Fallback priority: sing-box > mihomo > xray > v2ray
        const QList<KernelType> priority{
            KernelType::SING_BOX,
            KernelType::MIHOMO,
            KernelType::XRAY,
            KernelType::V2RAY
        };
        for (auto t : priority) if (IsKernelAvailable(t)) return t;
        return preferred; // likely unavailable; caller checks again
    }

    // Static helpers
    QString PolyCoreKernelManager::ParseVersionFromOutput(KernelType type, const QString &outputText)
    {
        const QString trimmed = outputText.trimmed();
        QRegularExpression re;
        if (type == KernelType::SING_BOX)
        {
            re.setPattern(R"((?i)sing-box\s+version\s+([\w\.-]+))");
        }
        else if (type == KernelType::MIHOMO)
        {
            re.setPattern(R"((?i)(mihomo|clash(?:\.meta)?)\s*[vV]?([\w\.-]+))");
        }
        else
        {
            re.setPattern(R"((?i)(xray|v2ray)\s+([\w\.-]+))");
        }
        auto m = re.match(trimmed);
        if (m.hasMatch())
        {
            return m.captured(m.lastCapturedIndex()).trimmed();
        }
        QString line = trimmed.split('\n').value(0).trimmed();
        return line.isEmpty() ? QStringLiteral("Unknown") : line;
    }

    PolyCoreKernelManager::KernelType PolyCoreKernelManager::ComputePreferredKernel(KernelType preferred, const QSet<KernelType> &available)
    {
        if (available.contains(preferred)) return preferred;
        if (available.contains(KernelType::SING_BOX)) return KernelType::SING_BOX;
        if (available.contains(KernelType::MIHOMO)) return KernelType::MIHOMO;
        if (available.contains(KernelType::XRAY)) return KernelType::XRAY;
        if (available.contains(KernelType::V2RAY)) return KernelType::V2RAY;
        return preferred;
    }

    // Slots implementation

    void PolyCoreKernelManager::OnKernelFinished(int exitCode, QProcess::ExitStatus exitStatus)
    {
        DEBUG("Kernel finished with exit code:", exitCode, "status:", exitStatus);
        
        if (connectionRunning)
        {
            QString errorMessage = "Kernel process terminated unexpectedly";
            if (exitStatus == QProcess::CrashExit)
            {
                errorMessage += " (crashed)";
            }
            errorMessage += ". Exit code: " + QString::number(exitCode);
            
            emit OnConnectionError(currentConnectionId, errorMessage);
            CleanupConnection();
        }
    }

    void PolyCoreKernelManager::OnKernelErrorOccurred(QProcess::ProcessError error)
    {
        QString errorString;
        switch (error)
        {
            case QProcess::FailedToStart:
                errorString = "Failed to start kernel process";
                break;
            case QProcess::Crashed:
                errorString = "Kernel process crashed";
                break;
            case QProcess::Timedout:
                errorString = "Kernel process timed out";
                break;
            case QProcess::ReadError:
                errorString = "Error reading from kernel process";
                break;
            case QProcess::WriteError:
                errorString = "Error writing to kernel process";
                break;
            default:
                errorString = "Unknown kernel process error";
                break;
        }

        LOG("Kernel error:", errorString);
        
        if (connectionRunning)
        {
            emit OnConnectionError(currentConnectionId, errorString);
            CleanupConnection();
        }
    }

    void PolyCoreKernelManager::OnKernelReadyReadStandardOutput()
    {
        if (!kernelProcess || !currentKernel)
        {
            return;
        }

        QByteArray data = kernelProcess->readAllStandardOutput();
        QString output = QString::fromUtf8(data);
        
        // Parse kernel-specific log format
        QString parsedLog = currentKernel->ParseKernelLog(output);
        
        emit OnKernelLogAvailable(currentConnectionId, parsedLog);

        // Check if kernel is ready
        if (currentKernel->IsKernelReady(output))
        {
            DEBUG("Kernel is ready and listening");
        }

        // Extract statistics if supported
        if (currentKernel->SupportsStats())
        {
            auto stats = currentKernel->ParseStatsOutput(output);
            if (!stats.isEmpty())
            {
                emit OnStatsDataAvailable(currentConnectionId, stats);
            }
        }
    }

    void PolyCoreKernelManager::OnKernelReadyReadStandardError()
    {
        if (!kernelProcess || !currentKernel)
        {
            return;
        }

        QByteArray data = kernelProcess->readAllStandardError();
        QString output = QString::fromUtf8(data);
        
        // Parse kernel-specific error log format
        QString parsedLog = currentKernel->ParseKernelLog(output);
        QString errorMsg = currentKernel->ExtractErrorMessage(output);
        
        if (!errorMsg.isEmpty())
        {
            LOG("Kernel error:", errorMsg);
            emit OnConnectionError(currentConnectionId, errorMsg);
        }
        
        emit OnKernelLogAvailable(currentConnectionId, parsedLog);
    }

} // namespace PolyCore::core::kernel