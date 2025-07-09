#include "RssDataFunc.h"
#include "Constant.h"
#include "Qbittorrent.h"
#include "RssData.h"
#include "SettingConfig.h"
#include "SocketModule/ClientSocket.h"
#include "SocketModule/HttpRequest.h"
#include "SocketModule/UrlParser.h"
#include "WinToast.h"
#include "tinyxml2.h"

#include <ctime>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>

using namespace GlobalConfig;

inline std::tm parseRssPubDate(const std::string& pubDateStr) {
    std::tm tm = {};
    std::regex re(R"(\d{2}\s\w{3}\s\d{4}\s\d{2}:\d{2}:\d{2})");
    std::smatch match;
    if (std::regex_search(pubDateStr, match, re)) {
        std::string coreDate = match.str(0);
        std::istringstream ss(coreDate);
        ss.imbue(std::locale("C"));
        ss >> std::get_time(&tm, "%d %b %Y %H:%M:%S");
    }
    return tm;
}

void RssDataFunc::requestRss(std::shared_ptr<RssData> data) {
    bool expected = false;
    if (!data->isRequesting.compare_exchange_strong(expected, true)) {
        return;
    }

    auto uP = std::make_unique<UrlParser>();
    uP->parseUrl(data->body.url);

    auto req = std::make_unique<HttpRequest>();
    req->setUrl(*uP);
    req->addHttpHead({{"Connection", "keep-alive"}});

    std::weak_ptr<RssData> weakData = data;
    auto callBack = [weakData](const std::string& response) {
        if (auto lock = weakData.lock()) {
            if (!response.empty()) {
                lock->body.originRssHtml = response;
                parseRss(lock);
                lock->isRequesting.store(false);
                if (lock->callScheduler)
                    lock->callScheduler();
            }
        }
    };

    if (GlobalConfig::config.enableHttpProxy) {
        ClientSocket::asyncRequestProxy(config.proxyAddress, std::to_string(config.proxyPort), uP->host, "443", req->httpRequest(), std::move(callBack));
    } else {
        ClientSocket::asyncRequest(uP->host, "443", req->httpRequest(), std::move(callBack));
    }
}

void RssDataFunc::parseRss(std::shared_ptr<RssData> data) {
    tinyxml2::XMLDocument doc;
    if (doc.Parse(data->body.originRssHtml.c_str()) != tinyxml2::XML_SUCCESS) {
        return;
    }

    tinyxml2::XMLElement* root = doc.RootElement();
    if (!root) {
        return;
    }

    auto* channel = root->FirstChildElement("channel");
    if (!channel)
        return;

    for (tinyxml2::XMLElement* item = channel->FirstChildElement("item"); item != nullptr; item = item->NextSiblingElement("item")) {
        const char* title = item->FirstChildElement("title")->GetText();
        const char* link = item->FirstChildElement("link")->GetText();
        const char* pubDate = item->FirstChildElement("pubDate")->GetText();
        const char* description = item->FirstChildElement("description")->GetText();

        const char* torrentUrl = nullptr;
        auto enclosureElement = item->FirstChildElement("enclosure");
        if (enclosureElement) {
            torrentUrl = enclosureElement->Attribute("url");
        }

        if (std::any_of(data->body.messages.begin(), data->body.messages.end(),
                        [&](const std::unique_ptr<RssMessage>& body) { return body->torrentUrl == torrentUrl; })) {
            continue;
        }

        data->body.messages.emplace_back(std::make_unique<RssMessage>(RssMessage{
            title ? title : "",
            link ? link : "",
            pubDate ? pubDate : "",
            description ? description : "",
            torrentUrl ? torrentUrl : "",
        }));
    }

    checkUpdate(data);
    data->saveAsJson();
    parseImageUrl(data);
}

void RssDataFunc::parseImageUrl(std::shared_ptr<RssData> data) {
    if (!data->body.imageUrl.empty()) {
        if (!(data->body.imagePath.empty() && std::filesystem::exists(data->body.imagePath))) {
            requestImage(data);
        }
        return;
    }
    if (data->body.messages.empty()) {
        data->body.imageUrl = "";
        return;
    }
    std::regex imgRegex(R"(<img\s+[^>]*src=["']([^"']+)["'])");
    std::cmatch match;
    for (auto& it : data->body.messages) {
        if (std::regex_search(it.get()->description.c_str(), match, imgRegex)) {
            data->body.imageUrl = match[1].str();
            requestImage(data);
            return;
        }
    }
}

void RssDataFunc::requestImage(std::shared_ptr<RssData> data) {
    bool expected = false;
    if (!data->isImageRequesting.compare_exchange_strong(expected, true)) {
        return; // 已经在请求图片，直接返回
    }

    auto uP = std::make_unique<UrlParser>();
    uP->parseUrl(data->body.imageUrl);

    auto req = std::make_unique<HttpRequest>();
    req->setUrl(*uP);
    req->addHttpHead({{"connection", "close"}, {"", ""}});

    std::string fileName = uP->fileExtension.empty() ? "" : uP->fileName + data->body.timeStamp + uP->fileExtension;

    std::weak_ptr<RssData> weakData = data;
    auto callBack = [weakData, fileName](const std::string& response) {
        if (auto lock = weakData.lock()) {
            lock->isImageRequesting.store(false);
            if (!response.empty()) {
                lock->body.imagePath = fileName.empty() ? lock->getImage() : ("./Preface/" + fileName);
                saveFile(lock->body.imagePath, response, std::ios::binary);
                lock->saveAsJson();
                if (lock->updateUI) {
                    lock->updateUI();
                }
            }
        }
    };

    if (GlobalConfig::config.enableHttpProxy) {
        ClientSocket::asyncRequestProxy(config.proxyAddress, std::to_string(config.proxyPort), uP->host, "443", req->httpRequest(), callBack);
    } else {
        ClientSocket::asyncRequest(uP->host, "443", req->httpRequest(), callBack);
    }
}

void RssDataFunc::checkUpdate(std::shared_ptr<RssData> data) {
    time_t lastU{};
    std::tm pubDateTm;
    for (const auto& it : data->body.messages) {
        pubDateTm = parseRssPubDate(it->pubDate);
        time_t date = std::mktime(&pubDateTm);
        if (date > lastU) {
            lastU = date;
            data->body.lastUpdate = it->pubDate;
        }
    }

    std::tm lastUpdateTm = parseRssPubDate(data->body.lastUpdate);
    std::tm lastReadTm = parseRssPubDate(data->body.lastRead);

    if (std::mktime(&lastUpdateTm) > std::mktime(&lastReadTm)) {
        std::wstring titleW = utf8_to_utf16(data->body.title);
        namespace fs = std::filesystem;
        std::wstring absImagePath = fs::absolute(fs::path(utf8_to_utf16(data->body.imagePath))).wstring();

        showWinToastNotification(titleW, L"has update!", absImagePath);

        data->isRead = false;
        postTorrent(data);
    }
}

void RssDataFunc::postTorrent(std::shared_ptr<RssData> data) {
    QBittorrent& Qbit = QBittorrent::Instance();
    for (auto& msg : data->body.messages) {
        if (msg->downloaded)
            continue;
        std::weak_ptr<RssData> weakData = data;

        Qbit.postTorrent(msg->torrentUrl, data->body.savePath, [&msg, weakData]() {
            if (auto lockData = weakData.lock()) {
                msg->downloaded = true;
                lockData->saveAsJson();
            }
        });
    }
}