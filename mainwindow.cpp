#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>
#include <QMap>
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QProcess>
#include <QString>
#include <QTableWidgetItem>
#include "headers/StartupManager.h"
#include <QMenu>         
#include <QTimer>        
#include <QColor>              

QString formatKB(long kb) {
    if (kb == 0) return "0.0 MB";
    if (kb < 1024 * 1024) { 
        return QString::number(kb / 1024.0, 'f', 1) + " MB";
    } else {
        return QString::number(kb / (1024.0 * 1024.0), 'f', 1) + " GB";
    }
}

QString formatBytes(long bytes) {
    if (bytes < 1024) {
        return QString::number(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        return QString::number(bytes / 1024.0, 'f', 1) + " KB";
    } else {
        return QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
    }
}

QString formatBitsRate(double bitsPerSec) {
    if (bitsPerSec < 1000 * 1000) {
        return QString::number(bitsPerSec / 1000.0, 'f', 1) + " Kbps";
    } else {
         return QString::number(bitsPerSec / (1000.0 * 1000.0), 'f', 1) + " Mbps";
    }
}

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

    ui->startupTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->startupTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->startupTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    m_servicesRefreshCounter = 0;

    ui->detailsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->detailsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->detailsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->detailsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);
    ui->detailsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    ui->detailsTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);

    ui->resourceUsageLabel->setText("Recent system log entries (from journalctl / syslog):");

    ui->deleteHistoryButton->setToolTip("Deleting system logs requires root permissions and is not supported.");

    ui->appHistoryTable->setColumnCount(3);
    ui->appHistoryTable->setHorizontalHeaderLabels(QStringList() 
        << "Timestamp" 
        << "Process" 
        << "Message");


    ui->appHistoryTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    ui->appHistoryTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    ui->appHistoryTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->appHistoryTable->setColumnWidth(0, 160); 
    ui->appHistoryTable->setColumnWidth(1, 140); 

    ui->servicesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    ui->servicesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->servicesTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->servicesTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui->servicesTable->setColumnWidth(0, 250); 

    ui->servicesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->servicesTable, &QTableWidget::customContextMenuRequested,
            this, &MainWindow::on_servicesTable_customContextMenuRequested);
            
    connect(ui->tabWidget, &QTabWidget::currentChanged,
            this, &MainWindow::on_tabWidget_currentChanged);



    connect(ui->startupTable, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::updateStartupButtonState);

    ui->disableStartupButton->setEnabled(false);


    populateStartupTable();

    processTimer = new QTimer(this);
    connect(processTimer, &QTimer::timeout, this, &MainWindow::refreshStats);
    
    prevCpuTimes = parser.getCpuTimes();

    setupPerformanceTab();

    m_prevDiskStats = parser.getDiskStats();
    m_prevNetStats = parser.getNetworkStats();
    m_prevDiskIOTime = m_prevDiskStats.timeSpentIO;

    m_currentGpuStats = parser.getAmdGpuStats();
    
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
    int currentRow = ui->startupTable->currentRow();
    if (currentRow < 0) {
        qDebug("No startup item selected.");
        return;
    }

    QTableWidgetItem *nameItem = ui->startupTable->item(currentRow, 0);
    if (!nameItem) return;

    QString itemPath = nameItem->data(Qt::UserRole).toString();
    bool currentStatus = nameItem->data(Qt::UserRole + 1).toBool();
    bool newStatus = !currentStatus;
    
    QString action = newStatus ? "enable" : "disable";
    QString itemName = nameItem->text();

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, QString("Confirm %1").arg(action),
                                  QString("Are you sure you want to %1 '%2'?")
                                      .arg(action)
                                      .arg(itemName),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::No) {
        return;
    }

    bool success = startupManager.setAutostartItemEnabled(itemPath.toStdString(), newStatus);

    if (success) {
        qDebug() << "Successfully" << action << "d:" << itemPath;
        QMessageBox::information(this, "Success",
                                 QString("'%1' has been %2d.").arg(itemName).arg(action));
        populateStartupTable();
    } else {
        qDebug() << "Failed to" << action << ":" << itemPath;
        QMessageBox::critical(this, "Error",
                              QString("Failed to %1 '%2'.").arg(action).arg(itemName));
    }
}


