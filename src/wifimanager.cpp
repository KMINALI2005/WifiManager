#include "wifimanager.h"
#include <QDebug>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkInterface>
#include <QHostInfo>
#include <QStandardPaths>

WifiManager::WifiManager(QObject *parent) 
    : QObject(parent), 
      m_process(std::make_unique<QProcess>(this)),
      m_refreshTimer(std::make_unique<QTimer>(this))
{
    connect(m_refreshTimer.get(), &QTimer::timeout, this, &WifiManager::refreshDevices);
    m_activeInterface = getActiveWifiInterface();
}

WifiManager::~WifiManager() {
    stopMonitoring();
}

QString WifiManager::executeCommand(const QString &command) {
    m_process->start("bash", QStringList() << "-c" << command);
    if (!m_process->waitForFinished(10000)) { // timeout 10 seconds
        m_process->kill();
        return QString();
    }
    
    QString output = m_process->readAllStandardOutput();
    QString error = m_process->readAllStandardError();
    
    if (!error.isEmpty() && m_process->exitCode() != 0) {
        qDebug() << "Command error:" << command << "Error:" << error;
    }
    
    return output;
}

bool WifiManager::isCommandAvailable(const QString &command) const {
    QProcess process;
    process.start("which", QStringList() << command);
    process.waitForFinished(3000);
    return process.exitCode() == 0;
}

QString WifiManager::getActiveWifiInterface() const {
    // البحث عن واجهة Wi-Fi نشطة
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    
    for (const QNetworkInterface &interface : interfaces) {
        if (interface.flags().testFlag(QNetworkInterface::IsUp) &&
            interface.flags().testFlag(QNetworkInterface::IsRunning) &&
            !interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            
            QString name = interface.name();
            if (name.startsWith("wl") || name.startsWith("wlan")) {
                return name;
            }
        }
    }
    
    // إذا لم نجد واجهة Wi-Fi، نستخدم أول واجهة نشطة
    for (const QNetworkInterface &interface : interfaces) {
        if (interface.flags().testFlag(QNetworkInterface::IsUp) &&
            interface.flags().testFlag(QNetworkInterface::IsRunning) &&
            !interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            return interface.name();
        }
    }
    
    return "wlan0"; // افتراضي
}

NetworkInfo WifiManager::getCurrentNetwork() const {
    NetworkInfo info;
    info.interface = m_activeInterface;
    
    // محاولة الحصول على معلومات الشبكة بطرق متعددة
    if (isCommandAvailable("iwgetid")) {
        QString output = const_cast<WifiManager*>(this)->executeCommand(
            QString("iwgetid %1 -r 2>/dev/null").arg(m_activeInterface));
        info.ssid = output.trimmed();
        
        output = const_cast<WifiManager*>(this)->executeCommand(
            QString("iwgetid %1 -a 2>/dev/null").arg(m_activeInterface));
        QRegularExpression macRegex("([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})");
        QRegularExpressionMatch match = macRegex.match(output);
        if (match.hasMatch()) {
            info.bssid = match.captured(0);
        }
    } else if (isCommandAvailable("nmcli")) {
        QString output = const_cast<WifiManager*>(this)->executeCommand(
            "nmcli -t -f active,ssid dev wifi | grep '^yes' | cut -d: -f2");
        info.ssid = output.trimmed();
    }
    
    // الحصول على قوة الإشارة
    if (isCommandAvailable("iwconfig")) {
        QString output = const_cast<WifiManager*>(this)->executeCommand(
            QString("iwconfig %1 2>/dev/null | grep 'Signal level'").arg(m_activeInterface));
        QRegularExpression signalRegex("Signal level[=:](-?\\d+)");
        QRegularExpressionMatch match = signalRegex.match(output);
        if (match.hasMatch()) {
            info.signalStrength = match.captured(1).toDouble();
        }
    } else if (isCommandAvailable("iw")) {
        QString output = const_cast<WifiManager*>(this)->executeCommand(
            QString("iw %1 link 2>/dev/null | grep signal").arg(m_activeInterface));
        QRegularExpression signalRegex("signal: (-?\\d+)");
        QRegularExpressionMatch match = signalRegex.match(output);
        if (match.hasMatch()) {
            info.signalStrength = match.captured(1).toDouble();
        }
    }
    
    return info;
}

