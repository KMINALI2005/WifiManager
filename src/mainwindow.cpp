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
#include <QProgressBar>
#include <QTimer>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>

QT_CHARTS_USE_NAMESPACE

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), 
      m_wifiManager(std::make_unique<WifiManager>(this)),
      m_statsManager(std::make_unique<NetworkStatsManager>(this))
{
    setupUi();
    
    // توصيل الإشارات
    connect(m_wifiManager.get(), &WifiManager::devicesUpdated, 
            this, &MainWindow::onDevicesUpdated);
    connect(m_wifiManager.get(), &WifiManager::errorOccurred,
            [this](const QString &error) { showMessage(error, true); });
    
    connect(m_statsManager.get(), &NetworkStatsManager::statsUpdated,
            this, &MainWindow::onStatsUpdated);
    
    // فحص متطلبات النظام
    checkSystemRequirements();
    
    // بدء المراقبة
    m_wifiManager->startMonitoring();
    m_statsManager->startMonitoring();
    updateNetworkInfo();
    
    // تحديث معلومات الشبكة كل 10 ثواني
    QTimer *networkTimer = new QTimer(this);
    connect(networkTimer, &QTimer::timeout, this, &MainWindow::updateNetworkInfo);
    networkTimer->start(10000);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    setWindowTitle("مدير شبكة Wi-Fi المنزلية");
    resize(1400, 900);
    
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // القسم العلوي - معلومات الشبكة
    QGroupBox *networkInfoBox = new QGroupBox("معلومات الشبكة", this);
    QGridLayout *networkLayout = new QGridLayout(networkInfoBox);
    
    m_ssidLabel = new QLabel("SSID: جاري التحميل...", this);
    m_signalLabel = new QLabel("قوة الإشارة: --", this);
    m_devicesCountLabel = new QLabel("الأجهزة المتصلة: 0", this);
    m_bandwidthLabel = new QLabel("إجمالي البيانات: 0 MB", this);
    m_downloadSpeedLabel = new QLabel("سرعة التحميل: 0 KB/s", this);
    m_uploadSpeedLabel = new QLabel("سرعة الرفع: 0 KB/s", this);
    
    m_signalProgressBar = new QProgressBar(this);
    m_signalProgressBar->setRange(0, 100);
    m_signalProgressBar->setValue(0);
    m_signalProgressBar->setTextVisible(true);
    
    networkLayout->addWidget(m_ssidLabel, 0, 0);
    networkLayout->addWidget(m_signalLabel, 0, 1);
    networkLayout->addWidget(m_signalProgressBar, 0, 2);
    networkLayout->addWidget(m_devicesCountLabel, 1, 0);
    networkLayout->addWidget(m_bandwidthLabel, 1, 1);
    networkLayout->addWidget(m_downloadSpeedLabel, 2, 0);
    networkLayout->addWidget(m_uploadSpeedLabel, 2, 1);
    
    // أزرار التحكم
    QGroupBox *controlBox = new QGroupBox("التحكم", this);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlBox);
    
    m_refreshBtn = new QPushButton("تحديث", this);
    m_blockBtn = new QPushButton("حظر الجهاز", this);
    m_unblockBtn = new QPushButton("إلغاء الحظر", this);
    QPushButton *changeSSIDBtn = new QPushButton("تغيير اسم الشبكة", this);
    QPushButton *changePasswordBtn = new QPushButton("تغيير كلمة المرور", this);
    QPushButton *restartBtn = new QPushButton("إعادة تشغيل الراوتر", this);
    QPushButton *requirementsBtn = new QPushButton("فحص المتطلبات", this);
    
    controlLayout->addWidget(m_refreshBtn);
    controlLayout->addWidget(m_blockBtn);
    controlLayout->addWidget(m_unblockBtn);
    controlLayout->addWidget(changeSSIDBtn);
    controlLayout->addWidget(changePasswordBtn);
    controlLayout->addWidget(restartBtn);
    controlLayout->addWidget(requirementsBtn);
    controlLayout->addStretch();
    
    // إنشاء Splitter للقسم السفلي
    QSplitter *bottomSplitter = new QSplitter(Qt::Horizontal, this);
    
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
    m_deviceTable->setAlternatingRowColors(true);
    
    devicesLayout->addWidget(m_deviceTable);
    
    // رسم بياني للإحصائيات
    QGroupBox *chartBox = new QGroupBox("إحصائيات الاستخدام", this);
    QVBoxLayout *chartLayout = new QVBoxLayout(chartBox);
    
    createChart();
    chartLayout->addWidget(m_chartView);
    
    bottomSplitter->addWidget(devicesBox);
    bottomSplitter->addWidget(chartBox);
    bottomSplitter->setSizes({800, 400});
    
    // سجل الأحداث
    QGroupBox *logBox = new QGroupBox("سجل الأحداث", this);
    QVBoxLayout *logLayout = new QVBoxLayout(logBox);
    
    m_logWidget = new QTextEdit(this);
    m_logWidget->setReadOnly(true);
    m_logWidget->setMaximumHeight(120);
    
    logLayout->addWidget(m_logWidget);
    
    // إضافة جميع العناصر للتخطيط الرئيسي
    mainLayout->addWidget(networkInfoBox);
    mainLayout->addWidget(controlBox);
    mainLayout->addWidget(bottomSplitter, 1);
    mainLayout->addWidget(logBox);
    
    // توصيل الأزرار
    connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);
    connect(m_blockBtn, &QPushButton::clicked, this, &MainWindow::onBlockDeviceClicked);
    connect(m_unblockBtn, &QPushButton::clicked, this, &MainWindow::onUnblockDeviceClicked);
    connect(changeSSIDBtn, &QPushButton::clicked, this, &MainWindow::onChangeSSIDClicked);
    connect(changePasswordBtn, &QPushButton::clicked, this, &MainWindow::onChangePasswordClicked);
    connect(restartBtn, &QPushButton::clicked, this, &MainWindow::onRestartRouterClicked);
    connect(requirementsBtn, &QPushButton::clicked, this, &MainWindow::checkSystemRequirements);
    connect(m_deviceTable, &QTableWidget::cellClicked, this, &MainWindow::showDeviceDetails);
    
    createMenuBar();
}

