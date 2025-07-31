#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QTimer>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
using namespace QtCharts;
#else
QT_CHARTS_USE_NAMESPACE
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_wifiManager(std::make_unique<WifiManager>(this))
{
    setupUi();
    
    // توصيل الإشارات
    connect(m_wifiManager.get(), &WifiManager::devicesUpdated, 
            this, &MainWindow::onDevicesUpdated);
    connect(m_wifiManager.get(), &WifiManager::errorOccurred,
            [this](const QString &error) { showMessage(error, true); });
    
    // بدء المراقبة
    m_wifiManager->startMonitoring();
    updateNetworkInfo();
    
    // تحديث معلومات الشبكة كل 10 ثواني
    QTimer *networkTimer = new QTimer(this);
    connect(networkTimer, &QTimer::timeout, this, &MainWindow::updateNetworkInfo);
    networkTimer->start(10000);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    setWindowTitle("مدير شبكة Wi-Fi المنزلية");
    resize(1200, 800);
    
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // القسم العلوي - معلومات الشبكة
    QGroupBox *networkInfoBox = new QGroupBox("معلومات الشبكة", this);
    QHBoxLayout *networkLayout = new QHBoxLayout(networkInfoBox);
    
    m_ssidLabel = new QLabel("SSID: جاري التحميل...", this);
    m_signalLabel = new QLabel("قوة الإشارة: --", this);
    m_devicesCountLabel = new QLabel("الأجهزة المتصلة: 0", this);
    m_bandwidthLabel = new QLabel("استخدام البيانات: 0 MB", this);
    
    networkLayout->addWidget(m_ssidLabel);
    networkLayout->addWidget(m_signalLabel);
    networkLayout->addWidget(m_devicesCountLabel);
    networkLayout->addWidget(m_bandwidthLabel);
    networkLayout->addStretch();
    
    // أزرار التحكم
    QGroupBox *controlBox = new QGroupBox("التحكم", this);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlBox);
    
    m_refreshBtn = new QPushButton("تحديث", this);
    m_blockBtn = new QPushButton("حظر الجهاز", this);
    m_unblockBtn = new QPushButton("إلغاء الحظر", this);
    QPushButton *changeSSIDBtn = new QPushButton("تغيير اسم الشبكة", this);
    QPushButton *changePasswordBtn = new QPushButton("تغيير كلمة المرور", this);
    QPushButton *restartBtn = new QPushButton("إعادة تشغيل الراوتر", this);
    
    controlLayout->addWidget(m_refreshBtn);
    controlLayout->addWidget(m_blockBtn);
    controlLayout->addWidget(m_unblockBtn);
    controlLayout->addWidget(changeSSIDBtn);
    controlLayout->addWidget(changePasswordBtn);
    controlLayout->addWidget(restartBtn);
    controlLayout->addStretch();
    
    // جدول الأجهزة
    QGroupBox *devicesBox = new QGroupBox("الأجهزة المتصلة", this);
    QVBoxLayout *devicesLayout = new QVBoxLayout(devicesBox);
    
    m_deviceTable = new QTableWidget(this);
    m_deviceTable->setColumnCount(6);
    m_deviceTable->setHorizontalHeaderLabels(QStringList() 
        << "عنوان IP" << "عنوان MAC" << "اسم الجهاز" 
        << "الشركة المصنعة" << "آخر ظهور" << "الحالة");
    m_deviceTable->horizontalHeader()->setStretchLastSection(true);
    m_deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_deviceTable->setSortingEnabled(true);
    
    devicesLayout->addWidget(m_deviceTable);
    
    // سجل الأحداث
    QGroupBox *logBox = new QGroupBox("سجل الأحداث", this);
    QVBoxLayout *logLayout = new QVBoxLayout(logBox);
    
    m_logWidget = new QTextEdit(this);
    m_logWidget->setReadOnly(true);
    m_logWidget->setMaximumHeight(150);
    
    logLayout->addWidget(m_logWidget);
    
    // إضافة جميع العناصر للتخطيط الرئيسي
    mainLayout->addWidget(networkInfoBox);
    mainLayout->addWidget(controlBox);
    mainLayout->addWidget(devicesBox, 1); // الجدول يأخذ المساحة المتبقية
    mainLayout->addWidget(logBox);
    
    // توصيل الأزرار
    connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);
    connect(m_blockBtn, &QPushButton::clicked, this, &MainWindow::onBlockDeviceClicked);
    connect(m_unblockBtn, &QPushButton::clicked, this, &MainWindow::onUnblockDeviceClicked);
    connect(changeSSIDBtn, &QPushButton::clicked, this, &MainWindow::onChangeSSIDClicked);
    connect(changePasswordBtn, &QPushButton::clicked, this, &MainWindow::onChangePasswordClicked);
    connect(restartBtn, &QPushButton::clicked, this, &MainWindow::onRestartRouterClicked);
    connect(m_deviceTable, &QTableWidget::cellClicked, this, &MainWindow::showDeviceDetails);
    
    createMenuBar();
}