QString WifiManager::getManufacturer(const QString &macAddress) const {
    // قاعدة بيانات مبسطة للشركات المصنعة
    static QHash<QString, QString> vendors = {
        {"00:1B:63", "Apple"},
        {"A4:D1:8C", "Apple"},
        {"BC:F5:AC", "Apple"},
        {"F0:18:98", "Apple"},
        {"AC:37:43", "Samsung"},
        {"E8:50:8B", "Samsung"},
        {"78:4F:43", "Samsung"},
        {"08:00:27", "VirtualBox"},
        {"00:0C:29", "VMware"},
        {"00:50:56", "VMware"},
        {"52:54:00", "QEMU"}
    };
    
    QString prefix = macAddress.left(8).toUpper();
    return vendors.value(prefix, "غير معروف");
}

QString WifiManager::getHostname(const QString &ipAddress) const {
    // محاولة الحصول على hostname
    if (isCommandAvailable("nslookup")) {
        QString output = const_cast<WifiManager*>(this)->executeCommand(
            QString("nslookup %1 2>/dev/null | grep 'name =' | head -1").arg(ipAddress));
        QRegularExpression hostnameRegex("name = (.+)\\.");
        QRegularExpressionMatch match = hostnameRegex.match(output);
        if (match.hasMatch()) {
            return match.captured(1);
        }
    }
    
    if (isCommandAvailable("host")) {
        QString output = const_cast<WifiManager*>(this)->executeCommand(
            QString("host %1 2>/dev/null | head -1").arg(ipAddress));
        QRegularExpression hostnameRegex("pointer (.+)\\.");
        QRegularExpressionMatch match = hostnameRegex.match(output);
        if (match.hasMatch()) {
            return match.captured(1);
        }
    }
    
    return QString();
}

std::vector<Device> WifiManager::scanWithNmap() {
    std::vector<Device> devices;
    
    if (!isCommandAvailable("nmap")) {
        return devices;
    }
    
    // الحصول على نطاق الشبكة
    QString gateway = executeCommand("ip route | grep default | awk '{print $3}' | head -1").trimmed();
    if (gateway.isEmpty()) {
        return devices;
    }
    
    // تحويل عنوان البوابة إلى نطاق الشبكة
    QStringList parts = gateway.split('.');
    if (parts.size() != 4) {
        return devices;
    }
    
    QString network = QString("%1.%2.%3.0/24").arg(parts[0], parts[1], parts[2]);
    
    QString output = executeCommand(QString("nmap -sn %1 2>/dev/null").arg(network));
    
    QStringList lines = output.split('\n');
    Device currentDevice;
    
    for (const QString &line : lines) {
        if (line.startsWith("Nmap scan report for")) {
            if (!currentDevice.ipAddress.isEmpty()) {
                devices.push_back(currentDevice);
            }
            currentDevice = Device();
            
            QRegularExpression ipRegex("(\\d+\\.\\d+\\.\\d+\\.\\d+)");
            QRegularExpressionMatch match = ipRegex.match(line);
            if (match.hasMatch()) {
                currentDevice.ipAddress = match.captured(1);
                currentDevice.isActive = true;
                currentDevice.lastSeen = QDateTime::currentDateTime();
            }
            
            // استخراج hostname إذا كان موجوداً
            if (line.contains('(') && line.contains(')')) {
                QRegularExpression hostnameRegex("\\(([^)]+)\\)");
                QRegularExpressionMatch hostnameMatch = hostnameRegex.match(line);
                if (hostnameMatch.hasMatch()) {
                    QString hostname = hostnameMatch.captured(1);
                    if (!hostname.contains('.') || hostname.split('.').size() == 4) {
                        currentDevice.ipAddress = hostname;
                    } else {
                        currentDevice.hostname = hostname;
                    }
                }
            }
        } else if (line.contains("MAC Address:")) {
            QRegularExpression macRegex("([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})");
            QRegularExpressionMatch match = macRegex.match(line);
            if (match.hasMatch()) {
                currentDevice.macAddress = match.captured(0).toUpper();
                currentDevice.manufacturer = getManufacturer(currentDevice.macAddress);
            }
        }
    }
    
    if (!currentDevice.ipAddress.isEmpty()) {
        devices.push_back(currentDevice);
    }
    
    return devices;
}

