#ifndef SERVICE_MANAGER_H

#define SERVICE_MANAGER_H

#include <string>
#include <vector>
#include <map>

struct ServiceInfo {
    std::string name;
    std::string description;
    std::string state; 
    std::string loadState;
};

class ServiceManager {
private:
    std::string executeCommand(const std::string& cmd);
    ServiceInfo parseSystemctlLine(const std::string& line);

public:
    std::vector<ServiceInfo> listServices();
    bool startService(const std::string& serviceName);
    bool stopService(const std::string& serviceName);
    bool restartService(const std::string& serviceName);
};



#endif // SERVICE_MANAGER_H