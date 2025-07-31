#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include "wifimanager.h"

QT_BEGIN_NAMESPACE
class QListWidget;
class QLabel;
class QPushButton;
class QLineEdit;
class QTableWidget;
class QChartView;
class QTextEdit;
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onDevicesUpdated(const std::vector<Device> &devices);
    void onBlockDeviceClicked();
    void onUnblockDeviceClicked();
    void onChangeSSIDClicked();
    void onChangePasswordClicked();
    void onRestartRouterClicked();
    void onRefreshClicked();
    void updateNetworkInfo();
    void showDeviceDetails(int row, int column);

private:
    void setupUi();
    void createMenuBar();
    void updateDeviceTable(const std::vector<Device> &devices);
    void showMessage(const QString &message, bool isError = false);
    
    std::unique_ptr<WifiManager> m_wifiManager;
    
    // UI Elements
    QTableWidget *m_deviceTable;
    QLabel *m_ssidLabel;
    QLabel *m_signalLabel;
    QLabel *m_devicesCountLabel;
    QLabel *m_bandwidthLabel;
    QPushButton *m_blockBtn;
    QPushButton *m_unblockBtn;
    QPushButton *m_refreshBtn;
    QTextEdit *m_logWidget;
    QChartView *m_chartView;
};

#endif // MAINWINDOW_H
