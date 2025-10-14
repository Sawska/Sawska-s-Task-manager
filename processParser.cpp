#include "headers/processParser.h"
#include <iostream>
#include <algorithm>
#include <map>
#include <string>
#include <fstream>
#include <stdexcept>
#include <map>
#include <iomanip> 
#include <cctype>
#include <signal.h>

std::vector<std::string> ProcessParser::readProcessFileSystem(const std::string& filePath) {
    std::vector<std::string> processes;



    if(!std::filesystem::exists(filePath) || !std::filesystem::is_directory(filePath)) {
        std::cerr << "Error: Directory does not exists or is not a directory " << std::endl;
        return processes;
    }

    for(const auto& entry : std::filesystem::directory_iterator(filePath)) {
        if(entry.is_directory()) {
            std::string process_name = entry.path().filename().string();

            processes.push_back(process_name);
        } else {
            std::cerr << "entry not a dir" +  entry.path().filename().string() << std::endl;
        }
    }

    return processes;
}


void trim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

std::string getSpecValue(const std::string& line) {

    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) {
        return ""; 
    }


    std::string value = line.substr(colonPos + 1);
    
    trim(value);

    return value;
}


std::string ProcessParser::getProcessorSpecs() {


    std::map<std::string, std::string> specs = {
        {"vendor_id", ""},
        {"model name", ""},
        {"cpu cores", ""},
        {"cache size", ""}
    };
    size_t specsFound = 0;

    std::ifstream processorSpecsFile("/proc/cpuinfo");
    if (!processorSpecsFile.is_open()) {
        throw std::runtime_error("Could not open file: /proc/cpuinfo");
    }

    std::string line;
    while (specsFound < specs.size() && std::getline(processorSpecsFile, line)) {
        for (auto const& [key, val] : specs) {
            if (val.empty() && line.find(key) != std::string::npos) {
                specs[key] = getSpecValue(line);
                specsFound++;
                break;
            }
        }
    }


    std::string jsonOutput = "{\n";
    jsonOutput += "  \"vendor_name\": \"" + specs["vendor_id"] + "\",\n";
    jsonOutput += "  \"model_name\": \"" + specs["model name"] + "\",\n";
    jsonOutput += "  \"cores\": " + specs["cpu cores"] + ",\n";
    jsonOutput += "  \"cache_size\": \"" + specs["cache size"] + "\"\n";
    jsonOutput += "}";

    return jsonOutput;
}

std::string ProcessParser::getCurrentCpuMhz() {
    std::string mhz = "0"; 
    std::ifstream processorSpecsFile("/proc/cpuinfo");
    if (!processorSpecsFile.is_open()) {
        return mhz; 
    }

    std::string line;
    while (std::getline(processorSpecsFile, line)) {

        if (line.find("cpu MHz") != std::string::npos) {
            mhz = getSpecValue(line);
            break; 
        }
    }
    return mhz;
}


std::string ProcessParser::getMemoryInfo() { 
    std::map<std::string, std::string> memSpecs = {
        {"MemTotal", ""},
        {"MemAvailable", ""},
        {"SwapTotal", ""},
        {"SwapFree", ""}
    };
    size_t specsFound = 0;
    
    std::ifstream memFile("/proc/meminfo");
    if (!memFile.is_open()) {
        throw std::runtime_error("Could not open file: /proc/meminfo");
    }

    std::string line;
    while (specsFound < memSpecs.size() && std::getline(memFile, line)) {
        for (auto const& [key, val] : memSpecs) {
            if (val.empty() && line.find(key) != std::string::npos) {
                memSpecs[key] = getSpecValue(line);
                specsFound++;
                break;
            }
        }
    }

    long memTotal = std::stol(memSpecs["MemTotal"]);
    long memAvailable = std::stol(memSpecs["MemAvailable"]);
    long swapTotal = std::stol(memSpecs["SwapTotal"]);
    long swapFree = std::stol(memSpecs["SwapFree"]);

    float memUsagePercent = 0.0;
    if (memTotal > 0) {
        memUsagePercent = (static_cast<float>(memTotal - memAvailable) / memTotal) * 100.0;
    }

    float swapUsagePercent = 0.0;
    if (swapTotal > 0) {
        swapUsagePercent = (static_cast<float>(swapTotal - swapFree) / swapTotal) * 100.0;
    }

    std::string jsonOutput = "{\n";
    jsonOutput += "  \"mem_total_kb\": " + std::to_string(memTotal) + ",\n";
    jsonOutput += "  \"mem_available_kb\": " + std::to_string(memAvailable) + ",\n";
    jsonOutput += "  \"mem_usage_percent\": " + std::to_string(memUsagePercent) + ",\n";
    jsonOutput += "  \"swap_usage_percent\": " + std::to_string(swapUsagePercent) + "\n";
    jsonOutput += "}";

    return jsonOutput;
}

