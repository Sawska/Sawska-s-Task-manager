#include "headers/StartupManager.h"
#include <fstream>

namespace fs = std::filesystem;

std::vector<StartupItem> StartupManager::parseAutostartDirectory(const std::string& dirPath) {
    std::vector<StartupItem> items;
    if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) {
        return items;
    }

    for (const auto& entry : fs::directory_iterator(dirPath)) {
        if (entry.path().extension() == ".desktop") {
            StartupItem item;
            item.path = entry.path().string();
            item.enabled = true; 

            std::ifstream file(item.path);
            std::string line;
            while (std::getline(file, line)) {
                if (line.rfind("Name=", 0) == 0) {
                    item.name = line.substr(5);
                } else if (line.rfind("Exec=", 0) == 0) {
                    item.command = line.substr(5);
                } else if (line.rfind("Hidden=true", 0) == 0) {
                    item.enabled = false;
                }
            }
            if (!item.name.empty()) {
                items.push_back(item);
            }
        }
    }
    return items;
}

std::vector<StartupItem> StartupManager::getSystemAutostartItems() {
    return parseAutostartDirectory("/etc/xdg/autostart");
}

std::vector<StartupItem> StartupManager::getUserAutostartItems() {
    const char* homeDir = std::getenv("HOME");
    if (homeDir) {
        return parseAutostartDirectory(std::string(homeDir) + "/.config/autostart");
    }
    return {};
}