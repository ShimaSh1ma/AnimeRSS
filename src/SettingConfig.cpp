#include "SettingConfig.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
const std::string configPath = "./setting.json";
}

namespace GlobalConfig {
SettingConfig config;
}

void SettingConfig::loadFromJson(const std::string& _json) {
    json j = json::parse(_json, nullptr, false);
    if (j.is_discarded())
        return;

    enableHttpProxy = j.value("enableHttpProxy", false);
    proxyAddress = j.value("proxyAddress", "");
    proxyPort = j.value("proxyPort", 0);

    qbittorrentIP = j.value("qbittorrentIP", "");
    qbittorrentPort = j.value("qbittorrentPort", 0);
    qbittorrentUsername = j.value("qbittorrentUsername", "");
    qbittorrentPassword = j.value("qbittorrentPassword", "");

    autoStart = j.value("autoStart", false);
}

std::string SettingConfig::toJsonString() const {
    json j;
    j["enableHttpProxy"] = enableHttpProxy;
    j["proxyAddress"] = proxyAddress;
    j["proxyPort"] = proxyPort;

    j["qbittorrentIP"] = qbittorrentIP;
    j["qbittorrentPort"] = qbittorrentPort;
    j["qbittorrentUsername"] = qbittorrentUsername;
    j["qbittorrentPassword"] = qbittorrentPassword;

    j["autoStart"] = autoStart;

    return j.dump(4);
}

void SettingConfig::loadFromFile() {
    if (!std::filesystem::exists(configPath))
        return;

    std::ifstream in(configPath);
    if (!in.is_open())
        return;

    std::string jsonStr((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    loadFromJson(jsonStr);
}

void SettingConfig::saveToFile() const {
    std::ofstream out(configPath);
    if (!out.is_open())
        return;

    out << toJsonString();
    out.close();
}
