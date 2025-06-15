#pragma once

#include <string>

struct SettingConfig {
    bool enableHttpProxy = false;
    std::string proxyAddress;
    int proxyPort = 0;

    std::string qbittorrentIP;
    int qbittorrentPort = 0;
    std::string qbittorrentUsername;
    std::string qbittorrentPassword;

    bool autoStart = false;

    void loadFromFile();
    void saveToFile() const;

    void loadFromJson(const std::string& jsonStr);
    std::string toJsonString() const;
};

// 全局配置对象
namespace GlobalConfig {
extern SettingConfig config;
}
