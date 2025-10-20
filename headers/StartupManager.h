#ifndef STARTUP_MANAGER_H

#define STARTUP_MANAGER_H



#include <string>
#include <vector>
#include <filesystem>

struct StartupItem {
    std::string name;
    std::string command;
    std::string path;
    bool enabled;
};

class StartupManager {
public:
    std::vector<StartupItem> getSystemAutostartItems();
    std::vector<StartupItem> getUserAutostartItems();

    bool setAutostartItemEnabled(const std::string& itemPath,bool enabled);

private:
    std::vector<StartupItem> parseAutostartDirectory(const std::string& dirPath);
};

#endif // STARTUP_MANAGER_H
