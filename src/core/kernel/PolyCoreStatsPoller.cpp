#include "PolyCoreStatsPoller.hpp"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

using namespace PolyCore::core::kernel;

PolyCoreStatsPoller::PolyCoreStatsPoller(QObject *parent) : QObject(parent)
{
    connect(&timer, &QTimer::timeout, this, &PolyCoreStatsPoller::OnTimer);
}

void PolyCoreStatsPoller::Start(const QUrl &baseUrl, int intervalMs)
{
    apiBase = baseUrl;
    lastUploadTotal = lastDownloadTotal = 0;
    lastTimestampMs = 0;
    baseIntervalMs = intervalMs;
    currentIntervalMs = intervalMs;
    errorCount = 0;
    timer.start(currentIntervalMs);
}

void PolyCoreStatsPoller::Stop()
{
    timer.stop();
}

void PolyCoreStatsPoller::OnTimer()
{
    if (!apiBase.isValid()) return;
    // Clash-compatible: /connections endpoint
    QUrl url = apiBase;
    url.setPath("/connections");
    auto *reply = network.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &PolyCoreStatsPoller::OnReply);
}

void PolyCoreStatsPoller::OnReply()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    if (reply->error() != QNetworkReply::NoError)
    {
        // Exponential backoff up to 10x on consecutive errors
        errorCount = std::min(errorCount + 1, 10);
        currentIntervalMs = std::min(baseIntervalMs * (1 << std::min(errorCount, 5)), baseIntervalMs * 10);
        timer.start(currentIntervalMs);
        reply->deleteLater();
        return;
    }
    const auto data = reply->readAll();
    reply->deleteLater();

    const auto nowMs = QDateTime::currentMSecsSinceEpoch();
    if (lastTimestampMs == 0) lastTimestampMs = nowMs;
    const double seconds = (nowMs - lastTimestampMs) / 1000.0;
    if (seconds <= 0.0) return;

    QJsonParseError perr;
    QJsonDocument doc = QJsonDocument::fromJson(data, &perr);
    if (perr.error != QJsonParseError::NoError)
    {
        errorCount = std::min(errorCount + 1, 10);
        currentIntervalMs = std::min(baseIntervalMs * (1 << std::min(errorCount, 5)), baseIntervalMs * 10);
        timer.start(currentIntervalMs);
        return;
    }
    if (!doc.isObject()) return;
    const auto obj = doc.object();
    // Aggregate all connection speeds
    quint64 upTotal = 0, downTotal = 0;
    auto getNum = [](const QJsonObject &o, const char *k) -> quint64 {
        auto v = o.value(k);
        if (v.isDouble()) return static_cast<quint64>(v.toDouble());
        return v.toVariant().toULongLong();
    };

    // Supports Clash connections schema: { connections: [ { upload, download }, ... ] }
    const auto conns = obj.value("connections").toArray();
    for (const auto &v : conns)
    {
        const auto c = v.toObject();
        upTotal += getNum(c, "upload");
        downTotal += getNum(c, "download");
    }

    // Compute delta speeds (bytes per second)
    quint64 upDelta = (lastUploadTotal == 0) ? 0 : (upTotal > lastUploadTotal ? upTotal - lastUploadTotal : 0);
    quint64 downDelta = (lastDownloadTotal == 0) ? 0 : (downTotal > lastDownloadTotal ? downTotal - lastDownloadTotal : 0);
    lastUploadTotal = upTotal;
    lastDownloadTotal = downTotal;
    lastTimestampMs = nowMs;
    // reset backoff on success
    errorCount = 0;
    if (currentIntervalMs != baseIntervalMs)
    {
        currentIntervalMs = baseIntervalMs;
        timer.start(currentIntervalMs);
    }

    QMap<StatisticsType, QvStatsSpeed> dataMap;
    dataMap[API_OUTBOUND_PROXY] = { static_cast<long>(upDelta / seconds), static_cast<long>(downDelta / seconds) };
    emit OnStats(dataMap);
}
