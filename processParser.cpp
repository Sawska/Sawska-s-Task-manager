#include "headers/processParser.h"
#include <iostream>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <fstream>
#include <stdexcept>
#include <map>
#include <iomanip> 
#include <cctype>
#include <signal.h>
#include <sys/statvfs.h>

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

return specs["model name"];

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

std::string ProcessParser::getProcessIoStats(const std::string& pid) {
    std::string ioFilePath = "/proc/" + pid + "/io";
    std::ifstream ioFile(ioFilePath);
    if (!ioFile.is_open()) {
        return "N/A";
    }

    long readBytes = 0;
    long writeBytes = 0;
    std::string line;

    while (std::getline(ioFile, line)) {
        if (line.rfind("read_bytes:", 0) == 0) {
            std::string value = getSpecValue(line);
            try {
                readBytes = std::stol(value);
            } catch (const std::invalid_argument& e) {
                readBytes = 0;
            }
        } else if (line.rfind("write_bytes:", 0) == 0) {
            std::string value = getSpecValue(line);
            try {
                writeBytes = std::stol(value);
            } catch (const std::invalid_argument& e) {
                writeBytes = 0;
            }
        }
    }

    return "R: " + formatBytes(readBytes) + " | W: " + formatBytes(writeBytes);
}

std::string ProcessParser::getProcessExecutablePath(const std::string& pid) {
    std::string exePath = "/proc/" + pid + "/exe";
    std::string realPath;
    
    std::vector<char> buf(4096);
    ssize_t len = readlink(exePath.c_str(), buf.data(), buf.size() - 1);

    if (len != -1) {
        buf[len] = '\0';
        realPath = std::string(buf.data());
        return realPath;
    }

    return "N/A (Access Denied or Terminated)";
}

bool ProcessParser::searchProcessOnInternet(const std::string& processName) {
        std::string url = "https://www.google.com/search?q=what+is+" + processName + "+process";
    
    
    std::replace(url.begin(), url.end(), ' ', '+');

    std::string command = "xdg-open " + url;
    

    return std::system((command + " &").c_str()) == 0;
}

std::vector<std::string> ProcessParser::getLoggedInUsers() {
    std::vector<std::string> users;
 
    
    FILE* pipe = popen("who | awk '{print $1}' | sort -u", "r");
    if (!pipe) return {"Error executing command"};

    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        std::string user_line(buffer);
        trim(user_line);
        if (!user_line.empty()) {
            users.push_back(user_line);
        }
    }

    pclose(pipe);
    return users;
}


std::string ProcessParser::getSystemDiskStats() {
    std::ifstream diskStatsFile("/proc/diskstats");
    if (!diskStatsFile.is_open()) {
        return "N/A (Could not open /proc/diskstats)";
    }

    std::stringstream result;
    std::string line;
    long totalReads = 0;
    long totalWrites = 0;
    
    
    while (std::getline(diskStatsFile, line)) {
        std::stringstream ss(line);
        int major, minor;
        std::string device;
        long reads, writes;
        long temp;

        ss >> major >> minor >> device;
        
        if (device.find("loop") == 0 || device.find("ram") == 0 || device.find("dm-") == 0) {
            continue;
        }

        ss >> reads;
        ss >> temp; 
        ss >> temp; 
        ss >> temp; 

        ss >> writes;
        
        ss >> temp;
        ss >> temp;
        long writtenSectors = temp;
        
        ss.clear();
        ss.seekg(0, std::ios::beg);

        ss >> major >> minor >> device >> temp >> temp;
        long readSectors;
        ss >> readSectors;

        totalReads += readSectors;
        totalWrites += writtenSectors;
    }

    long readBytes = totalReads * 512;
    long writeBytes = totalWrites * 512;

    result << "Total Read: " << formatBytes(readBytes)
           << " | Total Write: " << formatBytes(writeBytes);

    return result.str();
}


std::string ProcessParser::executeCommand(const std::string& cmd) {
    std::string result;
    char buffer[128];
    FILE* pipe = popen(cmd.c_str(), "r"); 
    if (!pipe) {
        return "Error: Could not execute command.";
    }
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}


