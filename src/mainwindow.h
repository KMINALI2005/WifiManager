#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include "wifimanager.h"
#include "networkstats.h"

QT_BEGIN_NAMESPACE
class QListWidget;
class QLabel;
class QPushButton;
class QLineEdit;
class QTableWidget;
class QTextEdit;
class QProgressBar;
class QChartView;
class QChart;
class QPieSeries;
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
    void onStatsUpdated(const NetworkStats &stats);
    void checkSystemRequirements();

private:
    void setupUi();
    void createMenuBar();
    void createChart();
    void updateDeviceTable(const std::vector<Device> &devices);
    void updateChart(const NetworkStats &stats);
    void showMessage(const QString &message, bool isError = false);
    
    std::unique_ptr<WifiManager> m_wifiManager;
    std::unique_ptr<NetworkStatsManager> m_statsManager;
    
    // UI Elements
    QTableWidget *m_deviceTable;
    QLabel *m_ssidLabel;
    QLabel *m_signalLabel;
    QLabel *m_devicesCountLabel;
    QLabel *m_bandwidthLabel;
    QLabel *m_downloadSpeedLabel;
    QLabel *m_uploadSpeedLabel;
    QPushButton *m_blockBtn;
    QPushButton *m_unblockBtn;
    QPushButton *m_refreshBtn;
    QTextEdit *m_logWidget;
    QChartView *m_chartView;
    QChart *m_chart;
    QPieSeries *m_pieSeries;
    QProgressBar *m_signalProgressBar;
};

#endif // MAINWINDOW_H
