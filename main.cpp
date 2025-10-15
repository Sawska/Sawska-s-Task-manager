#include "headers/processParser.h"
#include "headers/ServiceManager.h"
#include "headers/StartupManager.h" 
#include <iostream>
#include <vector>
#include <string>
#include <map>     
#include <thread> 
#include <chrono> 
#include <algorithm> 


int main()
{
    ProcessParser parser;
    ServiceManager serviceManager;
    StartupManager startupManager;

    std::cout << "--- Static System Info ---\n";
    std::cout << "Processor Specs: " << parser.getProcessorSpecs() << std::endl;
    std::cout << "Kernel Version: " << parser.getKernelVersion() << std::endl;
    std::cout << "--------------------------\n\n";

    std::cout << "--- Live System Stats (Update 1) ---\n";
    CpuTimes prevSysTimes = parser.getCpuTimes();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    CpuTimes currSysTimes = parser.getCpuTimes();

    long deltaTotal = currSysTimes.totalTime() - prevSysTimes.totalTime();
    long deltaIdle = currSysTimes.totalIdleTime() - prevSysTimes.totalIdleTime();
    float cpuPercent = 100.0 * (deltaTotal - deltaIdle) / deltaTotal;

    std::cout << "Total CPU Usage: " << cpuPercent << "%\n";
    std::cout << "Memory Info: " << parser.getMemoryInfo() << std::endl;
    std::cout << "Uptime: " << parser.getUptime() << std::endl;
    std::cout << "Load Average: " << parser.getLoadAvg() << std::endl;
    std::cout << "Network Stats:\n" << parser.getNetworkStats() << std::endl;
    std::cout << "Disk Stats (I/O, total): " << parser.getSystemDiskStats() << "\n";
    std::cout << "-------------------------------------\n\n";

    std::cout << "--- Process List (Top 10) ---\n";
    std::vector<std::string> pids = parser.getPids();
    std::map<std::string, long> prevProcJiffies;

    for (const auto &pid : pids)
    {
        prevProcJiffies[pid] = parser.getProcessActiveJiffies(pid);
    }
    long prevTotalSysJiffies = parser.getCpuTimes().totalTime();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    long currTotalSysJiffies = parser.getCpuTimes().totalTime();
    long deltaSys = currTotalSysJiffies - prevTotalSysJiffies;

    std::cout << "PID\tCPU %\tMEM (KB)\tOWNER\t\tNAME\n";
    std::cout << "-----------------------------------------------------------------\n";

    for (const auto &pid : pids)
    {
         
        
        ProcessInfo info = parser.getProcessInfo(pid);
        long currProcJiffies = parser.getProcessActiveJiffies(pid);
        
        long deltaProc = (prevProcJiffies.count(pid) > 0) ? (currProcJiffies - prevProcJiffies[pid]) : 0;
        
        float procCpuPercent = 0.0;
        if (deltaSys > 0)
        {
            procCpuPercent = 100.0 * deltaProc / deltaSys;
        }

        std::cout << info.pid << "\t";
        printf("%.2f%%\t", procCpuPercent); 
        std::cout << info.memoryKb << "\t\t";
        std::cout << info.owner.substr(0, 10) << (info.owner.length() < 10 ? "\t" : "") << "\t";
        std::cout << info.name.substr(0, 20) << "\n";

    }
    std::cout << "---------------------\n\n";

    
    std::cout << "--- Users and Logs ---\n";
    std::vector<std::string> users = parser.getLoggedInUsers();
    std::cout << "Logged In Users: ";
    for (const auto& user : users) {
        std::cout << user << " ";
    }
    std::cout << "\n\n";

    std::cout << "--- Last 5 Systemd Log Messages (journalctl) ---\n";
    std::cout << parser.getSystemLogs(5) << std::endl;
    std::cout << "-------------------------------------\n\n";
    std::cout << "--- Process Details Test ---\n";
    std::string testPid = "1"; 
    if (pids.size() > 1) {
        testPid = pids[1]; 
    }
    
    std::cout << "Details for PID " << testPid << ":\n";
    std::cout << "  Executable Path: " << parser.getProcessExecutablePath(testPid) << "\n";
    std::cout << "  I/O Stats: " << parser.getProcessIoStats(testPid) << "\n";

    ProcessInfo infoForSearch = parser.getProcessInfo(testPid);
    if (infoForSearch.name != "process_terminated") {
        std::cout << "  Search link for '" << infoForSearch.name << "' generated successfully.\n"; 
    }
    std::cout << "--------------------------\n\n";

    std::cout << "--- Service Manager Test (Requires sudo/permissions!) ---\n";
    std::vector<ServiceInfo> services = serviceManager.listServices();
    std::cout << "Total Active Services: " << services.size() << "\n";
    
    for (size_t i = 0; i < std::min((size_t)3, services.size()); ++i) {
        std::cout << "  [" << services[i].state << "] " << services[i].name << " - " << services[i].description << "\n";
    }

    std::cout << "\nAttempting to stop a non-critical service (e.g., cron.service): \n";
    if (serviceManager.stopService("cron.service")) {
        std::cout << "  Stop signal sent.\n";
    } else {
        std::cout << "  Failed to stop (Need root/sudo).\n";
    }
    std::cout << "----------------------------------------\n\n";

    std::cout << "--- Startup Manager Test ---\n";
    std::vector<StartupItem> userItems = startupManager.getUserAutostartItems();
    std::vector<StartupItem> systemItems = startupManager.getSystemAutostartItems();

    std::cout << "User Autostart Items (" << userItems.size() << " found):\n";
    for (const auto& item : userItems) {
        std::cout << "  " << (item.enabled ? "[ON] " : "[OFF]") << item.name << " Command: " << item.command << "\n";
    }

    std::cout << "System Autostart Items (" << systemItems.size() << " found):\n";
    for (const auto& item : systemItems) {
        std::cout << "  " << (item.enabled ? "[ON] " : "[OFF]") << item.name << " Command: " << item.command << "\n";
    }
    std::cout << "--------------------------\n\n";

    std::cout << "--- Test Process Termination ---\n";
    std::cout << "Enter the PID of a non-critical process to terminate (e.g., a text editor, or '0' to skip): ";
    std::string pidToKill;
    std::cin >> pidToKill;

    if (pidToKill != "0")
    {
        std::cout << "Attempting to terminate process " << pidToKill << "...\n";
        bool success = parser.terminateProcess(pidToKill);
        if (success)
        {
            std::cout << "Signal sent successfully.\n";
        }
        else
        {
            std::cout << "Failed to send signal. The process may not exist or you may lack permissions.\n";
        }
    }
    std::cout << "-------------------------------\n";

    return 0;
}