std::string ProcessParser::getSystemLogs(int numLines) {
    if (numLines <= 0) {
        return "Error: Invalid number of lines requested.";
    }

 
    std::string command = "journalctl -n " + std::to_string(numLines) + " --no-pager";
    
    return executeCommand(command);
}


IoStats ProcessParser::getProcessIoBytes(const std::string& pid) {
    std::string ioFilePath = "/proc/" + pid + "/io";
    std::ifstream ioFile(ioFilePath);
    IoStats stats;

    if (!ioFile.is_open()) {
        return stats; 
    }

    std::string line;
    while (std::getline(ioFile, line)) {
        if (line.rfind("read_bytes:", 0) == 0) {
            std::string value = getSpecValue(line);
            try {
                stats.readBytes = std::stol(value);
            } catch (const std::invalid_argument& e) {
                stats.readBytes = 0;
            }
        } else if (line.rfind("write_bytes:", 0) == 0) {
            std::string value = getSpecValue(line);
            try {
                stats.writeBytes = std::stol(value);
            } catch (const std::invalid_argument& e) {
                stats.writeBytes = 0;
            }
        }
    }
    return stats;
}

int ProcessParser::getCoreCount() {
    std::ifstream stream("/proc/cpuinfo");
    if (!stream.is_open()) return 0;
    std::string line;
    std::set<std::string> coreIds;
    std::string currentPhysicalId;

    while (std::getline(stream, line)) {
        if (line.find("physical id") == 0) {
            currentPhysicalId = line.substr(line.find(":") + 2);
        }
        if (line.find("core id") == 0) {
            std::string coreId = line.substr(line.find(":") + 2);
            coreIds.insert(currentPhysicalId + ":" + coreId);
        }
    }
    return coreIds.size() > 0 ? coreIds.size() : 1;
}

int ProcessParser::getLogicalProcessorCount() {
    std::ifstream stream("/proc/cpuinfo");
    if (!stream.is_open()) return 0;
    std::string line;
    int count = 0;
    while (std::getline(stream, line)) {
        if (line.find("processor") == 0) {
            count++;
        }
    }
    return count > 0 ? count : 1;
}

int ProcessParser::getTotalThreads() {
    int totalThreads = 0;
    try {
        for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
            if (entry.is_directory()) {
                std::string pid = entry.path().filename().string();
                if (std::all_of(pid.begin(), pid.end(), ::isdigit)) {
                    std::ifstream stream(entry.path().string() + "/status");
                    std::string line;
                    while (std::getline(stream, line)) {
                        if (line.find("Threads:") == 0) {
                            totalThreads += std::stoi(line.substr(line.find(":") + 1));
                            break;
                        }
                    }
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
    }
    return totalThreads;
}


long getMemInfoValue(std::ifstream& stream, const std::string& key) {
    stream.clear();
    stream.seekg(0);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find(key) == 0) {
            try {
                return std::stol(line.substr(line.find(":") + 1));
            } catch (...) {
                return 0;
            }
        }
    }
    return 0;
}

MemInfo ProcessParser::getMemoryStats() {
    MemInfo info;
    std::ifstream stream("/proc/meminfo");
    if (!stream.is_open()) return info;

    info.memTotal = getMemInfoValue(stream, "MemTotal:");
    info.memFree = getMemInfoValue(stream, "MemFree:");
    info.memAvailable = getMemInfoValue(stream, "MemAvailable:");
    info.buffers = getMemInfoValue(stream, "Buffers:");
    info.cached = getMemInfoValue(stream, "Cached:");
    info.swapTotal = getMemInfoValue(stream, "SwapTotal:");
    info.swapFree = getMemInfoValue(stream, "SwapFree:");

    return info;
}

FsInfo ProcessParser::getFilesystemStats(const std::string& path) {
    FsInfo info;
    struct statvfs stat;
    if (statvfs(path.c_str(), &stat) != 0) {
        return info; 
    }
    info.total = stat.f_blocks * stat.f_frsize;
    info.free = stat.f_bavail * stat.f_frsize;
    return info;
}

std::string ProcessParser::getPrimaryDiskName() {
    std::ifstream stream("/proc/mounts");
    std::string line;
    while (std::getline(stream, line)) {
        std::stringstream ss(line);
        std::string device, mountPoint;
        ss >> device >> mountPoint;
        if (mountPoint == "/") {
            if (device.find("/dev/") == 0) {
                return device.substr(5); 
            }
            return device;
        }
    }
    return "sda"; 
}