std::vector<Device> WifiManager::scanWithArpScan() {
    std::vector<Device> devices;
    
    if (!isCommandAvailable("arp-scan")) {
        return devices;
    }
    
    QString output = executeCommand("arp-scan --local 2>/dev/null");
    
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
            device.hostname = getHostname(device.ipAddress);
            
            devices.push_back(device);
        }
    }
    
    return devices;
}

std::vector<Device> WifiManager::scanWithArpTable() {
    std::vector<Device> devices;
    
    QString output = executeCommand("arp -a 2>/dev/null");
    
    QStringList lines = output.split('\n');
    QRegularExpression arpRegex("([^\\s]+)\\s+\\((\\d+\\.\\d+\\.\\d+\\.\\d+)\\)\\s+at\\s+([0-9a-fA-F:]+)");
    
    for (const QString &line : lines) {
        QRegularExpressionMatch match = arpRegex.match(line);
        if (match.hasMatch()) {
            Device device;
            device.hostname = match.captured(1);
            device.ipAddress = match.captured(2);
            device.macAddress = match.captured(3).toUpper();
            device.manufacturer = getManufacturer(device.macAddress);
            device.isActive = true;
            device.lastSeen = QDateTime::currentDateTime();
            
            devices.push_back(device);
        }
    }
    
    return devices;
}

std::vector<Device> WifiManager::getConnectedDevices() {
    std::vector<Device> devices;
    
    // محاولة استخدام طرق متعددة للحصول على الأجهزة
    devices = scanWithNmap();
    
    if (devices.empty()) {
        devices = scanWithArpScan();
    }
    
    if (devices.empty()) {
        devices = scanWithArpTable();
    }
    
    // إضافة معلومات إضافية للأجهزة
    for (Device &device : devices) {
        if (device.hostname.isEmpty()) {
            device.hostname = getHostname(device.ipAddress);
        }
        
        if (device.manufacturer == "غير معروف") {
            device.manufacturer = getManufacturer(device.macAddress);
        }
    }
    
    m_devices = devices;
    emit devicesUpdated(devices);
    return devices;
}

bool WifiManager::blockDevice(const QString &macAddress) {
    if (!isCommandAvailable("iptables")) {
        emit errorOccurred("iptables غير متوفر على النظام");
        return false;
    }
    
    // حظر جهاز باستخدام iptables
    QString command = QString("iptables -A INPUT -m mac --mac-source %1 -j DROP").arg(macAddress);
    QString output = executeCommand(command);
    
    command = QString("iptables -A FORWARD -m mac --mac-source %1 -j DROP").arg(macAddress);
    output += executeCommand(command);
    
    return m_process->exitCode() == 0;
}

bool WifiManager::unblockDevice(const QString &macAddress) {
    if (!isCommandAvailable("iptables")) {
        emit errorOccurred("iptables غير متوفر على النظام");
        return false;
    }
    
    // إلغاء حظر جهاز
    QString command = QString("iptables -D INPUT -m mac --mac-source %1 -j DROP 2>/dev/null").arg(macAddress);
    executeCommand(command);
    
    command = QString("iptables -D FORWARD -m mac --mac-source %1 -j DROP 2>/dev/null").arg(macAddress);
    executeCommand(command);
    
    return true; // iptables -D لا يفشل حتى لو لم تكن القاعدة موجودة
}

bool WifiManager::changeSSID(const QString &newSSID) {
    // هذه الوظيفة تتطلب إعداد Access Point
    // سنحاول تغيير SSID في ملفات التكوين المختلفة
    
    QStringList configPaths = {
        "/etc/hostapd/hostapd.conf",
        "/etc/hostapd.conf",
        "/etc/wpa_supplicant/wpa_supplicant.conf"
    };
    
    bool found = false;
    for (const QString &path : configPaths) {
        if (QFile::exists(path)) {
            QString command = QString("sed -i 's/^ssid=.*/ssid=%1/' %2").arg(newSSID, path);
            executeCommand(command);
            found = true;
        }
    }
    
    if (!found) {
        emit errorOccurred("لم يتم العثور على ملفات تكوين Access Point");
        return false;
    }
    
    return restartRouter();
}

