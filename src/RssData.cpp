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

void RssData::requestRss() {
    bool expected = false;
    if (!isRequesting.compare_exchange_strong(expected, true)) {
        return;
    }
    auto self = shared_from_this();

    auto uP = std::make_unique<UrlParser>();
    uP->parseUrl(body.url);

    auto req = std::make_unique<HttpRequest>();
    req->setUrl(*uP);
    req->addHttpHead({{"Connection", "close"}});

    // 异步请求，返回future
    if (GlobalConfig::config.enableHttpProxy) {
        rssFuture = ClientSocket::asyncRequestProxy(config.proxyAddress, std::to_string(config.proxyPort), uP->host, "443", req->httpRequest());
    } else {
        rssFuture = ClientSocket::asyncRequest(uP->host, "443", req->httpRequest());
    }

    std::thread([self]() {
        const int timeoutMs = 30000;
        auto start = std::chrono::steady_clock::now();

        while (true) {
            auto status = self->rssFuture.wait_for(std::chrono::milliseconds(100));
            if (status == std::future_status::ready) {
                try {
                    self->body.originRssHtml = self->rssFuture.get();
                } catch (...) {
                    self->body.originRssHtml.clear();
                }

                self->isRequesting = false;

                if (!self->body.originRssHtml.empty()) {
                    self->parseRss();
                }

                if (self->callScheduler)
                    self->callScheduler();
                break;
            }
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > timeoutMs) {
                self->isRequesting = false;
                if (self->callScheduler)
                    self->callScheduler();
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }).detach();
}

void RssData::parseRss() {
    tinyxml2::XMLDocument doc;
    if (doc.Parse(body.originRssHtml.c_str()) != tinyxml2::XML_SUCCESS) {
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

        if (std::any_of(body.messages.begin(), body.messages.end(), [&](const std::unique_ptr<RssMessage>& body) { return body->torrentUrl == torrentUrl; })) {
            continue;
        }

        body.messages.emplace_back(std::make_unique<RssMessage>(RssMessage{
            title ? title : "",
            link ? link : "",
            pubDate ? pubDate : "",
            description ? description : "",
            torrentUrl ? torrentUrl : "",
        }));
    }
    checkUpdate();
    saveAsJson();

    parseImageUrl();
}

void RssData::parseImageUrl() {
    if (!body.imageUrl.empty()) {
        if (!(body.imagePath.empty() && std::filesystem::exists(body.imagePath))) {
            requestImage();
        }
        return;
    }
    if (body.messages.empty()) {
        body.imageUrl = "";
        return;
    }
    std::regex imgRegex(R"(<img\s+[^>]*src=["']([^"']+)["'])");
    std::cmatch match;
    for (auto& it : body.messages) {
        if (std::regex_search(it.get()->description.c_str(), match, imgRegex)) {
            body.imageUrl = match[1].str();
            requestImage();
            return;
        }
    }
}

void RssData::requestImage() {
    bool expected = false;
    if (!isImageRequesting.compare_exchange_strong(expected, true)) {
        return; // 已经在请求图片，直接返回
    }
    auto self = shared_from_this();

    auto uP = std::make_unique<UrlParser>();
    uP->parseUrl(body.imageUrl);

    auto req = std::make_unique<HttpRequest>();
    req->setUrl(*uP);
    req->addHttpHead({{"connection", "close"}, {"", ""}});

    if (GlobalConfig::config.enableHttpProxy) {
        imageFuture = ClientSocket::asyncRequestProxy(config.proxyAddress, std::to_string(config.proxyPort), uP->host, "443", req->httpRequest());
    } else {
        imageFuture = ClientSocket::asyncRequest(uP->host, "443", req->httpRequest());
    }

    std::thread([self, uP = std::move(uP)]() mutable {
        const int timeoutMs = 30000;
        auto start = std::chrono::steady_clock::now();

        std::string fileName = uP->fileExtension.empty() ? "" : uP->fileName + self->body.timeStamp + uP->fileExtension;

        while (true) {
            auto status = self->imageFuture.wait_for(std::chrono::milliseconds(100));
            if (status == std::future_status::ready) {
                try {
                    std::string image = self->imageFuture.get();
                    if (!image.empty()) {
                        self->body.imagePath = fileName.empty() ? self->getImage() : ("./Preface/" + fileName);
                        saveFile(self->body.imagePath, image, std::ios::binary);
                        self->saveAsJson();
                        if (self->updateUI) {
                            self->updateUI();
                        }
                    }
                } catch (...) {
                }

                self->isImageRequesting = false;
                break;
            }

            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > timeoutMs) {
                self->isImageRequesting = false;
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }).detach();
}

void RssData::checkUpdate() {
    time_t lastU{};
    for (const auto& it : body.messages) {
        time_t date = std::mktime(&parseRssPubDate(it->pubDate));
        if (date > lastU) {
            body.lastUpdata = it->pubDate;
        }
    }

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    if (std::mktime(&parseRssPubDate(body.lastUpdata)) > std::mktime(&parseRssPubDate(body.lastRead))) {
        std::wstring titleW = converter.from_bytes(body.title);
        namespace fs = std::filesystem;
        std::wstring absImagePath = fs::absolute(fs::path(converter.from_bytes(body.imagePath))).wstring();

        showWinToastNotification(titleW, L"has update!", absImagePath);

        isRead = false;
        postTorrent();
    }
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
    if (!std::filesystem::exists(path))
        return;
    std::string targetPath = this->getImage() + std::filesystem::path(path).extension().string();
    if (std::filesystem::copy_file(path, targetPath, std::filesystem::copy_options::overwrite_existing)) {
        body.imagePath = targetPath;
        saveAsJson();
    }
}

void RssData::setRead() {
    isRead = true;
    body.lastRead = body.lastUpdata;
    saveAsJson();
}

bool RssData::getRead() {
    return this->isRead;
}

void RssData::postTorrent() {
    QBittorrent& Qbit = QBittorrent::Instance();
    for (auto& msg : body.messages) {
        if (msg->downloaded)
            continue;
        Qbit.postTorrent(msg->torrentUrl, body.savePath, [&msg, this]() {
            msg->downloaded = true;
            saveAsJson();
        });
    }
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
    j["lastUpdata"] = body.lastUpdata;
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
    body.lastUpdata = j.value("lastUpdata", "");
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
    if (std::filesystem::exists(jsonPath)) {
        try {
            std::filesystem::remove(jsonPath);
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << e.what();
        }
    }

    if (!body.imagePath.empty() && std::filesystem::exists(body.imagePath)) {
        try {
            std::filesystem::remove(body.imagePath);
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << e.what();
        }
    }
}
