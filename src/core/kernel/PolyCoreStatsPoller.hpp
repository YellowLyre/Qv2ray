#pragma once

#include "base/Qv2rayBase.hpp"

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

namespace PolyCore::core::kernel
{
    // Polls Clash-compatible API (/connections) and emits speed deltas
    class PolyCoreStatsPoller : public QObject
    {
        Q_OBJECT
    public:
        explicit PolyCoreStatsPoller(QObject *parent = nullptr);
        void Start(const QUrl &baseUrl, int intervalMs = 1000);
        void Stop();

    signals:
        void OnStats(const QMap<StatisticsType, QvStatsSpeed> &data);

    private slots:
        void OnTimer();
        void OnReply();

    private:
        QNetworkAccessManager network;
        QTimer timer;
        QUrl apiBase;
        quint64 lastUploadTotal = 0;
        quint64 lastDownloadTotal = 0;
        qint64 lastTimestampMs = 0;
        int baseIntervalMs = 1000;
        int currentIntervalMs = 1000;
        int errorCount = 0;
    };
}
