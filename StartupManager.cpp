#include "headers/StartupManager.h"
#include <fstream>
#include <iostream>

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

static std::string ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\n\r\f\v");
    return (start == std::string::npos) ? "" : s.substr(start);
}

bool StartupManager::setAutostartItemEnabled(const std::string& itemPath, bool enabled) {
    
    std::vector<std::string> lines;
    std::string line;
    
    std::ifstream inFile(itemPath);
    if (!inFile.is_open()) {
        std::cerr << "Error: Could not open file for reading: " << itemPath << std::endl;
        return false;
    }

    bool inDesktopEntrySection = false;
    size_t desktopEntryLineIndex = -1; 
    
    while (std::getline(inFile, line)) {
        std::string trimmedLine = ltrim(line);

        if (trimmedLine.find("[Desktop Entry]") == 0) {
            inDesktopEntrySection = true;
            desktopEntryLineIndex = lines.size();
        
        } else if (trimmedLine.find("[") == 0 && trimmedLine.find("]") != std::string::npos) {
            inDesktopEntrySection = false;
        }

        if (inDesktopEntrySection && trimmedLine.find("Hidden=") == 0) {
            continue; 
        }
        
        lines.push_back(line);
    }
    inFile.close();

    if (!enabled) {
        if (desktopEntryLineIndex != (size_t)-1) {
            lines.insert(lines.begin() + desktopEntryLineIndex + 1, "Hidden=true");
        } else {
            lines.insert(lines.begin(), "Hidden=true");
        }
    }

    std::ofstream outFile(itemPath);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open file for writing: " << itemPath << std::endl;
        return false;
    }

    for (const std::string& newLine : lines) {
        outFile << newLine << "\n";
    }
    outFile.close();

    if (!outFile.good()) {
         std::cerr << "Error: Failed to write to file: " << itemPath << std::endl;
        return false;
    }

    return true;
}