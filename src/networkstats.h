#ifndef NETWORKSTATS_H
#define NETWORKSTATS_H

#include <QObject>
#include <QTimer>
#include <QFileSystemWatcher>

struct NetworkStats {
    qint64 bytesReceived = 0;
    qint64 bytesSent = 0;
    double downloadSpeed = 0.0; // KB/s
    double uploadSpeed = 0.0;   // KB/s
    QString interface;
};

class NetworkStatsManager : public QObject {
    Q_OBJECT

public:
    explicit NetworkStatsManager(QObject *parent = nullptr);
    ~NetworkStatsManager();

    NetworkStats getCurrentStats() const;
    void startMonitoring();
    void stopMonitoring();
    void setInterface(const QString &interface);

signals:
    void statsUpdated(const NetworkStats &stats);

private slots:
    void updateStats();

private:
    QTimer *m_timer;
    NetworkStats m_currentStats;
    NetworkStats m_previousStats;
    QString m_interface;
    
    NetworkStats readInterfaceStats(const QString &interface) const;
    QString getDefaultInterface() const;
};

#endif // NETWORKSTATS_H
