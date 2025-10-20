#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "headers/processParser.h"
#include <map> 
#include "headers/StartupManager.h"
#include "headers/ServiceManager.h"
#include <QMessageBox>



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

    void refreshStats();
private:
    Ui::MainWindow *ui;
    QTimer *processTimer;
    ProcessParser parser;

    StartupManager startupManager;

    

    CpuTimes prevCpuTimes;
    std::map<std::string, long> prevProcessJiffies;
    std::map<std::string, IoStats> prevProcessIo;

    ServiceManager m_serviceManager;
    int m_servicesRefreshCounter;     
    void populateServicesTable();
    void populateAppHistoryTable();

};
#endif // MAINWINDOW_H
