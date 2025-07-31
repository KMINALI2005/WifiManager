#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QNetworkInterface>
#include <QDateTime>
#include <memory>
#include <vector>

struct Device {
    QString macAddress;
    QString ipAddress;
    QString hostname;
    QString manufacturer;
    double signalStrength = 0.0;
    qint64 bytesReceived = 0;
    qint64 bytesSent = 0;
    bool isActive = false;
    QDateTime lastSeen;
};

struct NetworkInfo {
    QString ssid;
    QString bssid;
    QString password;
    QString encryption;
    int channel = 0;
    int frequency = 0;
    double signalStrength = 0.0;
    QString interface;
};

class WifiManager : public QObject {
    Q_OBJECT

public:
    explicit WifiManager(QObject *parent = nullptr);
    ~WifiManager();

    // معلومات الشبكة
    NetworkInfo getCurrentNetwork() const;
    std::vector<NetworkInfo> getAvailableNetworks();
    
    // إدارة الأجهزة
    std::vector<Device> getConnectedDevices();
    bool blockDevice(const QString &macAddress);
    bool unblockDevice(const QString &macAddress);
    
    // إعدادات الشبكة
    bool changeSSID(const QString &newSSID);
    bool changePassword(const QString &newPassword);
    bool changeChannel(int channel);
    bool restartRouter();
    
    // إحصائيات
    qint64 getTotalBandwidthUsage();
    double getCurrentSpeed();
    
    // فحص المتطلبات
    bool checkSystemRequirements();
    QStringList getMissingTools();

public slots:
    void refreshDevices();
    void startMonitoring();
    void stopMonitoring();

signals:
    void devicesUpdated(const std::vector<Device> &devices);
    void networkStatusChanged(const NetworkInfo &info);
    void errorOccurred(const QString &error);
    void bandwidthUpdated(qint64 download, qint64 upload);

private:
    std::unique_ptr<QProcess> m_process;
    std::unique_ptr<QTimer> m_refreshTimer;
    NetworkInfo m_currentNetwork;
    std::vector<Device> m_devices;
    QString m_activeInterface;
    
    void parseConnectedDevices(const QString &output);
    void updateDeviceInfo(Device &device);
    QString executeCommand(const QString &command);
    bool requiresRoot() const;
    bool isCommandAvailable(const QString &command) const;
    QString getActiveWifiInterface() const;
    QString getManufacturer(const QString &macAddress) const;
    QString getHostname(const QString &ipAddress) const;
    std::vector<Device> scanWithNmap();
    std::vector<Device> scanWithArpScan();
    std::vector<Device> scanWithArpTable();
};

#endif // WIFIMANAGER_H
