#include "networkstats.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QStringList>
#include <QDir>

NetworkStatsManager::NetworkStatsManager(QObject *parent)
    : QObject(parent), m_timer(new QTimer(this))
{
    m_interface = getDefaultInterface();
    connect(m_timer, &QTimer::timeout, this, &NetworkStatsManager::updateStats);
}

NetworkStatsManager::~NetworkStatsManager() {
    stopMonitoring();
}

QString NetworkStatsManager::getDefaultInterface() const {
    // البحث عن الواجهة النشطة
    QFile file("/proc/net/route");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString line;
        while (!(line = in.readLine()).isNull()) {
            QStringList fields = line.split('\t');
            if (fields.size() >= 2 && fields[1] == "00000000") {
                return fields[0];
            }
        }
    }
    
    // إذا لم نجد الواجهة الافتراضية، نبحث عن أول واجهة نشطة
    QDir netDir("/sys/class/net");
    QStringList interfaces = netDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    
    for (const QString &iface : interfaces) {
        if (iface != "lo") { // تجاهل loopback
            QFile operstate("/sys/class/net/" + iface + "/operstate");
            if (operstate.open(QIODevice::ReadOnly)) {
                QString state = operstate.readAll().trimmed();
                if (state == "up") {
                    return iface;
                }
            }
        }
    }
    
    return "wlan0"; // افتراضي
}

NetworkStats NetworkStatsManager::readInterfaceStats(const QString &interface) const {
    NetworkStats stats;
    stats.interface = interface;
    
    QFile file("/proc/net/dev");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return stats;
    }
    
    QTextStream in(&file);
    QString line;
    
    while (!(line = in.readLine()).isNull()) {
        if (line.contains(interface + ":")) {
            QStringList parts = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 10) {
                // الإحصائيات في /proc/net/dev:
                // bytes packets errs drop fifo frame compressed multicast
                stats.bytesReceived = parts[1].toLongLong();
                stats.bytesSent = parts[9].toLongLong();
            }
            break;
        }
    }
    
    return stats;
}

void NetworkStatsManager::updateStats() {
    NetworkStats newStats = readInterfaceStats(m_interface);
    
    if (m_previousStats.bytesReceived > 0) {
        // حساب السرعة (بالكيلوبايت/ثانية)
        qint64 downloadDiff = newStats.bytesReceived - m_previousStats.bytesReceived;
        qint64 uploadDiff = newStats.bytesSent - m_previousStats.bytesSent;
        
        newStats.downloadSpeed = downloadDiff / 1024.0; // KB/s
        newStats.uploadSpeed = uploadDiff / 1024.0;     // KB/s
    }
    
    m_previousStats = m_currentStats;
    m_currentStats = newStats;
    
    emit statsUpdated(m_currentStats);
}

NetworkStats NetworkStatsManager::getCurrentStats() const {
    return m_currentStats;
}

void NetworkStatsManager::startMonitoring() {
    // تحديث أولي
    m_currentStats = readInterfaceStats(m_interface);
    m_previousStats = m_currentStats;
    
    m_timer->start(1000); // تحديث كل ثانية
}

void NetworkStatsManager::stopMonitoring() {
    m_timer->stop();
}

void NetworkStatsManager::setInterface(const QString &interface) {
    m_interface = interface;
    if (m_timer->isActive()) {
        stopMonitoring();
        startMonitoring();
    }
}