std::string ProcessParser::getUptime() {
    std::ifstream uptimeFile("/proc/uptime");
    if (!uptimeFile.is_open()) {
        throw std::runtime_error("Could not open file: /proc/uptime");
    }

    double totalSeconds;
  
    uptimeFile >> totalSeconds;


    long seconds = static_cast<long>(totalSeconds);


    long days = seconds / (24 * 3600);
    seconds %= (24 * 3600);
    long hours = seconds / 3600;
    seconds %= 3600;
    long minutes = seconds / 60;
    seconds %= 60;


    std::stringstream ss;
    ss << days << " days, "
       << std::setfill('0') << std::setw(2) << hours << ":"
       << std::setfill('0') << std::setw(2) << minutes << ":"
       << std::setfill('0') << std::setw(2) << seconds;

    return ss.str();
}

std::string ProcessParser::getKernelVersion() {
    std::ifstream versionFile("/proc/version");
    if (!versionFile.is_open()) {
        throw std::runtime_error("Could not open file: /proc/version");
    }

    std::string line;
    std::getline(versionFile, line);

    std::stringstream ss(line);

    std::string osName, versionText, kernelVersion;


    ss >> osName >> versionText >> kernelVersion;

    return kernelVersion;
}
std::string ProcessParser::getMountsInfo() {
    std::ifstream mountsFile("/proc/mounts");
    if (!mountsFile.is_open()) {
        throw std::runtime_error("Could not open file: /proc/mounts");
    }

    std::stringstream result;
    std::string line;
    
   
    while (std::getline(mountsFile, line)) {

        if (line.rfind("/dev/", 0) == 0) {
            std::stringstream ss(line);
            std::string device, mountPoint;
            
            ss >> device >> mountPoint;

            result << "Device: " << device << ", Mounted at: " << mountPoint << "\n";
        }
    }

    return result.str();
}


CpuTimes ProcessParser::getCpuTimes() {
    std::ifstream statFile("/proc/stat");
    if (!statFile.is_open()) {
        throw std::runtime_error("Could not open file: /proc/stat");
    }

    std::string line;
    std::getline(statFile, line);
    
    std::stringstream ss(line);
    CpuTimes times;
    std::string cpuLabel;

    ss >> cpuLabel >> times.user >> times.nice >> times.system >> times.idle
       >> times.iowait >> times.irq >> times.softirq >> times.steal;

    return times;
}

std::string ProcessParser::getLoadAvg() {
    std::ifstream loadFile("/proc/loadavg");
    if (!loadFile.is_open()) {
        throw std::runtime_error("Could not open file: /proc/loadavg");
    }

    float load1, load5, load15;
    loadFile >> load1 >> load5 >> load15;

    return "1m: " + std::to_string(load1) + " | 5m: " + std::to_string(load5) + " | 15m: " + std::to_string(load15);
}

std::string ProcessParser::getSwapInfo() {
    std::ifstream swapsFile("/proc/swaps");
    if (!swapsFile.is_open()) {
        throw std::runtime_error("Could not open file: /proc/swaps");
    }

    long totalSize = 0;
    long totalUsed = 0;
    std::string line;
    

    std::getline(swapsFile, line);

    while (std::getline(swapsFile, line)) {
        std::stringstream ss(line);
        std::string filename, type;
        long size, used, priority;

        ss >> filename >> type >> size >> used >> priority;
        totalSize += size;
        totalUsed += used;
    }

    if (totalSize == 0) {
        return "Swap Usage: 0.00%";
    }

    float usagePercent = 100.0 * totalUsed / totalSize;
    return "Swap Usage: " + std::to_string(usagePercent) + "%";
}




std::string formatBytes(long bytes) {
    std::stringstream ss;
    double value = static_cast<double>(bytes);

    if (bytes >= 1024 * 1024 * 1024) { 
        ss << std::fixed << std::setprecision(2) << value / (1024 * 1024 * 1024) << " GB";
    } else if (bytes >= 1024 * 1024) { 
        ss << std::fixed << std::setprecision(2) << value / (1024 * 1024) << " MB";
    } else if (bytes >= 1024) { 
        ss << std::fixed << std::setprecision(2) << value / 1024 << " KB";
    } else { 
        ss << bytes << " B";
    }
    return ss.str();
}

std::string ProcessParser::getNetworkStats() {
    std::stringstream result;
    std::ifstream netFile("/proc/net/dev");
    if (!netFile.is_open()) {
        throw std::runtime_error("Could not open file: /proc/net/dev");
    }

    std::string line;

    std::getline(netFile, line);
    std::getline(netFile, line);

    while (std::getline(netFile, line)) {
        std::stringstream ss(line);
        std::string interfaceName;
        long rx_bytes, tx_bytes;
        long temp;

        ss >> interfaceName;
        

        if (interfaceName == "lo:") {
            continue;
        }

        interfaceName.pop_back();


        ss >> rx_bytes;
        for (int i = 0; i < 7; ++i) ss >> temp; 
        ss >> tx_bytes;
        

        result << interfaceName << " - Rx: " << formatBytes(rx_bytes)
               << ", Tx: " << formatBytes(tx_bytes) << "\n";
    }
    
    return result.str();
}