void MainWindow::populateStartupTable()
{
    disconnect(ui->startupTable, &QTableWidget::itemSelectionChanged,
               this, &MainWindow::updateStartupButtonState);

    ui->startupTable->setSortingEnabled(false);
    ui->startupTable->setRowCount(0);

    std::vector<StartupItem> items = startupManager.getSystemAutostartItems();
    std::vector<StartupItem> userItems = startupManager.getUserAutostartItems();
    items.insert(items.end(), userItems.begin(), userItems.end());

    for (const auto& item : items) {
        int row = ui->startupTable->rowCount();
        ui->startupTable->insertRow(row);

        QTableWidgetItem *nameItem = new QTableWidgetItem(QString::fromStdString(item.name));
        nameItem->setData(Qt::UserRole, QString::fromStdString(item.path));
        nameItem->setData(Qt::UserRole + 1, item.enabled);

        QTableWidgetItem *commandItem = new QTableWidgetItem(QString::fromStdString(item.command));
        QTableWidgetItem *statusItem = new QTableWidgetItem(item.enabled ? "Enabled" : "Disabled");

        if (!item.enabled) {
            QFont font = nameItem->font();
            font.setItalic(true);
            nameItem->setForeground(Qt::gray);
            nameItem->setFont(font);
            commandItem->setForeground(Qt::gray);
            commandItem->setFont(font);
            statusItem->setForeground(Qt::gray);
            statusItem->setFont(font);
        }

        ui->startupTable->setItem(row, 0, nameItem);
        ui->startupTable->setItem(row, 1, commandItem);
        ui->startupTable->setItem(row, 2, statusItem);
    }

    ui->startupTable->setSortingEnabled(true);
    
    ui->startupTable->clearSelection();
    ui->disableStartupButton->setEnabled(false);
    ui->disableStartupButton->setText("Enable/Disable");

    connect(ui->startupTable, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::updateStartupButtonState);
}

void MainWindow::updateStartupButtonState()
{
    int currentRow = ui->startupTable->currentRow();
    if (currentRow < 0) {
        ui->disableStartupButton->setEnabled(false);
        ui->disableStartupButton->setText("Enable/Disable");
        return;
    }

    QTableWidgetItem *nameItem = ui->startupTable->item(currentRow, 0);
    if (!nameItem) return;

    bool isEnabled = nameItem->data(Qt::UserRole + 1).toBool();

    if (isEnabled) {
        ui->disableStartupButton->setText("Disable");
    } else {
        ui->disableStartupButton->setText("Enable");
    }
    
    ui->disableStartupButton->setEnabled(true);
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    if (ui->tabWidget->widget(index) == ui->servicesTab) {
        m_servicesRefreshCounter = 0;
        populateServicesTable();    
    }

    if (ui->tabWidget->widget(index) == ui->appHistoryTab) {
        populateAppHistoryTable();
    }
}

void MainWindow::populateServicesTable()
{
    int currentServiceRow = ui->servicesTable->currentRow();

    ui->servicesTable->setSortingEnabled(false);
    ui->servicesTable->setRowCount(0); 

    std::vector<ServiceInfo> services = m_serviceManager.listServices();

    for (const auto& service : services) {
        int row = ui->servicesTable->rowCount();
        ui->servicesTable->insertRow(row);

        QTableWidgetItem *nameItem = new QTableWidgetItem(QString::fromStdString(service.name));
        nameItem->setData(Qt::UserRole, QString::fromStdString(service.name)); 
        
        QTableWidgetItem *descItem = new QTableWidgetItem(QString::fromStdString(service.description));
        QTableWidgetItem *statusItem = new QTableWidgetItem(QString::fromStdString(service.state));
        QTableWidgetItem *loadItem = new QTableWidgetItem(QString::fromStdString(service.loadState));

        if (service.state == "active" || service.state == "running") {
            statusItem->setForeground(QColor(0, 100, 0)); 
        } else if (service.state == "failed") {
            statusItem->setForeground(Qt::red);
        } else if (service.state == "inactive" || service.state == "dead") {
            statusItem->setForeground(Qt::gray);
        }

        ui->servicesTable->setItem(row, 0, nameItem);
        ui->servicesTable->setItem(row, 1, descItem);
        ui->servicesTable->setItem(row, 2, statusItem);
        ui->servicesTable->setItem(row, 3, loadItem);
    }

    ui->servicesTable->setSortingEnabled(true);

    if (currentServiceRow >= 0 && currentServiceRow < ui->servicesTable->rowCount()) {
        ui->servicesTable->selectRow(currentServiceRow);
    }
}