DiskStats ProcessParser::getDiskStats() {
    DiskStats stats;
    std::string primaryDisk = getPrimaryDiskName();
    primaryDisk.erase(std::remove_if(primaryDisk.begin(), primaryDisk.end(), ::isdigit), primaryDisk.end());

    std::ifstream stream("/proc/diskstats");
    std::string line;
    while (std::getline(stream, line)) {
        std::stringstream ss(line);
        std::string name;
        ss >> name >> name >> name; 
        if (name == primaryDisk) {
            ss >> stats.sectorsRead;
            ss >> name >> name; 
            ss >> stats.sectorsWritten;
            ss >> name >> name; 
            ss >> stats.timeSpentIO;
            break;
        }
    }
    return stats;
}

std::string ProcessParser::getPrimaryNetworkInterface() {
    std::ifstream stream("/proc/net/route");
    std::string line;
    while (std::getline(stream, line)) {
        std::stringstream ss(line);
        std::string iface, dest;
        ss >> iface >> dest;
        if (dest == "00000000") { 
             return iface;
        }
    }
    return "eth0"; 
}

NetStats ProcessParser::getNetworkStats() {
    NetStats stats;
    std::string iface = getPrimaryNetworkInterface();
    std::ifstream stream("/proc/net/dev");
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find(iface) != std::string::npos) {
            std::stringstream ss(line);
            std::string name;
            ss >> name; 
            ss >> stats.bytesReceived;
            for (int i = 0; i < 7; ++i) ss >> line; 
            ss >> stats.bytesSent;
            break;
        }
    }
    return stats;
}


bool ProcessParser::isRocmSmiAvailable() {
    std::string result = executeCommand("which rocm-smi 2>/dev/null");
    trim(result);
    return !result.empty();
}