void MainWindow::createChart() {
    m_chart = new QChart();
    m_pieSeries = new QPieSeries();
    
    // إضافة البيانات الأولية
    m_pieSeries->append("تحميل", 0);
    m_pieSeries->append("رفع", 0);
    
    // تخصيص الألوان
    m_pieSeries->slices().at(0)->setColor(QColor("#3498db"));
    m_pieSeries->slices().at(1)->setColor(QColor("#e74c3c"));
    
    m_chart->addSeries(m_pieSeries);
    m_chart->setTitle("استخدام البيانات");
    m_chart->legend()->setAlignment(Qt::AlignBottom);
    
    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(250);
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
    QAction *requirementsAction = toolsMenu->addAction("فحص المتطلبات");
    
    connect(scanAction, &QAction::triggered, this, &MainWindow::onRefreshClicked);
    connect(requirementsAction, &QAction::triggered, this, &MainWindow::checkSystemRequirements);
    
    // قائمة مساعدة
    QMenu *helpMenu = menuBar->addMenu("مساعدة");
    QAction *aboutAction = helpMenu->addAction("حول");
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "حول", 
            "مدير شبكة Wi-Fi\n"
            "الإصدار 1.0\n"
            "تطبيق لإدارة الشبكة المنزلية\n\n"
            "يتطلب صلاحيات root للوظائف المتقدمة");
    });
}