void MainWindow::on_openServicesButton_clicked()
{
    qDebug("Attempting to launch gnome-system-monitor...");
    QProcess::startDetached("gnome-system-monitor");
}

void MainWindow::on_servicesTable_customContextMenuRequested(const QPoint &pos)
{
    QTableWidgetItem *item = ui->servicesTable->itemAt(pos);
    if (!item) {
        return; 
    }

    QMenu contextMenu(this);
    contextMenu.addAction("Start", this, &MainWindow::startSelectedService);
    contextMenu.addAction("Stop", this, &MainWindow::stopSelectedService);
    contextMenu.addAction("Restart", this, &MainWindow::restartSelectedService);
    
    contextMenu.exec(ui->servicesTable->viewport()->mapToGlobal(pos));
}

void MainWindow::startSelectedService()
{
    int row = ui->servicesTable->currentRow();
    if (row < 0) return;

    QString serviceName = ui->servicesTable->item(row, 0)->data(Qt::UserRole).toString();
    if (serviceName.isEmpty()) return;

    qDebug() << "Attempting to START service:" << serviceName;
    m_serviceManager.startService(serviceName.toStdString());
    
    QTimer::singleShot(500, this, &MainWindow::populateServicesTable);
}

void MainWindow::stopSelectedService()
{
    int row = ui->servicesTable->currentRow();
    if (row < 0) return;

    QString serviceName = ui->servicesTable->item(row, 0)->data(Qt::UserRole).toString();
    if (serviceName.isEmpty()) return;

    qDebug() << "Attempting to STOP service:" << serviceName;
    m_serviceManager.stopService(serviceName.toStdString());
    
    QTimer::singleShot(500, this, &MainWindow::populateServicesTable);
}

void MainWindow::restartSelectedService()
{
    int row = ui->servicesTable->currentRow();
    if (row < 0) return;

    QString serviceName = ui->servicesTable->item(row, 0)->data(Qt::UserRole).toString();
    if (serviceName.isEmpty()) return;

    qDebug() << "Attempting to RESTART service:" << serviceName;
    m_serviceManager.restartService(serviceName.toStdString());
    
    QTimer::singleShot(500, this, &MainWindow::populateServicesTable);
}




void MainWindow::on_deleteHistoryButton_clicked()
{
    QMessageBox::information(this, "Operation Not Permitted",
        "Deleting system-wide logs is a privileged (root) operation and "
        "is not supported by this application for safety reasons.");
}

