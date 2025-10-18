#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>
#include <QMap>
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QProcess>
#include <QString>
 

QString formatBytesRate(double bytesPerSec) {
    if (bytesPerSec < 1024) {
        return QString::number(bytesPerSec, 'f', 0) + " B/s";
    } else if (bytesPerSec < 1024 * 1024) {
        return QString::number(bytesPerSec / 1024.0, 'f', 1) + " KB/s";
    } else if (bytesPerSec < 1024 * 1024 * 1024) {
        return QString::number(bytesPerSec / (1024.0 * 1024.0), 'f', 1) + " MB/s";
    } else {
        return QString::number(bytesPerSec / (1024.0 * 1024.0 * 1024.0), 'f', 1) + " GB/s";
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->processTable->setColumnWidth(0, 250);
    ui->processTable->setColumnWidth(1, 80); 
    ui->processTable->setColumnWidth(2, 80);  
    ui->processTable->setColumnWidth(3, 120); 
    ui->processTable->setColumnWidth(4, 150); 

    ui->usersTreeWidget->setColumnWidth(0, 250); 
    ui->usersTreeWidget->setColumnWidth(1, 80);  
    ui->usersTreeWidget->setColumnWidth(2, 120); 

    processTimer = new QTimer(this);
    connect(processTimer, &QTimer::timeout, this, &MainWindow::refreshStats);
    
    prevCpuTimes = parser.getCpuTimes();

    refreshStats();
    processTimer->start(1000);
}

MainWindow::~MainWindow()
{
    processTimer->stop();
    delete ui;
}


void MainWindow::on_terminateProcessButton_clicked()
{
    int currentRow = ui->processTable->currentRow();
    if (currentRow < 0) {
        qDebug("No process selected.");
        return; 
    }

    QTableWidgetItem *pidItem = ui->processTable->item(currentRow, 1);
    if (!pidItem) {
        qDebug("Could not find PID item for selected row.");
        return; 
    }

    QString pidString = pidItem->text();
    std::string pid_std_string = pidString.toStdString();

    qDebug() << "Attempting to terminate PID:" << pidString;

    bool success = parser.terminateProcess(pid_std_string);

    if (success) {
        qDebug() << "Process" << pidString << "terminated.";
    } else {
        qDebug() << "Failed to terminate process" << pidString;
    }

    QTimer::singleShot(500, this, &MainWindow::refreshStats);
}

void MainWindow::on_disconnectUserButton_clicked() 
{
    QTreeWidgetItem *item = ui->usersTreeWidget->currentItem();
    if (!item) {
        qDebug("No user selected.");
        return;
    }

    QString username = item->text(0);

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Disconnect User", 
                                  QString("Are you sure you want to disconnect '%1'?\n"
                                          "This will terminate all processes owned by this user.")
                                  .arg(username),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::No) {
        return; 
    }

    qDebug() << "Attempting to disconnect user:" << username;

    QProcess::execute("pkill", QStringList() << "-u" << username);

    QTimer::singleShot(500, this, &MainWindow::refreshStats);
}

void MainWindow::on_disableStartupButton_clicked() 
{
    qDebug("Disable app startup button clicked (not implemented).");
}

void MainWindow::on_endTaskDetailsButton_clicked() 
{
    qDebug("End task details button clicked (not implemented).");
}