AmdGpuInfo ProcessParser::getAmdGpuStats() {
    AmdGpuInfo info;
    info.deviceName = "N/A";
    info.vendor = "AMD";
    info.vramTotal = "N/A";
    info.vramUsed = "N/A";
    info.gpuTemp = "N/A";
    info.fanSpeed = "N/A";
    info.gpuUsage = "N/A";
    info.powerUsage = "N/A";

   if (isRocmSmiAvailable()) {
        std::string command = "rocm-smi --showid --showmeminfo vram --showtemp --showfan --showuse --showpower --csv";
        std::string output = executeCommand(command);
        trim(output); 

        if (!output.empty()) {
            std::string lastLine;

            std::stringstream lineStream(output);
            std::string currentLine;
            while (std::getline(lineStream, currentLine)) {
                trim(currentLine);
                if (!currentLine.empty()) {
                    lastLine = currentLine;
                }
            }

            if (!lastLine.empty()) {
                std::stringstream ss(lastLine);
                std::string segment;
                std::vector<std::string> parts;
                
                while (std::getline(ss, segment, ',')) {
                    trim(segment);
                    parts.push_back(segment);
                }

                if (parts.size() >= 14) { 
                    info.deviceName = parts[1];
                    info.vramTotal = parts[12];
                    info.vramUsed = parts[13];
                    info.gpuTemp = parts[6];
                    info.fanSpeed = parts[8];  
                    info.gpuUsage = parts[11];
                    info.powerUsage = parts[10];
                }
            }
        }
    } else {
        try {
            std::string drmPath = "/sys/class/drm/";
            for (const auto& entry : std::filesystem::directory_iterator(drmPath)) {
                std::string filename = entry.path().filename().string();
                if (filename.rfind("card", 0) == 0 && std::all_of(filename.begin() + 4, filename.end(), ::isdigit)) {
                    std::ifstream deviceFile(entry.path().string() + "/device/uevent");
                    std::string line;
                    while (std::getline(deviceFile, line)) {
                        if (line.find("DRIVER=amdgpu") != std::string::npos) {
                            std::ifstream nameFile(entry.path().string() + "/device/vendor");
                            std::string vendorID;
                            if (std::getline(nameFile, vendorID) && vendorID == "0x1002") { 
                                info.vendor = "AMD";
                                
                                std::ifstream vramTotalFile(entry.path().string() + "/device/mem_info_vram_total");
                                long totalBytes = 0;
                                if (vramTotalFile >> totalBytes) {
                                    info.vramTotal = formatBytes(totalBytes);
                                }
                                
                                std::ifstream vramUsedFile(entry.path().string() + "/device/mem_info_vram_used");
                                long usedBytes = 0;
                                if (vramUsedFile >> usedBytes) {
                                    info.vramUsed = formatBytes(usedBytes);
                                }
                                
                                std::ifstream nameFile2(entry.path().string() + "/device/modalias");
                                std::string modalias;
                                if (std::getline(nameFile2, modalias)) {
                                    info.deviceName = modalias;
                                }
                                
                                break; 
                            }
                        }
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
             info.deviceName = "Error reading sysfs";
        }
    }

    return info;
}

std::string ProcessParser::formatAmdGpuInfoToJson(const AmdGpuInfo& info) {
    std::string jsonOutput = "{\n";
    jsonOutput += "  \"device_name\": \"" + info.deviceName + "\",\n";
    jsonOutput += "  \"vendor\": \"" + info.vendor + "\",\n";
    jsonOutput += "  \"vram_total\": \"" + info.vramTotal + "\",\n";
    jsonOutput += "  \"vram_used\": \"" + info.vramUsed + "\",\n";
    jsonOutput += "  \"gpu_temp\": \"" + info.gpuTemp + "\",\n";
    jsonOutput += "  \"fan_speed\": \"" + info.fanSpeed + "\",\n";
    jsonOutput += "  \"gpu_usage\": \"" + info.gpuUsage + "\",\n";
    jsonOutput += "  \"power_usage\": \"" + info.powerUsage + "\"\n";
    jsonOutput += "}";
    return jsonOutput;
}


std::vector<std::string> ProcessParser::getMountedPartitions() {
    std::set<std::string> partitions_set;
    std::ifstream mountsFile("/proc/mounts");
    
    if (!mountsFile.is_open()) {
        return {"/"};
    }

    std::string line;
    while (std::getline(mountsFile, line)) {
        std::stringstream ss(line);
        std::string device, mountPoint, fsType;
        ss >> device >> mountPoint >> fsType;

        if (device.rfind("/dev/", 0) == 0 && 
            fsType != "squashfs" && 
            fsType != "tmpfs") 
        {
            partitions_set.insert(mountPoint);
        }
    }

    partitions_set.insert("/"); 
    
    std::vector<std::string> partitions(partitions_set.begin(), partitions_set.end());
    return partitions;
}


std::string ProcessParser::getDeviceForMountPoint(std::string mountPath) {
    std::ifstream file("/proc/mounts");
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string devicePath, mount;
        if (!(iss >> devicePath >> mount)) { continue; }
        if (mount == mountPath) {
            // Strip "/dev/" from the path (e.g., "/dev/sda1" -> "sda1")
            if (devicePath.rfind("/dev/", 0) == 0) {
                return devicePath.substr(5);
            }
            return devicePath;
        }
    }
    return "unknown"; // Return a default if not found
}

// This is your new *overloaded* function to get stats for one specific device
DiskStats ProcessParser::getDiskStats(std::string deviceName) {
    std::ifstream file("/proc/diskstats");
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        int major, minor;
        std::string device;
        if (!(iss >> major >> minor >> device)) continue;

        if (device == deviceName) {
            DiskStats stats = {};
            std::string token;
            // Parse the /proc/diskstats line.
            // Column 6 (index 5) is sectorsRead
            // Column 10 (index 9) is sectorsWritten
            // Column 13 (index 12) is timeSpentIO
            for (int i = 0; i < 3; ++i) iss >> token; // Skip (reads merged, etc.)
            iss >> stats.sectorsRead; // Col 6
            for (int i = 0; i < 3; ++i) iss >> token; // Skip (time_read, etc.)
            iss >> stats.sectorsWritten; // Col 10
            iss >> token; // Skip (writes_merged)
            iss >> token; // Skip (time_write)
            iss >> stats.timeSpentIO; // Col 13
            return stats;
        }
    }
    return {}; // Return empty stats if not found
}