void MainWindow::populateAppHistoryTable()
{
    ui->appHistoryTable->setSortingEnabled(false);
    ui->appHistoryTable->setRowCount(0);

    std::string logData = parser.getSystemLogs(500);
    if (logData.empty()) {
        qDebug("getSystemLogs returned empty data.");
        return;
    }

    QString qLogData = QString::fromStdString(logData);
    QStringList lines = qLogData.split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        if (line.isEmpty()) continue;

        QString timestamp, process, message;

        
        timestamp = line.left(15); 
        QString rest = line.mid(16).trimmed();
        
        int colonPos = rest.indexOf(':');
        
        if (colonPos > 0 && colonPos < 40) { 
            process = rest.left(colonPos);
            int bracketPos = process.indexOf('[');
            if (bracketPos != -1) {
                process = process.left(bracketPos);
            }
            int spacePos = process.lastIndexOf(' ');
            if (spacePos != -1) {
                process = process.mid(spacePos + 1);
            }
            message = rest.mid(colonPos + 1).trimmed();
        } else {
            process = "system";
            message = rest;
        }

        int row = ui->appHistoryTable->rowCount();
        ui->appHistoryTable->insertRow(row);

        ui->appHistoryTable->setItem(row, 0, new QTableWidgetItem(timestamp));
        ui->appHistoryTable->setItem(row, 1, new QTableWidgetItem(process.trimmed()));
        ui->appHistoryTable->setItem(row, 2, new QTableWidgetItem(message));
    }

    ui->appHistoryTable->setSortingEnabled(true);
    ui->appHistoryTable->scrollToBottom();
}

void MainWindow::on_endTaskDetailsButton_clicked() 
{
    int currentRow = ui->detailsTable->currentRow();
    if (currentRow < 0) {
        qDebug("No process selected in details table.");
        return; 
    }

    QTableWidgetItem *pidItem = ui->detailsTable->item(currentRow, 1);
    if (!pidItem) {
        qDebug("Could not find PID item for selected row in details table.");
        return; 
    }

    QString pidString = pidItem->text();
    std::string pid_std_string = pidString.toStdString();

    qDebug() << "Attempting to terminate PID from details table:" << pidString;

    bool success = parser.terminateProcess(pid_std_string);

    if (success) {
        qDebug() << "Process" << pidString << "terminated.";
    } else {
        qDebug() << "Failed to terminate process" << pidString;
    }

    QTimer::singleShot(500, this, &MainWindow::refreshStats);
}


void MainWindow::setupGraph(QCustomPlot* graph, const QColor& color, bool twoLines)
{
    graph->addGraph(); 
    graph->graph(0)->setPen(QPen(color));
    graph->graph(0)->setBrush(QBrush(QColor(color.red(), color.green(), color.blue(), 20)));

    if (twoLines) {
        graph->addGraph(); 
        QColor color2 = (color == Qt::blue ? Qt::red : Qt::blue); 
        graph->graph(1)->setPen(QPen(color2));
    }


    QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
    dateTicker->setDateTimeFormat("hh:mm:ss");
    dateTicker->setTickStepStrategy(QCPAxisTicker::tssReadability);
    dateTicker->setTickCount(5); 
    graph->xAxis->setTicker(dateTicker);


    graph->yAxis->setRange(0, 100); 
    graph->axisRect()->setupFullAxesBox();
    graph->legend->setVisible(false);
}

void MainWindow::setupPerformanceTab()
{
    ui->performanceList->addItem("CPU");
    ui->performanceList->addItem("Memory");
    
    std::string diskName = parser.getPrimaryDiskName();
    diskName.erase(std::remove_if(diskName.begin(), diskName.end(), ::isdigit), diskName.end());
    ui->performanceList->addItem(QString("Disk 0 (%1)").arg(QString::fromStdString(diskName)));
    
    std::string netIface = parser.getPrimaryNetworkInterface();
    ui->performanceList->addItem(QString("Ethernet (%1)").arg(QString::fromStdString(netIface)));
    
    ui->performanceList->addItem("GPU 0");
    
    connect(ui->performanceList, &QListWidget::currentRowChanged,
            this, &MainWindow::on_performanceList_currentRowChanged);
    
    ui->cpuNameLabel->setText(QString::fromStdString(parser.getProcessorSpecs()));
    ui->coresLabel->setText(QString::number(parser.getCoreCount()));
    ui->logicalProcessorsLabel->setText(QString::number(parser.getLogicalProcessorCount()));
    ui->diskNameLabel->setText(QString("Disk 0 (%1)").arg(QString::fromStdString(diskName)));
    ui->netInterfaceLabel->setText(QString("Ethernet (%1)").arg(QString::fromStdString(netIface)));
    
    setupGraph(ui->cpuGraph, Qt::blue);
    setupGraph(ui->memoryGraph, Qt::magenta);
    setupGraph(ui->diskGraph, Qt::green);
    setupGraph(ui->ethernetGraph, Qt::cyan, true);

    setupGraph(ui->gpuGraph, QColor(220, 50, 50));
    
    ui->performanceList->setCurrentRow(0);
}