void MainWindow::checkSystemRequirements() {
    QStringList missing = m_wifiManager->getMissingTools();
    
    QString message;
    if (missing.isEmpty()) {
        message = "جميع الأدوات المطلوبة متوفرة!";
        QMessageBox::information(this, "فحص المتطلبات", message);
    } else {
        message = QString("الأدوات المفقودة:\n%1\n\n"
                         "لتثبيت الأدوات المفقودة على Kali Linux:\n"
                         "sudo apt install %2")
                         .arg(missing.join("\n"), missing.join(" "));
        QMessageBox::warning(this, "أدوات مفقودة", message);
    }
    
    showMessage(QString("فحص المتطلبات: %1 أداة مفقودة").arg(missing.size()));
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
        m_deviceTable->setItem(i, 2, new QTableWidgetItem(device.hostname.isEmpty() ? "غير معروف" : device.hostname));
        m_deviceTable->setItem(i, 3, new QTableWidgetItem(device.manufacturer));
        m_deviceTable->setItem(i, 4, new QTableWidgetItem(device.lastSeen.toString("yyyy-MM-dd hh:mm:ss")));
        
        QTableWidgetItem *statusItem = new QTableWidgetItem(device.isActive ? "متصل" : "غير متصل");
        if (device.isActive) {
            statusItem->setBackground(QBrush(QColor(46, 204, 113, 100)));
            statusItem->setForeground(QBrush(QColor(39, 174, 96)));
        } else {
            statusItem->setBackground(QBrush(QColor(231, 76, 60, 100)));
            statusItem->setForeground(QBrush(QColor(192, 57, 43)));
        }
        m_deviceTable->setItem(i, 5, statusItem);
    }
    
    m_deviceTable->resizeColumnsToContents();
}

void MainWindow::onStatsUpdated(const NetworkStats &stats) {
    m_downloadSpeedLabel->setText(QString("سرعة التحميل: %.2f KB/s").arg(stats.downloadSpeed));
    m_uploadSpeedLabel->setText(QString("سرعة الرفع: %.2f KB/s").arg(stats.uploadSpeed));
    
    double totalMB = (stats.bytesReceived + stats.bytesSent) / (1024.0 * 1024.0);
    m_bandwidthLabel->setText(QString("إجمالي البيانات: %.2f MB").arg(totalMB));
    
    updateChart(stats);
}

void MainWindow::updateChart(const NetworkStats &stats) {
    m_pieSeries->clear();
    
    double downloadMB = stats.bytesReceived / (1024.0 * 1024.0);
    double uploadMB = stats.bytesSent / (1024.0 * 1024.0);
    
    if (downloadMB > 0 || uploadMB > 0) {
        QPieSlice *downloadSlice = m_pieSeries->append(QString("تحميل (%.2f MB)").arg(downloadMB), downloadMB);
        QPieSlice *uploadSlice = m_pieSeries->append(QString("رفع (%.2f MB)").arg(uploadMB), uploadMB);
        
        downloadSlice->setColor(QColor("#3498db"));
        uploadSlice->setColor(QColor("#e74c3c"));
        
        // إظهار النسب المئوية
        downloadSlice->setLabelVisible(true);
        uploadSlice->setLabelVisible(true);
    }
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
        QString("هل أنت متأكد من حظر الجهاز %1؟\n\n"
                "ملاحظة: يتطلب صلاحيات root")
                .arg(deviceName.isEmpty() || deviceName == "غير معروف" ? macAddress : deviceName));
    
    if (ret == QMessageBox::Yes) {
        if (m_wifiManager->blockDevice(macAddress)) {
            showMessage(QString("تم حظر الجهاز %1").arg(macAddress));
            onRefreshClicked();
        } else {
            showMessage("فشل حظر الجهاز - تأكد من صلاحيات root", true);
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
        "أدخل اسم الشبكة الجديد:\n"
        "(ملاحظة: يتطلب إعداد Access Point)", 
        QLineEdit::Normal, "", &ok);
    
    if (ok && !newSSID.isEmpty()) {
        if (m_wifiManager->changeSSID(newSSID)) {
            showMessage(QString("تم تغيير اسم الشبكة إلى %1").arg(newSSID));
            updateNetworkInfo();
        } else {
            showMessage("فشل تغيير اسم الشبكة - تأكد من إعداد Access Point", true);
        }
    }
}

