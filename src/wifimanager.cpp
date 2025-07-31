#include "wifimanager.h"
#include <QDebug>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

WifiManager::WifiManager(QObject *parent) 
    : QObject(parent), 
      m_process(std::make_unique<QProcess>(this)),
      m_refreshTimer(std::make_unique<QTimer>(this))
{
    connect(m_refreshTimer.get(), &QTimer::timeout, this, &WifiManager::refreshDevices);
}

WifiManager::~WifiManager() {
    stopMonitoring();
}

QString WifiManager::executeCommand(const QString &command) {
    m_process->start("bash", QStringList() << "-c" << command);
    m_process->waitForFinished();
    return m_process->readAllStandardOutput();
}

NetworkInfo WifiManager::getCurrentNetwork() const {
    NetworkInfo info;
    
    // الحصول على معلومات الشبكة الحالية
    QString output = const_cast<WifiManager*>(this)->executeCommand("iwgetid -r");
    info.ssid = output.trimmed();
    
    output = const_cast<WifiManager*>(this)->executeCommand("iwgetid -a");
    QRegularExpression macRegex("([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})");
    QRegularExpressionMatch match = macRegex.match(output);
    if (match.hasMatch()) {
        info.bssid = match.captured(0);
    }
    
    // الحصول على قوة الإشارة
    output = const_cast<WifiManager*>(this)->executeCommand("iwconfig 2>/dev/null | grep 'Signal level'");
    QRegularExpression signalRegex("Signal level[=:](-?\\d+)");
    match = signalRegex.match(output);
    if (match.hasMatch()) {
        info.signalStrength = match.captured(1).toDouble();
    }
    
    return info;
}

std::vector<Device> WifiManager::getConnectedDevices() {
    std::vector<Device> devices;
    
    // استخدام arp-scan للحصول على الأجهزة المتصلة
    QString output = executeCommand("sudo arp-scan --local 2>/dev/null");
    
    QStringList lines = output.split('\n');
    QRegularExpression deviceRegex("(\\d+\\.\\d+\\.\\d+\\.\\d+)\\s+([0-9a-fA-F:]+)\\s+(.*)");
    
    for (const QString &line : lines) {
        QRegularExpressionMatch match = deviceRegex.match(line);
        if (match.hasMatch()) {
            Device device;
            device.ipAddress = match.captured(1);
            device.macAddress = match.captured(2).toUpper();
            device.manufacturer = match.captured(3);
            device.isActive = true;
            device.lastSeen = QDateTime::currentDateTime();
            
            // محاولة الحصول على hostname
            QString hostnameOutput = executeCommand(QString("nslookup %1 2>/dev/null | grep 'name ='").arg(device.ipAddress));
            if (!hostnameOutput.isEmpty()) {
                QRegularExpression hostnameRegex("name = (.+)\\.");
                QRegularExpressionMatch hostnameMatch = hostnameRegex.match(hostnameOutput);
                if (hostnameMatch.hasMatch()) {
                    device.hostname = hostnameMatch.captured(1);
                }
            }
            
            devices.push_back(device);
        }
    }
    
    m_devices = devices;
    emit devicesUpdated(devices);
    return devices;
}

bool WifiManager::blockDevice(const QString &macAddress) {
    // حظر جهاز باستخدام iptables
    QString command = QString("sudo iptables -A INPUT -m mac --mac-source %1 -j DROP").arg(macAddress);
    QString output = executeCommand(command);
    
    command = QString("sudo iptables -A FORWARD -m mac --mac-source %1 -j DROP").arg(macAddress);
    output += executeCommand(command);
    
    return output.isEmpty(); // لا توجد أخطاء إذا كان الناتج فارغ
}

bool WifiManager::unblockDevice(const QString &macAddress) {
    // إلغاء حظر جهاز
    QString command = QString("sudo iptables -D INPUT -m mac --mac-source %1 -j DROP").arg(macAddress);
    QString output = executeCommand(command);
    
    command = QString("sudo iptables -D FORWARD -m mac --mac-source %1 -j DROP").arg(macAddress);
    output += executeCommand(command);
    
    return output.isEmpty();
}

bool WifiManager::changeSSID(const QString &newSSID) {
    // تغيير اسم الشبكة (يتطلب تعديل ملف hostapd.conf)
    QString command = QString("sudo sed -i 's/^ssid=.*/ssid=%1/' /etc/hostapd/hostapd.conf").arg(newSSID);
    executeCommand(command);
    
    // إعادة تشغيل hostapd
    return restartRouter();
}

bool WifiManager::changePassword(const QString &newPassword) {
    // تغيير كلمة المرور
    QString command = QString("sudo sed -i 's/^wpa_passphrase=.*/wpa_passphrase=%1/' /etc/hostapd/hostapd.conf").arg(newPassword);
    executeCommand(command);
    
    return restartRouter();
}

bool WifiManager::restartRouter() {
    // إعادة تشغيل خدمات الشبكة
    executeCommand("sudo systemctl restart hostapd");
    executeCommand("sudo systemctl restart dnsmasq");
    return true;
}

void WifiManager::refreshDevices() {
    getConnectedDevices();
}

void WifiManager::startMonitoring() {
    m_refreshTimer->start(5000); // تحديث كل 5 ثواني
    refreshDevices();
}

void WifiManager::stopMonitoring() {
    m_refreshTimer->stop();
}