void MainWindow::on_performanceList_currentRowChanged(int row)
{
    ui->performanceStackedWidget->setCurrentIndex(row);
    switch(row) {
        case 0: ui->cpuGraph->replot(); break;
        case 1: ui->memoryGraph->replot(); break;
        case 2: ui->diskGraph->replot(); break;
        case 3: ui->ethernetGraph->replot(); break;
    }
}

void MainWindow::refreshStats()
{

    CpuTimes currentCpuTimes = parser.getCpuTimes();
    long deltaTotal = currentCpuTimes.totalTime() - prevCpuTimes.totalTime();
    long deltaIdle = currentCpuTimes.totalIdleTime() - prevCpuTimes.totalIdleTime();
    double cpuUsagePercent = 0.0;
    if (deltaTotal > 0) {
        cpuUsagePercent = (1.0 - (static_cast<double>(deltaIdle) / static_cast<double>(deltaTotal))) * 100.0;
    }

    MemInfo memInfo = parser.getMemoryStats();
    double memUsagePercent = 0.0;
    if (memInfo.memTotal > 0) {
        memUsagePercent = (static_cast<double>(memInfo.memUsed()) / memInfo.memTotal) * 100.0;
    }

    DiskStats currentDiskStats = parser.getDiskStats();
    double timeIntervalSec = processTimer->interval() / 1000.0;
    
    double readSpeed = (currentDiskStats.sectorsRead - m_prevDiskStats.sectorsRead) * 512.0 / timeIntervalSec;
    double writeSpeed = (currentDiskStats.sectorsWritten - m_prevDiskStats.sectorsWritten) * 512.0 / timeIntervalSec;
    
    long deltaIOTime = currentDiskStats.timeSpentIO - m_prevDiskIOTime;
    double diskActivePercent = (static_cast<double>(deltaIOTime) / (timeIntervalSec * 1000.0)) * 100.0;
    if (diskActivePercent > 100.0) diskActivePercent = 100.0;
    
    FsInfo fsInfo = parser.getFilesystemStats("/");

    NetStats currentNetStats = parser.getNetworkStats();
    double sendSpeed = (currentNetStats.bytesSent - m_prevNetStats.bytesSent) * 8.0 / timeIntervalSec;
    double recvSpeed = (currentNetStats.bytesReceived - m_prevNetStats.bytesReceived) * 8.0 / timeIntervalSec;


    std::map<std::string, long> currentProcessJiffies;
    std::map<std::string, IoStats> currentProcessIo;
    
    QMap<QString, long> userMemoryMap;
    QMap<QString, double> userCpuMap;
    QMap<QString, double> userDiskReadMap; 
    QMap<QString, double> userDiskWriteMap;

    ui->processTable->setSortingEnabled(false);
    ui->processTable->setRowCount(0); 
    ui->detailsTable->setSortingEnabled(false);
    ui->detailsTable->setRowCount(0);
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
        double procCpuPercent = 0.0;
        if (deltaTotal > 0) {
            procCpuPercent = (static_cast<double>(deltaProc) / static_cast<double>(deltaTotal)) * 100.0;
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
        cpuItem->setData(Qt::DisplayRole, procCpuPercent); 

        QTableWidgetItem *memItem = new QTableWidgetItem();
        memItem->setData(Qt::DisplayRole, static_cast<qlonglong>(info.memoryKb));
        
        QTableWidgetItem *userItem = new QTableWidgetItem(QString::fromStdString(info.owner));

        ui->processTable->setItem(row, 0, nameItem);
        ui->processTable->setItem(row, 1, pidItem);
        ui->processTable->setItem(row, 2, cpuItem);
        ui->processTable->setItem(row, 3, memItem);
        ui->processTable->setItem(row, 4, userItem);

        int detailsRow = ui->detailsTable->rowCount();
        ui->detailsTable->insertRow(detailsRow);
        
        QTableWidgetItem *d_pidItem = new QTableWidgetItem();
        d_pidItem->setData(Qt::DisplayRole, pidNum);
        
        ui->detailsTable->setItem(detailsRow, 0, nameItem->clone());
        ui->detailsTable->setItem(detailsRow, 1, d_pidItem);
        ui->detailsTable->setItem(detailsRow, 2, new QTableWidgetItem(QString::fromStdString(info.state)));
        ui->detailsTable->setItem(detailsRow, 3, userItem->clone());
        ui->detailsTable->setItem(detailsRow, 4, cpuItem->clone());
        ui->detailsTable->setItem(detailsRow, 5, memItem->clone());

        QString user = QString::fromStdString(info.owner);
        userMemoryMap[user] += info.memoryKb;
        userCpuMap[user] += procCpuPercent;
        userDiskReadMap[user] += readRate;
        userDiskWriteMap[user] += writeRate;
    }
    
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
    ui->detailsTable->setSortingEnabled(true);




    m_servicesRefreshCounter++;
    if (m_servicesRefreshCounter >= 5) {
        m_servicesRefreshCounter = 0; 
        if (ui->tabWidget->currentWidget() == ui->servicesTab) {
            populateServicesTable();
        }
    }
    
  m_currentGpuStats = parser.getAmdGpuStats(); 

  std::string usageStr = m_currentGpuStats.gpuUsage;
    double gpuUsagePercent = 0.0;
    try {
        size_t pos;
        pos = usageStr.find(" ");
        if (pos != std::string::npos) {
            usageStr = usageStr.substr(0, pos); 
        }
        gpuUsagePercent = std::stod(usageStr);
    } catch (const std::exception& e) {
        gpuUsagePercent = 0.0; 
    }

    ui->gpuUsageLabel->setText(QString::fromStdString(m_currentGpuStats.gpuUsage));
    
    QString gpuNameText = QString::fromStdString(m_currentGpuStats.deviceName);
    QString gpuVendor = QString::fromStdString(m_currentGpuStats.vendor);

    if (gpuNameText == "N/A" || gpuNameText.isEmpty()) {
        ui->gpuNameLabel->setText("GPU: Detection Failed or N/A");
    } else {
        ui->gpuNameLabel->setText(gpuNameText + (gpuVendor != "N/A" ? " (" + gpuVendor + ")" : ""));
    }

    
    ui->gpuUsageLabel->setText(QString::fromStdString(m_currentGpuStats.gpuUsage));
    ui->gpuTempLabel->setText(QString::fromStdString(m_currentGpuStats.gpuTemp));
    
    QString vramText = QString("%1 / %2")
        .arg(QString::fromStdString(m_currentGpuStats.vramUsed))
        .arg(QString::fromStdString(m_currentGpuStats.vramTotal));
    ui->gpuVramLabel->setText(vramText);
    
    ui->gpuFanLabel->setText(QString::fromStdString(m_currentGpuStats.fanSpeed));
    ui->gpuPowerLabel->setText(QString::fromStdString(m_currentGpuStats.powerUsage));
    
  double currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0;
    m_timeData.append(currentTime);
    m_cpuData.append(cpuUsagePercent);
    m_memData.append(memUsagePercent);
    m_diskData.append(diskActivePercent);
    m_netSendData.append(sendSpeed / 1000.0); 
    m_netRecvData.append(recvSpeed / 1000.0);


    m_gpuUsageData.append(gpuUsagePercent);

    while (m_timeData.size() > 60) {
        m_timeData.removeFirst();
        m_cpuData.removeFirst();
        m_memData.removeFirst();
        m_diskData.removeFirst();
        m_netSendData.removeFirst();
        m_netRecvData.removeFirst();
        m_gpuUsageData.removeFirst();
    }
    
    ui->cpuUsageLabel->setText(QString::number(cpuUsagePercent, 'f', 1) + " %");
    ui->cpuSpeedLabel->setText(QString::fromStdString(parser.getCurrentCpuMhz()));
    ui->uptimeLabel->setText(QString::fromStdString(parser.getUptime()));
    ui->processesLabel->setText(QString::number(pids.size()));
    ui->threadsLabel->setText(QString::number(parser.getTotalThreads()));

    ui->memUsageLabel->setText(QString("%1 / %2")
        .arg(formatKB(memInfo.memUsed()))
        .arg(formatKB(memInfo.memTotal)));
    ui->memAvailableLabel->setText(formatKB(memInfo.memAvailable));
    ui->memCachedLabel->setText(formatKB(memInfo.cached));
    ui->swapUsageLabel->setText(QString("%1 / %2")
        .arg(formatKB(memInfo.swapUsed()))
        .arg(formatKB(memInfo.swapTotal)));

    ui->diskActiveTimeLabel->setText(QString::number(diskActivePercent, 'f', 0) + " %");
    ui->diskReadSpeedLabel->setText(formatBytesRate(readSpeed));
    ui->diskWriteSpeedLabel->setText(formatBytesRate(writeSpeed));
    ui->diskCapacityLabel->setText(QString("%1 / %2 GB")
        .arg(fsInfo.used() / (1024.0*1024.0*1024.0), 0, 'f', 1)
        .arg(fsInfo.total / (1024.0*1024.0*1024.0), 0, 'f', 1));

    ui->netSendLabel->setText(formatBitsRate(sendSpeed));
    ui->netReceiveLabel->setText(formatBitsRate(recvSpeed));

    int currentIndex = ui->performanceStackedWidget->currentIndex();
    QCustomPlot* currentGraph = qobject_cast<QCustomPlot*>(ui->performanceStackedWidget->widget(currentIndex)->findChild<QCustomPlot*>());
    
    if (currentGraph) {
        currentGraph->xAxis->setRange(currentTime, 60, Qt::AlignRight);
        
        if (currentIndex == 0) { 
            ui->cpuGraph->graph(0)->setData(m_timeData, m_cpuData);
            ui->cpuGraph->yAxis->setRange(0, 100);
        } else if (currentIndex == 1) { 
            ui->memoryGraph->graph(0)->setData(m_timeData, m_memData);
            ui->memoryGraph->yAxis->setRange(0, 100);
        } else if (currentIndex == 2) { 
            ui->diskGraph->graph(0)->setData(m_timeData, m_diskData);
            ui->diskGraph->yAxis->setRange(0, 100);
        } else if (currentIndex == 3) { 
            ui->ethernetGraph->graph(0)->setData(m_timeData, m_netRecvData);
            ui->ethernetGraph->graph(1)->setData(m_timeData, m_netSendData);
            double maxRecv = m_netRecvData.isEmpty() ? 100.0 : *std::max_element(m_netRecvData.constBegin(), m_netRecvData.constEnd());
            double maxSend = m_netSendData.isEmpty() ? 100.0 : *std::max_element(m_netSendData.constBegin(), m_netSendData.constEnd());
            double maxSpeed = qMax(100.0, qMax(maxRecv, maxSend)); 
            ui->ethernetGraph->yAxis->setRange(0, maxSpeed * 1.1); 
        }
        
        currentGraph->replot();
    }

    if (ui->tabWidget->currentWidget() == ui->gpuPage) {
        ui->gpuGraph->xAxis->setRange(currentTime, 60, Qt::AlignRight);
        ui->gpuGraph->graph(0)->setData(m_timeData, m_gpuUsageData);
        ui->gpuGraph->yAxis->setRange(0, 100);
        ui->gpuGraph->replot();
    }

    prevCpuTimes = currentCpuTimes;
    prevProcessJiffies = currentProcessJiffies;
    prevProcessIo = currentProcessIo;
    m_prevDiskStats = currentDiskStats;
    m_prevNetStats = currentNetStats;
    m_prevDiskIOTime = currentDiskStats.timeSpentIO;
}