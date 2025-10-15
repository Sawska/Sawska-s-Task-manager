#include "headers/ServiceManager.h"
#include <sstream>


std::string ServiceManager::executeCommand(const std::string& cmd) {
    std::string result;
    char buffer[128];
    FILE* pipe = popen(cmd.c_str(), "r"); 
    if (!pipe) return "";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

std::vector<ServiceInfo> ServiceManager::listServices() {
    std::vector<ServiceInfo> services;
    std::string output = executeCommand("systemctl list-units --type=service --no-pager --plain");
    
    std::stringstream ss(output);
    std::string line;
    std::getline(ss, line); 

    while (std::getline(ss, line)) {
        if (line.empty() || line.find("UNIT") != std::string::npos) continue;
        
        ServiceInfo info;
        std::stringstream lineStream(line);
        std::string subState;
        
        lineStream >> info.name >> info.loadState >> info.state >> subState;
        
        std::string description;
        std::getline(lineStream, description);
        if (!description.empty()) {
            size_t firstChar = description.find_first_not_of(' ');
            if (firstChar != std::string::npos) {
                info.description = description.substr(firstChar);
            }
        }
        services.push_back(info);
    }
    return services;
}

bool ServiceManager::startService(const std::string& serviceName) {
    return std::system(("systemctl start " + serviceName).c_str()) == 0;
}

bool ServiceManager::stopService(const std::string& serviceName) {
    return std::system(("systemctl stop " + serviceName).c_str()) == 0;
}

bool ServiceManager::restartService(const std::string& serviceName) {
    return std::system(("systemctl restart " + serviceName).c_str()) == 0;
}