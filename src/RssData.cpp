#include "RssData.h"
#include "Constant.h"
#include "Qbittorrent.h"
#include "SocketModule/ClientSocket.h"
#include "SocketModule/HttpRequest.h"
#include "SocketModule/UrlParser.h"
#include "tinyxml2.h"
#include <QElapsedTimer>
#include <QTimer>
#include <ctime>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <sstream>

using json = nlohmann::json;

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
    jsonPath = R"(.\rss\)" + body.title + ".json";
    saveAsJson();
}

std::string RssData::getTitle() {
    return body.title;
}

std::string RssData::getImage() {
    return body.imagePath;
}

std::string RssData::getUrl() {
    return body.url;
}

void RssData::requestRss() {
    if (isRequesting)
        return;
    isRequesting = true;

    auto uP = std::make_unique<UrlParser>();
    uP->parseUrl(body.url);

    auto req = std::make_unique<HttpRequest>();
    req->setUrl(*uP);
    req->addHttpHead({
        {"Connection", "close"},
    });

    rssFuture = ClientSocket::asyncRequestProxy("127.0.0.1", "7890", uP->host, "443", req->httpRequest());

    QTimer* timer = new QTimer();
    QElapsedTimer* elapsed = new QElapsedTimer();
    elapsed->start();

    timer->setInterval(500);
    connect(timer, &QTimer::timeout, this, [this, timer, elapsed]() {
        if (rssFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            timer->stop();
            timer->deleteLater();
            delete elapsed;

            body.originRssHtml = rssFuture.get();
            parseRss();
            isRequesting = false;
        } else if (elapsed->elapsed() > 1000 * 60) {
            timer->stop();
            timer->deleteLater();
            delete elapsed;
            isRequesting = false;
        }
    });

    timer->start();
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

    for (tinyxml2::XMLElement* item = root->FirstChildElement("channel")->FirstChildElement("item"); item != nullptr; item = item->NextSiblingElement("item")) {
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
    if (isImageRequesting)
        return;
    isImageRequesting = true;

    auto uP = std::make_unique<UrlParser>();
    uP->parseUrl(body.imageUrl);

    auto req = std::make_unique<HttpRequest>();
    req->setUrl(*uP);
    req->addHttpHead({{"", ""}, {"", ""}});

    imageFuture = ClientSocket::asyncRequestProxy("127.0.0.1", "7890", uP->host, "443", req->httpRequest());

    std::string fileName = uP->fileName + uP->fileExtension;

    QTimer* timer = new QTimer();
    QElapsedTimer* elapsed = new QElapsedTimer();
    elapsed->start();

    timer->setInterval(500);
    connect(timer, &QTimer::timeout, this, [this, timer, elapsed, fileName]() {
        if (imageFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            timer->stop();
            timer->deleteLater();
            delete elapsed;

            std::string image = imageFuture.get();
            if (!image.empty()) {
                body.imagePath = "./Preface/" + fileName;
                saveFile(body.imagePath, image, std::ios::binary);
                saveAsJson();
                if (updateUI) {
                    updateUI();
                }
            }
            isImageRequesting = false;
        } else if (elapsed->elapsed() > 1000 * 60) {
            timer->stop();
            timer->deleteLater();
            delete elapsed;
            isImageRequesting = false;
        }
    });
    timer->start();
}

void RssData::checkUpdate() {
    time_t lastU{};
    for (const auto& it : body.messages) {
        time_t date = std::mktime(&parseRssPubDate(it->pubDate));
        if (date > lastU) {
            body.lastUpdata = it->pubDate;
        }
    }

    if (std::mktime(&parseRssPubDate(body.lastUpdata)) > std::mktime(&parseRssPubDate(body.lastRead))) {
        isRead = false;
        postTorrent();
    }
}

void RssData::read() {
    isRead = true;
    body.lastRead = body.lastUpdata;
    saveAsJson();
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
    json j;
    j["url"] = body.url;
    j["savePath"] = body.savePath;
    j["title"] = body.title;
    j["imageUrl"] = body.imageUrl;
    j["imagePath"] = body.imagePath;
    j["lastUpdata"] = body.lastUpdata;
    j["lastRead"] = body.lastRead;
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

    jsonPath = R"(.\rss\)" + body.title + ".json";
}

void RssData::deleteData() {
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
