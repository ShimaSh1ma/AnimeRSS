#pragma once

#include <atomic>
#include <functional>
#include <queue>
#include <shared_mutex>
#include <string>

class QBittorrent {
  private:
    QBittorrent();

    std::string cookie;

    std::queue<std::tuple<std::string, std::string, std::function<void()>>> torrentQueue;

    std::shared_mutex queueMutex;
    std::atomic_bool isPosting{false};

    void post();

  public:
    inline static QBittorrent& Instance() {
        static QBittorrent instance;
        return instance;
    }

    bool login();
    void postTorrent(const std::string& torrent, const std::string& savePath, std::function<void()> successBlock);
};