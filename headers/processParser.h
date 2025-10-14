# ifndef PROCESSPARSER_H

# define PROCESSPARSER_H

#include <string>
#include <fstream>
#include <vector>
#include <filesystem>


class CpuTimes {
public:
    long user, nice, system, idle, iowait, irq, softirq, steal;


    long totalActiveTime() const {
        return user + nice + system + irq + softirq + steal;
    }

    long totalIdleTime() const {
        return idle + iowait;
    }

    long totalTime() const {
        return totalActiveTime() + totalIdleTime();
    }
};


struct ProcessInfo {
    std::string pid;
    std::string name;
    std::string state;
    std::string ppid;
    std::string owner;
    long memoryKb = 0;
};


class ProcessParser {
    public:
     std::string getProcessorSpecs();
    // private:
    ProcessParser() = default;
    ~ProcessParser() = default;

    std::vector<std::string> readProcessFileSystem(const std::string& filePath);

    std::string getCurrentCpuMhz();

    std::string getMemoryInfo();

    std::string getUptime();

    std::string getKernelVersion();

    std::string getMountsInfo();

    CpuTimes getCpuTimes();

    std::string getLoadAvg();

    std::string getSwapInfo();


    std::string getNetworkStats();
    
    std::vector<std::string> getPids();

    ProcessInfo getProcessInfo(const std::string& pid);

    std::string formatProcessInfoToJson(const ProcessInfo& info);

    bool stopProcess(const std::string& pid);

    bool terminateProcess(const std::string& pid);

    std::string mapUidToUsername(const std::string& uid);

    std::string getProcessOwner(const std::string& pid);

    long getProcessActiveJiffies(const std::string& pid);
};


#endif // PROCESSPARSER_H