void MainWindow::createMenuBar() {
    QMenuBar *menuBar = this->menuBar();
    
    // قائمة ملف
    QMenu *fileMenu = menuBar->addMenu("ملف");
    QAction *exitAction = fileMenu->addAction("خروج");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    // قائمة أدوات
    QMenu *toolsMenu = menuBar->addMenu("أدوات");
    QAction *scanAction = toolsMenu->addAction("فحص الشبكة");
    connect(scanAction, &QAction::triggered, this, &MainWindow::onRefreshClicked);
    
    // قائمة مساعدة
    QMenu *helpMenu = menuBar->addMenu("مساعدة");
    QAction *aboutAction = helpMenu->addAction("حول");
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "حول", "مدير شبكة Wi-Fi\nالإصدار 1.0\nتطبيق لإدارة الشبكة المنزلية");
    });
}

void MainWindow::onDevicesUpdated(const std::vector<Device> &devices) {
    updateDeviceTable(devices);
    m_devicesCountLabel->setText(QString("الأجهزة المتصلة: %1").arg(devices.size()));
    showMessage(QString("تم العثور على %1 جهاز متصل").arg(devices.size()));
}

void MainWindow::updateDeviceTable(const std::vector<Device> &devices) {
    m_deviceTable->setRowCount(devices.size());
    
    for (size_t i = 0; i < devices.size(); ++i) {
        const Device &device = devices[i];
        
        m_deviceTable->setItem(i, 0, new QTableWidgetItem(device.ipAddress));
        m_deviceTable->setItem(i, 1, new QTableWidgetItem(device.macAddress));
        m_deviceTable->setItem(i, 2, new QTableWidgetItem(device.hostname));
        m_deviceTable->setItem(i, 3, new QTableWidgetItem(device.manufacturer));
        m_deviceTable->setItem(i, 4, new QTableWidgetItem(device.lastSeen.toString("yyyy-MM-dd hh:mm:ss")));
        
        QTableWidgetItem *statusItem = new QTableWidgetItem(device.isActive ? "متصل" : "غير متصل");
        if (device.isActive) {
            statusItem->setBackground(QBrush(QColor(0, 255, 0, 50)));
        } else {
            statusItem->setBackground(QBrush(QColor(255, 0, 0, 50)));
        }
        m_deviceTable->setItem(i, 5, statusItem);
    }
    
    m_deviceTable->resizeColumnsToContents();
}

void MainWindow::onBlockDeviceClicked() {
    int currentRow = m_deviceTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "تحذير", "الرجاء اختيار جهاز أولاً");
        return;
    }
    
    QString macAddress = m_deviceTable->item(currentRow, 1)->text();
    QString deviceName = m_deviceTable->item(currentRow, 2)->text();
    
    int ret = QMessageBox::question(this, "تأكيد", 
        QString("هل أنت متأكد من حظر الجهاز %1؟").arg(deviceName.isEmpty() ? macAddress : deviceName));
    
    if (ret == QMessageBox::Yes) {
        if (m_wifiManager->blockDevice(macAddress)) {
            showMessage(QString("تم حظر الجهاز %1").arg(macAddress));
            onRefreshClicked();
        } else {
            showMessage("فشل حظر الجهاز", true);
        }
    }
}

