# ifndef PROCESSPARSER_H

# define PROCESSPARSER_H

#include <string>
#include <fstream>
#include <vector>
#include <filesystem>
#include <map>

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


struct IoStats {
    long readBytes = 0;
    long writeBytes = 0;
};

struct ProcessInfo {
    std::string pid;
    std::string name;
    std::string state;
    std::string ppid;
    std::string owner;
    long memoryKb = 0;
};

struct MemInfo {
    long memTotal = 0;
    long memFree = 0;
    long memAvailable = 0;
    long buffers = 0;
    long cached = 0;
    long swapTotal = 0;
    long swapFree = 0;
    
    long memUsed() const { return memTotal - memAvailable; }
    long swapUsed() const { return swapTotal - swapFree; }
};

struct DiskStats {
    long sectorsRead = 0;
    long sectorsWritten = 0;
    long timeSpentIO = 0;
};

struct NetStats {
    long bytesReceived = 0;
    long bytesSent = 0;
};

struct FsInfo {
    long total = 0;
    long free = 0;
    long used() const { return total - free; }
};

struct AmdGpuInfo {
    std::string deviceName;
    std::string vendor;
    std::string vramTotal;
    std::string vramUsed;
    std::string gpuTemp;
    std::string fanSpeed;
    std::string gpuUsage; 
    std::string powerUsage;
};

class ProcessParser {
    public:
     std::string getProcessorSpecs();
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


    NetStats getNetworkStats();
    
    std::vector<std::string> getPids();

    ProcessInfo getProcessInfo(const std::string& pid);

    std::string formatProcessInfoToJson(const ProcessInfo& info);

    bool stopProcess(const std::string& pid);

    bool terminateProcess(const std::string& pid);

    std::string mapUidToUsername(const std::string& uid);

    std::string getProcessOwner(const std::string& pid);

    long getProcessActiveJiffies(const std::string& pid);

    std::string getProcessIoStats(const std::string& pid);

    std::string getProcessExecutablePath(const std::string& pid);

    bool searchProcessOnInternet(const std::string& processName);

    std::vector<std::string> getLoggedInUsers();
    
    std::string getSystemDiskStats();

    std::string executeCommand(const std::string& cmd);

    std::string getSystemLogs(int numLines);


    IoStats getProcessIoBytes(const std::string& pid);

    int getCoreCount();
    int getLogicalProcessorCount();
    int getTotalThreads();
    
    MemInfo getMemoryStats();
    FsInfo getFilesystemStats(const std::string& path);
    
    DiskStats getDiskStats();

    AmdGpuInfo getAmdGpuStats();

    bool isRocmSmiAvailable();

    std::string formatAmdGpuInfoToJson(const AmdGpuInfo& info);

    std::string getPrimaryDiskName();
    std::string getPrimaryNetworkInterface();

    std::vector<std::string> getMountedPartitions();
};


#endif // PROCESSPARSER_H