bool WifiManager::changePassword(const QString &newPassword) {
    QStringList configPaths = {
        "/etc/hostapd/hostapd.conf",
        "/etc/hostapd.conf"
    };
    
    bool found = false;
    for (const QString &path : configPaths) {
        if (QFile::exists(path)) {
            QString command = QString("sed -i 's/^wpa_passphrase=.*/wpa_passphrase=%1/' %2").arg(newPassword, path);
            executeCommand(command);
            found = true;
        }
    }
    
    if (!found) {
        emit errorOccurred("لم يتم العثور على ملفات تكوين Access Point");
        return false;
    }
    
    return restartRouter();
}

bool WifiManager::restartRouter() {
    // محاولة إعادة تشغيل خدمات الشبكة المختلفة
    QStringList services = {"hostapd", "dnsmasq", "networking", "NetworkManager"};
    
    bool success = false;
    for (const QString &service : services) {
        if (isCommandAvailable("systemctl")) {
            QString command = QString("systemctl restart %1 2>/dev/null").arg(service);
            executeCommand(command);
            if (m_process->exitCode() == 0) {
                success = true;
            }
        }
    }
    
    return success;
}

bool WifiManager::checkSystemRequirements() {
    QStringList requiredTools = {"ip", "arp"};
    QStringList optionalTools = {"nmap", "arp-scan", "iwconfig", "iw", "nmcli", "iptables"};
    
    bool hasRequired = true;
    for (const QString &tool : requiredTools) {
        if (!isCommandAvailable(tool)) {
            hasRequired = false;
            emit errorOccurred(QString("الأداة المطلوبة %1 غير متوفرة").arg(tool));
        }
    }
    
    int optionalCount = 0;
    for (const QString &tool : optionalTools) {
        if (isCommandAvailable(tool)) {
            optionalCount++;
        }
    }
    
    if (optionalCount == 0) {
        emit errorOccurred("لا توجد أدوات شبكة إضافية متوفرة. قد تكون الوظائف محدودة");
    }
    
    return hasRequired;
}

QStringList WifiManager::getMissingTools() {
    QStringList missing;
    QStringList tools = {"nmap", "arp-scan", "iwconfig", "iw", "nmcli", "iptables", "hostapd", "dnsmasq"};
    
    for (const QString &tool : tools) {
        if (!isCommandAvailable(tool)) {
            missing.append(tool);
        }
    }
    
    return missing;
}

qint64 WifiManager::getTotalBandwidthUsage() {
    // قراءة إحصائيات الواجهة
    QString command = QString("cat /proc/net/dev | grep %1").arg(m_activeInterface);
    QString output = executeCommand(command);
    
    QStringList parts = output.split(QRegExp("\\s+"), Qt::SkipEmptyParts);
    if (parts.size() >= 10) {
        qint64 received = parts[1].toLongLong();
        qint64 sent = parts[9].toLongLong();
        return received + sent;
    }
    
    return 0;
}

double WifiManager::getCurrentSpeed() {
    // هذه دالة مبسطة لحساب السرعة
    static qint64 lastBytes = 0;
    static QDateTime lastTime = QDateTime::currentDateTime();
    
    qint64 currentBytes = getTotalBandwidthUsage();
    QDateTime currentTime = QDateTime::currentDateTime();
    
    if (lastBytes > 0) {
        qint64 bytesDiff = currentBytes - lastBytes;
        qint64 timeDiff = lastTime.secsTo(currentTime);
        
        if (timeDiff > 0) {
            double speed = (bytesDiff / 1024.0) / timeDiff; // KB/s
            lastBytes = currentBytes;
            lastTime = currentTime;
            return speed;
        }
    }
    
    lastBytes = currentBytes;
    lastTime = currentTime;
    return 0.0;
}

void WifiManager::refreshDevices() {
    getConnectedDevices();
}

void WifiManager::startMonitoring() {
    // فحص المتطلبات عند البدء
    if (!checkSystemRequirements()) {
        QStringList missing = getMissingTools();
        if (!missing.isEmpty()) {
            emit errorOccurred(QString("أدوات مفقودة: %1").arg(missing.join(", ")));
        }
    }
    
    m_refreshTimer->start(5000); // تحديث كل 5 ثواني
    refreshDevices();
}

void WifiManager::stopMonitoring() {
    m_refreshTimer->stop();
}
