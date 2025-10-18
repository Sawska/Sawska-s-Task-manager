#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "headers/processParser.h"
#include <map> 


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
    
    void refreshStats();
private:
    Ui::MainWindow *ui;
    QTimer *processTimer;
    ProcessParser parser;

    CpuTimes prevCpuTimes;
    std::map<std::string, long> prevProcessJiffies;
    std::map<std::string, IoStats> prevProcessIo;
};
#endif // MAINWINDOW_H
