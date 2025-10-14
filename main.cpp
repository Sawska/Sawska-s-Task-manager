#include "headers/processParser.h"
#include <vector>
#include <string>
#include <iostream>
#include <map>

#include <iostream>
#include <string>
#include <vector>
#include <thread> // For std::this_thread::sleep_for
#include <chrono> // For std::chrono::seconds

// Include your ProcessParser header
#include "headers/processParser.h"

int main() {
    ProcessParser parser;

    // --- 1. Print Static System Information (Once) ---
    std::cout << "--- Static System Info ---\n";
    std::cout << "Processor Specs: " << parser.getProcessorSpecs() << std::endl;
    std::cout << "Kernel Version: " << parser.getKernelVersion() << std::endl;
    std::cout << "--------------------------\n\n";


    // --- 2. Test Live System Stats (CPU Usage Calculation) ---
    std::cout << "--- Live System Stats (Update 1) ---\n";
    // Snapshot 1
    CpuTimes prevSysTimes = parser.getCpuTimes();

    // Wait for a second
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Snapshot 2
    CpuTimes currSysTimes = parser.getCpuTimes();
    
    // Calculate Total CPU Usage
    long deltaTotal = currSysTimes.totalTime() - prevSysTimes.totalTime();
    long deltaIdle = currSysTimes.totalIdleTime() - prevSysTimes.totalIdleTime();
    float cpuPercent = 100.0 * (deltaTotal - deltaIdle) / deltaTotal;

    std::cout << "Total CPU Usage: " << cpuPercent << "%\n";
    std::cout << "Memory Info: " << parser.getMemoryInfo() << std::endl;
    std::cout << "Uptime: " << parser.getUptime() << std::endl;
    std::cout << "Load Average: " << parser.getLoadAvg() << std::endl;
    std::cout << "-------------------------------------\n\n";


    // --- 3. List All Processes with their CPU Usage ---
    std::cout << "--- Process List ---\n";
    // Snapshot 1 for all processes
    std::vector<std::string> pids = parser.getPids();
    std::map<std::string, long> prevProcJiffies;
    for (const auto& pid : pids) {
        prevProcJiffies[pid] = parser.getProcessActiveJiffies(pid);
    }
    long prevTotalSysJiffies = parser.getCpuTimes().totalTime();
    
    // Wait for a second
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Snapshot 2 and display info
    long currTotalSysJiffies = parser.getCpuTimes().totalTime();
    long deltaSys = currTotalSysJiffies - prevTotalSysJiffies;
    
    for (const auto& pid : pids) {
        ProcessInfo info = parser.getProcessInfo(pid);
        long currProcJiffies = parser.getProcessActiveJiffies(pid);
        long deltaProc = currProcJiffies - prevProcJiffies[pid];
        float procCpuPercent = 0.0;
        if (deltaSys > 0) {
            procCpuPercent = 100.0 * deltaProc / deltaSys;
        }

        // Print process info, now including owner, ppid, and its specific CPU %
        std::cout << "PID: " << info.pid << " | Name: " << info.name
                  << " | Owner: " << info.owner << " | PPID: " << info.ppid
                  << " | Memory: " << info.memoryKb << " KB"
                  << " | CPU %: " << procCpuPercent << std::endl;
    }
    std::cout << "---------------------\n\n";


    // --- 4. Interactive Test for Terminating a Process ---
    std::cout << "--- Test Process Termination ---\n";
    std::cout << "Enter the PID of a non-critical process to terminate (e.g., a text editor, or '0' to skip): ";
    std::string pidToKill;
    std::cin >> pidToKill;

    if (pidToKill != "0") {
        std::cout << "Attempting to terminate process " << pidToKill << "...\n";
        bool success = parser.terminateProcess(pidToKill);
        if (success) {
            std::cout << "Signal sent successfully.\n";
        } else {
            std::cout << "Failed to send signal. The process may not exist or you may lack permissions.\n";
        }
    }
    std::cout << "-------------------------------\n";

    return 0;
}