void MainWindow::refreshStats()
{
    CpuTimes currentCpuTimes = parser.getCpuTimes();
    long deltaTotal = currentCpuTimes.totalTime() - prevCpuTimes.totalTime();
    
    double timeIntervalSec = processTimer->interval() / 1000.0;
    if (timeIntervalSec == 0) timeIntervalSec = 1.0; 

    std::map<std::string, long> currentProcessJiffies;
    std::map<std::string, IoStats> currentProcessIo;
    
    QMap<QString, long> userMemoryMap;
    QMap<QString, double> userCpuMap;
    QMap<QString, double> userDiskReadMap; 
    QMap<QString, double> userDiskWriteMap;

    ui->processTable->setSortingEnabled(false);
    ui->processTable->setRowCount(0); 
    ui->usersTreeWidget->clear();

    std::vector<std::string> pids = parser.getPids();
    for (const std::string& pid_std : pids) 
    {
        ProcessInfo info = parser.getProcessInfo(pid_std);
        
        long currentJiffies = parser.getProcessActiveJiffies(pid_std);
        currentProcessJiffies[pid_std] = currentJiffies;
        long prevJiffies = 0;
        auto it = prevProcessJiffies.find(pid_std);
        if (it != prevProcessJiffies.end()) prevJiffies = it->second;
        long deltaProc = currentJiffies - prevJiffies;
        double cpuPercent = 0.0;
        if (deltaTotal > 0) {
            cpuPercent = (static_cast<double>(deltaProc) / static_cast<double>(deltaTotal)) * 100.0;
        }

        IoStats currentIo = parser.getProcessIoBytes(pid_std);
        currentProcessIo[pid_std] = currentIo;
        IoStats prevIo;
        auto io_it = prevProcessIo.find(pid_std);
        if (io_it != prevProcessIo.end()) prevIo = io_it->second;

        double readRate = (static_cast<double>(currentIo.readBytes - prevIo.readBytes)) / timeIntervalSec;
        double writeRate = (static_cast<double>(currentIo.writeBytes - prevIo.writeBytes)) / timeIntervalSec;

        int row = ui->processTable->rowCount();
        ui->processTable->insertRow(row);
        
        bool ok;
        QTableWidgetItem *nameItem = new QTableWidgetItem(QString::fromStdString(info.name));
        QTableWidgetItem *pidItem = new QTableWidgetItem();
        qlonglong pidNum = QString::fromStdString(info.pid).toLongLong(&ok);
        if (ok) pidItem->setData(Qt::DisplayRole, pidNum);
        else pidItem->setText(QString::fromStdString(info.pid)); 
        
        QTableWidgetItem *cpuItem = new QTableWidgetItem();
        cpuItem->setData(Qt::DisplayRole, cpuPercent); 

        QTableWidgetItem *memItem = new QTableWidgetItem();
        memItem->setData(Qt::DisplayRole, static_cast<qlonglong>(info.memoryKb));
        
        QTableWidgetItem *userItem = new QTableWidgetItem(QString::fromStdString(info.owner));

        ui->processTable->setItem(row, 0, nameItem);
        ui->processTable->setItem(row, 1, pidItem);
        ui->processTable->setItem(row, 2, cpuItem);
        ui->processTable->setItem(row, 3, memItem);
        ui->processTable->setItem(row, 4, userItem);

        QString user = QString::fromStdString(info.owner);
        userMemoryMap[user] += info.memoryKb;
        userCpuMap[user] += cpuPercent;
        userDiskReadMap[user] += readRate;
        userDiskWriteMap[user] += writeRate;
    }

    prevCpuTimes = currentCpuTimes;
    prevProcessJiffies = currentProcessJiffies;
    prevProcessIo = currentProcessIo; 

    for (auto it = userMemoryMap.constBegin(); it != userMemoryMap.constEnd(); ++it) {
        QString user = it.key();
        long memoryKb = it.value();
        double cpu = userCpuMap.value(user, 0.0);
        double diskRead = userDiskReadMap.value(user, 0.0);
        double diskWrite = userDiskWriteMap.value(user, 0.0);

        QTreeWidgetItem *item = new QTreeWidgetItem(ui->usersTreeWidget);
        
        QString diskString = QString("R: %1 | W: %2")
                                 .arg(formatBytesRate(diskRead))
                                 .arg(formatBytesRate(diskWrite));

        item->setText(0, user);
        item->setData(1, Qt::DisplayRole, cpu);
        item->setData(2, Qt::DisplayRole, static_cast<qlonglong>(memoryKb)); 
        item->setText(3, diskString); 
        item->setText(4, "N/A");      
    }

    ui->processTable->setSortingEnabled(true);
}