bool is_numeric(const std::string& s) {
    if (s.empty()) {
        return false;
    }

    for(char c: s) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}


std::vector<std::string> ProcessParser::getPids() {
    std::vector<std::string> pids;

    const std::string proc_path = "/proc";

    try {
        for (const auto& entry : std::filesystem::directory_iterator(proc_path)) {

            if (entry.is_directory()) {
                std::string filename = entry.path().filename().string();
                
                
                if(is_numeric(filename)) {
                    pids.push_back(filename);
                }
            }
        }
    } catch (std::filesystem::filesystem_error& e) {
        throw std::runtime_error("Filesystem error while reading /proc " + std::string(e.what()));
    }

    return pids;
}


ProcessInfo ProcessParser::getProcessInfo(const std::string& pid) {
    ProcessInfo info;
    info.pid = pid;

    std::string memString, uidString; 
    std::string statusFilePath = "/proc/" + pid + "/status";

    std::map<std::string, std::string*> specMapping = {
        {"Name:", &info.name},
        {"State:", &info.state},
        {"PPid:", &info.ppid},
        {"Uid:", &uidString},   
        {"VmRSS:", &memString}
    };

    std::ifstream statusFile(statusFilePath);
    if (!statusFile.is_open()) {
        info.name = "process_terminated";
        return info;
    }

    std::string line;
    while (std::getline(statusFile, line)) {
        for (auto const& [key, valPtr] : specMapping) {
            if (line.rfind(key, 0) == 0) {
                *valPtr = getSpecValue(line);
                break;
            }
        }
    }

    std::string realUid;
    if (!uidString.empty()) {
        std::stringstream ss(uidString);
        ss >> realUid; 
    }
    info.owner = mapUidToUsername(realUid); 

    try {
        if (!memString.empty()) {
            size_t spacePos = memString.find(' ');
            if (spacePos != std::string::npos) {
                info.memoryKb = std::stol(memString.substr(0, spacePos));
            }
        }
    } catch (const std::invalid_argument& e) {
        info.memoryKb = 0;
    }
    
    return info;
}

std::string ProcessParser::formatProcessInfoToJson(const ProcessInfo& info) {
    std::string jsonOutput = "{\n";
    jsonOutput += "  \"pid\": \"" + info.pid + "\",\n";
    jsonOutput += "  \"name\": \"" + info.name + "\",\n";
    jsonOutput += "  \"state\": \"" + info.state + "\",\n";
    jsonOutput += "  \"owner\": \"" + info.owner + "\",\n";
    jsonOutput += "  \"ppid\": \"" + info.ppid + "\",\n";
    jsonOutput += "  \"memory_kb\": " + std::to_string(info.memoryKb) + "\n";
    jsonOutput += "}";
    return jsonOutput;
}

bool ProcessParser::stopProcess(const std::string& pid) {
    try {
       
        return kill(std::stoi(pid), SIGTERM) == 0;
    } catch (const std::invalid_argument& e) {
        return false; 
    }
}

bool ProcessParser::terminateProcess(const std::string& pid) {
    try {
        return kill(std::stoi(pid), SIGKILL) == 0;
    } catch (const std::invalid_argument& e) {
        return false; 
    }
}


std::string ProcessParser::mapUidToUsername(const std::string& uid) {
    std::ifstream passwdFile("/etc/passwd");

    if(!passwdFile.is_open()) {
        return uid;
    }

    std::string line;

    while (std::getline(passwdFile,line)) {
        std::stringstream ss(line);

        std::string username,placeholder,currentUid;

        std::getline(ss,username,':');
        std::getline(ss,placeholder,':');
        std::getline(ss,currentUid,':');

        if (currentUid == uid) {
            return username;
        }

    }
    return uid;
}

std::string ProcessParser::getProcessOwner(const std::string& pid) {
    std::ifstream statusFile("/proc/" + pid + "/status");

    if(!statusFile.is_open()) {
        return "N/A";
    }

    std::string line;
    

    while(std::getline(statusFile,line)) {
        if(line.rfind("Uid:",0) == 0) {
            std::stringstream ss(line);
            std::string key,uid;
            ss >> key >> uid;
            return mapUidToUsername(uid);
        }
    }
    return "N/A";
}


long ProcessParser::getProcessActiveJiffies(const std::string& pid) {
    std::ifstream statFile("/proc/" + pid + "/stat");

    if(!statFile.is_open()) {
        return 0;
    }

    std::string line;

    std::getline(statFile,line);

    size_t nameEndPos = line.rfind(')');

    if(nameEndPos == std::string::npos) {
        return 0;
    }


    std::stringstream ss(line.substr(nameEndPos+2));

    long utime,stime,cutime,cstime;
    std::string temp;

    for(int i =0;i<11;i++) {
        ss >> temp;
    }

    ss >> utime >> stime >> cutime >> cstime;

    return utime + stime + cutime + cstime;
}