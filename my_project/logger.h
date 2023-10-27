#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>
#include <filesystem>
using std::string;
class Logger {
public:
    Logger(const string& configFile = "config.json") {
        loadConfig(configFile);
        logFolder = "logs";
        createLogFolder();
        lastLogRotationTime = std::chrono::system_clock::now();
        openLogFile();
    }

    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    void log(const string& message) {
        if (logFile.is_open()) {
            while (true) {
                checkLogRotation();
                logFile << getCurrentUTCTime() << " " << message << std::endl;

                std::this_thread::sleep_for(std::chrono::seconds(logFrequencySeconds));
            }
        }
    }

private:
    string logFolder;
    std::ofstream logFile;
    size_t maxLogFiles;
    int logFrequencySeconds;
    std::chrono::system_clock::time_point lastLogRotationTime;

    void createLogFolder() {
        if (!std::filesystem::exists(logFolder)) {
            std::filesystem::create_directory(logFolder);
        }
    }

    void checkLogRotation() {
        auto now = std::chrono::system_clock::now();
        auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - lastLogRotationTime).count();
        if (elapsedSeconds >= logFrequencySeconds) {
            rotateLogFiles();
            lastLogRotationTime = now;
        }
    }

    void rotateLogFiles() {
        if (logFile.is_open()) {
            logFile.close();
        }

        cleanUpOldLogFiles();
        openLogFile();
    }

    void cleanUpOldLogFiles() {
        std::vector<std::filesystem::directory_entry> logFiles;
        for (const auto& entry : std::filesystem::directory_iterator(logFolder)) {
            if (entry.is_regular_file()) {
                logFiles.push_back(entry);
            }
        }

        if (logFiles.size() >= maxLogFiles) {
            std::sort(logFiles.begin(), logFiles.end(),
                      [](const std::filesystem::directory_entry& a, const std::filesystem::directory_entry& b) {
                          return a.last_write_time() < b.last_write_time();
                      });

            size_t filesToRemove = logFiles.size() - maxLogFiles + 1;
            for (size_t i = 0; i < filesToRemove; ++i) {
                std::filesystem::remove(logFiles[i].path());
            }
        }
    }

    void openLogFile() {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm timeInfo = *std::gmtime(&now);

        std::ostringstream filename;
        filename << logFolder << "/" << std::put_time(&timeInfo, "%Y-%m-%d-%H-%M-%S") << ".log";

        logFile.open(filename.str(), std::ios::app);
    }

    string getCurrentUTCTime() {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm timeInfo = *std::gmtime(&now);
        std::ostringstream ss;
        ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S UTC");
        return ss.str();
    }

    void loadConfig(const string& configFile) {
        std::ifstream logfile(configFile);
        if (logfile.is_open()) {
            string line;
            while (std::getline(logfile, line)) {
                size_t symbol = line.find(':');
                if (symbol != std::string::npos) {
                    string key = line.substr(0, symbol);
                    string value = line.substr(symbol + 1);
                    if (key == "maxLogFiles") {
                        maxLogFiles = std::stoi(value);
                    } else if (key == "logFrequencySeconds") {
                        logFrequencySeconds = std::stoi(value);
                    }
                }
            }
        }
    }
};

#endif
