#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "headers/processParser.h"
#include <map> 
#include "headers/StartupManager.h"
#include "headers/ServiceManager.h"
#include <QMessageBox>
#include "headers/qcustomplot.h" 
#include <QVector>



QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_terminateProcessButton_clicked();
    void on_disableStartupButton_clicked();
    void on_disconnectUserButton_clicked();
    void on_endTaskDetailsButton_clicked();
    void updateStartupButtonState();

    void on_tabWidget_currentChanged(int index);
    void on_servicesTable_customContextMenuRequested(const QPoint &pos);
    void startSelectedService();
    void stopSelectedService();
    void restartSelectedService();
    void populateStartupTable();
    void on_openServicesButton_clicked();
    void on_deleteHistoryButton_clicked();

    void on_performanceList_currentRowChanged(int row);


    void refreshStats();
private:
    Ui::MainWindow *ui;
    QTimer *processTimer;
    ProcessParser parser;

    StartupManager startupManager;


    struct DiskPageWidgets {
        QWidget* pageWidget = nullptr;
        QCustomPlot* graph = nullptr;
        QLabel* activeTimeLabel = nullptr;
        QLabel* readSpeedLabel = nullptr;
        QLabel* writeSpeedLabel = nullptr;
        QLabel* capacityLabel = nullptr;
        
        QString mountPath;
        FsInfo fsInfo; 

        std::string deviceName; // To store the device name, e.g., "sda1"
    DiskStats prevStats;    // To store the previous stats for *this* disk
    long prevIOTime;
    };
    QList<DiskPageWidgets> m_diskPages;
    CpuTimes prevCpuTimes;
    std::map<std::string, long> prevProcessJiffies;
    std::map<std::string, IoStats> prevProcessIo;

    AmdGpuInfo m_currentGpuStats;

    ServiceManager m_serviceManager;
    int m_servicesRefreshCounter;     
    void populateServicesTable();
    void populateAppHistoryTable();

    DiskStats m_prevDiskStats;
    NetStats m_prevNetStats;
    long m_prevDiskIOTime;

    QVector<double> m_timeData;
    QVector<double> m_cpuData;
    QVector<double> m_memData;
    QVector<double> m_diskData;
    QVector<double> m_netSendData;
    QVector<double> m_netRecvData;

    QVector<double> m_gpuUsageData;

    long m_lastReadBytes = 0;
    long m_lastWriteBytes = 0;
    QElapsedTimer m_diskTimer; 


    void setupPerformanceTab();
    void setupGraph(QCustomPlot* graph, const QColor& color, bool twoLines = false);
};
#endif // MAINWINDOW_H