void MainWindow::onUnblockDeviceClicked() {
    int currentRow = m_deviceTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "تحذير", "الرجاء اختيار جهاز أولاً");
        return;
    }
    
    QString macAddress = m_deviceTable->item(currentRow, 1)->text();
    
    if (m_wifiManager->unblockDevice(macAddress)) {
        showMessage(QString("تم إلغاء حظر الجهاز %1").arg(macAddress));
        onRefreshClicked();
    } else {
        showMessage("فشل إلغاء حظر الجهاز", true);
    }
}

void MainWindow::onChangeSSIDClicked() {
    bool ok;
    QString newSSID = QInputDialog::getText(this, "تغيير اسم الشبكة", 
        "أدخل اسم الشبكة الجديد:", QLineEdit::Normal, "", &ok);
    
    if (ok && !newSSID.isEmpty()) {
        if (m_wifiManager->changeSSID(newSSID)) {
            showMessage(QString("تم تغيير اسم الشبكة إلى %1").arg(newSSID));
            updateNetworkInfo();
        } else {
            showMessage("فشل تغيير اسم الشبكة", true);
        }
    }
}

void MainWindow::onChangePasswordClicked() {
    bool ok;
    QString newPassword = QInputDialog::getText(this, "تغيير كلمة المرور", 
        "أدخل كلمة المرور الجديدة:", QLineEdit::Password, "", &ok);
    
    if (ok && !newPassword.isEmpty()) {
        if (newPassword.length() < 8) {
            QMessageBox::warning(this, "تحذير", "يجب أن تكون كلمة المرور 8 أحرف على الأقل");
            return;
        }
        
        if (m_wifiManager->changePassword(newPassword)) {
            showMessage("تم تغيير كلمة المرور بنجاح");
        } else {
            showMessage("فشل تغيير كلمة المرور", true);
        }
    }
}

void MainWindow::onRestartRouterClicked() {
    int ret = QMessageBox::question(this, "تأكيد", 
        "هل أنت متأكد من إعادة تشغيل الراوتر؟\nسيتم قطع الاتصال مؤقتاً");
    
    if (ret == QMessageBox::Yes) {
        if (m_wifiManager->restartRouter()) {
            showMessage("تمت إعادة تشغيل الراوتر");
        } else {
            showMessage("فشلت إعادة تشغيل الراوتر", true);
        }
    }
}

void MainWindow::onRefreshClicked() {
    showMessage("جاري تحديث قائمة الأجهزة...");
    m_wifiManager->refreshDevices();
}

void MainWindow::updateNetworkInfo() {
    NetworkInfo info = m_wifiManager->getCurrentNetwork();
    m_ssidLabel->setText(QString("SSID: %1").arg(info.ssid));
    m_signalLabel->setText(QString("قوة الإشارة: %1 dBm").arg(info.signalStrength));
}

void MainWindow::showDeviceDetails(int row, int column) {
    Q_UNUSED(column);
    
    QString ip = m_deviceTable->item(row, 0)->text();
    QString mac = m_deviceTable->item(row, 1)->text();
    QString name = m_deviceTable->item(row, 2)->text();
    QString manufacturer = m_deviceTable->item(row, 3)->text();
    
    QString details = QString("تفاصيل الجهاز:\n"
                             "IP: %1\n"
                             "MAC: %2\n"
                             "الاسم: %3\n"
                             "الشركة: %4").arg(ip, mac, name, manufacturer);
    
    QMessageBox::information(this, "تفاصيل الجهاز", details);
}

void MainWindow::showMessage(const QString &message, bool isError) {
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString formattedMessage = QString("[%1] %2").arg(timestamp, message);
    
    if (isError) {
        m_logWidget->setTextColor(Qt::red);
    } else {
        m_logWidget->setTextColor(Qt::black);
    }
    
    m_logWidget->append(formattedMessage);
}
