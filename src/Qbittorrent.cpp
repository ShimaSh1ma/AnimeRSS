#include "Qbittorrent.h"
#include "SettingConfig.h"
#include "SocketModule/ClientSocket.h"
#include "SocketModule/HttpRequest.h"
#include "SocketModule/HttpResponseParser.h"
#include <regex>

using namespace GlobalConfig;

QBittorrent::QBittorrent() {
    login();
};

inline std::string parserCookie(const std::string& str) {
    std::regex rx(R"((SID=([^;\s]+)))");
    std::smatch match;
    if (std::regex_search(str, match, rx)) {
        return match[1].str();
    }
    return "";
};

void QBittorrent::post() {
    while (true) {
        std::shared_lock sl(queueMutex);
        if (torrentQueue.empty()) {
            break;
        }
        auto [torrent, savePath, successBlock] = torrentQueue.front();
        sl.unlock();

        socketIndex index;
        index = ClientSocket::connectToServer(config.qbittorrentIP, std::to_string(config.qbittorrentPort));
        if (index == -1) {
            continue;
        }

        HttpRequest request;
        std::string body{"urls=" + torrent + (savePath.empty() ? "" : "&savepath=" + savePath)};
        request.setUrl(config.qbittorrentIP, "/api/v2/torrents/add");
        request.setHttpMethod("POST");
        request.addHttpHead({{"Host", ""},
                             {"Accept", ""},
                             {"Accept-Charset", ""},
                             {"Accept-Language", ""},
                             {"Referer", ""},
                             {"Cookie", this->cookie},
                             {"User-Agent", ""},
                             {"Content-Type", "application/x-www-form-urlencoded"},
                             {"Content-Length", std::to_string(body.size())}});
        request.addHttpBody(body);

        if (!ClientSocket::socketSend(index, request.httpRequest())) {
            continue;
        }

        std::unique_ptr<HttpResponseParser> parser = ClientSocket::socketReceive(index);
        ClientSocket::releaseSocket(index);

        if (parser && parser->getStatusCode() == "200") {
            {
                std::unique_lock ul(queueMutex);
                torrentQueue.pop();
            }
            if (successBlock)
                successBlock();
        }
        if (parser && parser->getStatusCode() == "403") {
            login();
        }
    }
}

bool QBittorrent::login() {
    socketIndex index;
    index = ClientSocket::connectToServer(config.qbittorrentIP, std::to_string(config.qbittorrentPort));
    if (index == -1) {
        return false;
    }

    HttpRequest request;
    std::string body{"username=" + config.qbittorrentUsername + "&password=" + config.qbittorrentPassword};
    request.setUrl(config.qbittorrentIP, "/api/v2/auth/login");
    request.setHttpMethod("POST");
    request.addHttpHead({{"Host", ""},
                         {"Accept", ""},
                         {"Accept-Charset", ""},
                         {"Accept-Language", ""},
                         {"Referer", ""},
                         {"Cookie", ""},
                         {"User-Agent", ""},
                         {"Content-Type", "application/x-www-form-urlencoded"},
                         {"Content-Length", std::to_string(body.size())}});
    request.addHttpBody(body);
    if (!ClientSocket::socketSend(index, request.httpRequest())) {
        return false;
    };

    std::unique_ptr<HttpResponseParser> parser = ClientSocket::socketReceive(index);
    if (parser && parser->getStatusCode() == "200") {
        this->cookie = parserCookie(parser->getHttpHead("Set-Cookie"));
        return true;
    }
    return false;
};

void QBittorrent::postTorrent(const std::string& torrent, const std::string& savePath, std::function<void()> successBlock) {
    {
        std::unique_lock ul(queueMutex);
        torrentQueue.push({torrent, savePath, successBlock});
    }

    bool expected = false;
    if (isPosting.compare_exchange_strong(expected, true)) {
        std::async(std::launch::async, [this]() {
            this->post();
            isPosting.store(false);
        });
    }
};