void MainWindow::onChangePasswordClicked() {
    bool ok;
    QString newPassword = QInputDialog::getText(this, "تغيير كلمة المرور", 
        "أدخل كلمة المرور الجديدة:\n"
        "(ملاحظة: يتطلب إعداد Access Point)", 
        QLineEdit::Password, "", &ok);
    
    if (ok && !newPassword.isEmpty()) {
        if (newPassword.length() < 8) {
            QMessageBox::warning(this, "تحذير", "يجب أن تكون كلمة المرور 8 أحرف على الأقل");
            return;
        }
        
        if (m_wifiManager->changePassword(newPassword)) {
            showMessage("تم تغيير كلمة المرور بنجاح");
        } else {
            showMessage("فشل تغيير كلمة المرور - تأكد من إعداد Access Point", true);
        }
    }
}

void MainWindow::onRestartRouterClicked() {
    int ret = QMessageBox::question(this, "تأكيد", 
        "هل أنت متأكد من إعادة تشغيل خدمات الشبكة؟\n"
        "قد يتم قطع الاتصال مؤقتاً\n\n"
        "ملاحظة: يتطلب صلاحيات root");
    
    if (ret == QMessageBox::Yes) {
        if (m_wifiManager->restartRouter()) {
            showMessage("تمت محاولة إعادة تشغيل خدمات الشبكة");
        } else {
            showMessage("فشلت إعادة تشغيل خدمات الشبكة", true);
        }
    }
}

void MainWindow::onRefreshClicked() {
    showMessage("جاري تحديث قائمة الأجهزة...");
    m_refreshBtn->setEnabled(false);
    m_refreshBtn->setText("جاري التحديث...");
    
    m_wifiManager->refreshDevices();
    
    // إعادة تفعيل الزر بعد ثانيتين
    QTimer::singleShot(2000, [this]() {
        m_refreshBtn->setEnabled(true);
        m_refreshBtn->setText("تحديث");
    });
}

void MainWindow::updateNetworkInfo() {
    NetworkInfo info = m_wifiManager->getCurrentNetwork();
    
    m_ssidLabel->setText(QString("SSID: %1").arg(info.ssid.isEmpty() ? "غير متصل" : info.ssid));
    
    if (info.signalStrength != 0) {
        m_signalLabel->setText(QString("قوة الإشارة: %1 dBm").arg(info.signalStrength));
        
        // تحويل قوة الإشارة إلى نسبة مئوية (تقريبي)
        // -30 dBm = 100%, -90 dBm = 0%
        int percentage = qBound(0, static_cast<int>((info.signalStrength + 90) * 100 / 60), 100);
        m_signalProgressBar->setValue(percentage);
        
        if (percentage > 70) {
            m_signalProgressBar->setStyleSheet("QProgressBar::chunk { background-color: #27ae60; }");
        } else if (percentage > 40) {
            m_signalProgressBar->setStyleSheet("QProgressBar::chunk { background-color: #f39c12; }");
        } else {
            m_signalProgressBar->setStyleSheet("QProgressBar::chunk { background-color: #e74c3c; }");
        }
    } else {
        m_signalLabel->setText("قوة الإشارة: غير متاح");
        m_signalProgressBar->setValue(0);
    }
}

void MainWindow::showDeviceDetails(int row, int column) {
    Q_UNUSED(column);
    
    if (row < 0 || row >= m_deviceTable->rowCount()) {
        return;
    }
    
    QString ip = m_deviceTable->item(row, 0)->text();
    QString mac = m_deviceTable->item(row, 1)->text();
    QString name = m_deviceTable->item(row, 2)->text();
    QString manufacturer = m_deviceTable->item(row, 3)->text();
    QString lastSeen = m_deviceTable->item(row, 4)->text();
    QString status = m_deviceTable->item(row, 5)->text();
    
    QString details = QString("تفاصيل الجهاز:\n\n"
                             "عنوان IP: %1\n"
                             "عنوان MAC: %2\n"
                             "اسم الجهاز: %3\n"
                             "الشركة المصنعة: %4\n"
                             "آخر ظهور: %5\n"
                             "الحالة: %6")
                             .arg(ip, mac, name, manufacturer, lastSeen, status);
    
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
    
    // التمرير إلى الأسفل تلقائياً
    QTextCursor cursor = m_logWidget->textCursor();
    cursor.movePosition(QTextCursor::End);
    m_logWidget->setTextCursor(cursor);
}
