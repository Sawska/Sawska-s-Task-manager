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


class ProcessParser {
    public:
    void outputHardwareSpecs();
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
    
};


#endif // PROCESSPARSER_H

