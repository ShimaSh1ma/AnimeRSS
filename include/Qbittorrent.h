#include <atomic>
#include <functional>
#include <queue>
#include <shared_mutex>
#include <string>

class QBittorrent {
  private:
    QBittorrent();

    std::string cookie;
    std::string host;
    std::string port;

    std::string admin;
    std::string password;

    std::queue<std::tuple<std::string, std::string, std::function<void()>>> torrentQueue;

    std::shared_mutex queueMutex;
    std::atomic_bool isPosting{false};

    void post();

  public:
    static QBittorrent& Instance() {
        static QBittorrent instance;
        return instance;
    }

    bool login();
    void postTorrent(const std::string& torrent, const std::string& savePath, std::function<void()> successBlock);
};