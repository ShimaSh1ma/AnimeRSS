#include "RssData.h"
#include "Constant.h"
#include "Qbittorrent.h"
#include "SettingConfig.h"
#include "SocketModule/ClientSocket.h"
#include "SocketModule/HttpRequest.h"
#include "SocketModule/UrlParser.h"
#include "WinToast.h"
#include "tinyxml2.h"

#include <codecvt>
#include <ctime>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>

using json = nlohmann::json;

RssData::RssData(const char* url, const char* savePath, const char* title) {
    body.url = url;
    body.savePath = savePath;
    body.title = title;
    setTimeStamp();
    saveAsJson();
}

std::string RssData::getTitle() {
    return body.title;
}

std::string RssData::getImage() {
    return body.imagePath.empty() ? "./Preface/" + this->getTitle() + "_" + body.timeStamp : body.imagePath;
}

std::string RssData::getUrl() {
    return body.url;
}

std::string RssData::getSavePath() {
    return body.savePath;
}

void RssData::setTimeStamp() {
    if (body.timeStamp.empty()) {
        auto now = std::chrono::system_clock::now();
        auto epoch_seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        body.timeStamp = std::to_string(epoch_seconds);
        jsonPath = R"(.\rss\)" + body.title + "_" + body.timeStamp + ".json";
    }
}

std::string RssData::getTimeStamp() {
    return body.timeStamp;
}

void RssData::setImage(const std::string& path) {
    if (!std::filesystem::exists(utf8_to_utf16(path)))
        return;
    std::filesystem::path targetPath = this->getImage();
    targetPath.replace_extension(std::filesystem::path(path).extension());
    if (std::filesystem::copy_file(utf8_to_utf16(path), utf8_to_utf16(targetPath.string()), std::filesystem::copy_options::overwrite_existing)) {
        body.imagePath = targetPath.string();
        saveAsJson();
    }
}

void RssData::setRead() {
    isRead = true;
    body.lastRead = body.lastUpdate;
    saveAsJson();
}

bool RssData::getRead() {
    return this->isRead;
}

void RssData::saveAsJson() {
    if (isDeleted)
        return;
    json j;
    j["url"] = body.url;
    j["savePath"] = body.savePath;
    j["title"] = body.title;
    j["imageUrl"] = body.imageUrl;
    j["imagePath"] = body.imagePath;
    j["lastUpdate"] = body.lastUpdate;
    j["lastRead"] = body.lastRead;
    j["timeStamp"] = body.timeStamp;
    j["isRead"] = isRead;

    j["messages"] = json::array();
    for (const auto& message : body.messages) {
        if (!message)
            continue;

        json item;
        item["title"] = message->title;
        item["link"] = message->link;
        item["pubDate"] = message->pubDate;
        item["description"] = message->description;
        item["torrentUrl"] = message->torrentUrl;
        item["downloaded"] = message->downloaded;
        j["messages"].push_back(item);
    }

    saveFile(jsonPath, j.dump(4), std::ios::trunc);
}

void RssData::loadFromJson(const std::string& _json) {
    json j = json::parse(_json);

    body.url = j.value("url", "");
    body.savePath = j.value("savePath", "");
    body.title = j.value("title", "");
    body.imageUrl = j.value("imageUrl", "");
    body.imagePath = j.value("imagePath", "");
    body.lastUpdate = j.value("lastUpdate", "");
    body.lastRead = j.value("lastRead", "");
    body.timeStamp = j.value("timeStamp", "");
    isRead = j.value("isRead", false);

    body.messages.clear();
    if (j.contains("messages")) {
        for (const auto& item : j["messages"]) {
            auto message = std::make_unique<RssMessage>();
            message->title = item.value("title", "");
            message->link = item.value("link", "");
            message->pubDate = item.value("pubDate", "");
            message->description = item.value("description", "");
            message->torrentUrl = item.value("torrentUrl", "");
            message->downloaded = item.value("downloaded", false);
            body.messages.push_back(std::move(message));
        }
    }

    jsonPath = R"(.\rss\)" + body.title + "_" + body.timeStamp + ".json";
}

void RssData::deleteData() {
    isDeleted.store(true);
    if (std::filesystem::exists(utf8_to_utf16(jsonPath))) {
        try {
            std::filesystem::remove(utf8_to_utf16(jsonPath));
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << e.what();
        }
    }

    if (!body.imagePath.empty() && std::filesystem::exists(utf8_to_utf16(body.imagePath))) {
        try {
            std::filesystem::remove(utf8_to_utf16(body.imagePath));
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << e.what();
        }
    }
}
