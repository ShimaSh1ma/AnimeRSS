#pragma once

#include <atomic>
#include <future>
#include <memory>
#include <string>
#include <vector>

struct RssMessage {
    std::string title;
    std::string link;
    std::string pubDate;
    std::string description;
    std::string torrentUrl;
    bool downloaded = false;
};

struct RssBody {
    std::string url;
    std::string savePath;
    std::string title;

    std::vector<std::unique_ptr<RssMessage>> messages;

    std::string originRssHtml;

    std::string imageUrl;
    std::string imagePath;

    std::string lastUpdata;
    std::string lastRead;

    std::string timeStamp;
};

class RssData : public std::enable_shared_from_this<RssData> {
  public:
    RssData() = default;
    RssData(const char* url, const char* savePath, const char* title);

    RssData(const RssData&) = default;
    RssData(RssData&&) noexcept = default;
    RssData& operator=(const RssData&) = default;
    RssData& operator=(RssData&&) noexcept = default;
    ~RssData() = default;

    std::function<void()> updateUI;
    std::function<void()> callScheduler;

    void requestRss();

    std::string getTitle();
    std::string getImage();
    std::string getUrl();
    std::string getSavePath();

    void setImage(const std::string& path);

    void setRead();

    bool getRead();

    void loadFromJson(const std::string& _json);

    void deleteData();

  private:
    std::atomic_bool isRequesting = false;
    std::atomic_bool isImageRequesting = false;
    std::atomic_bool isDeleted = false;

    bool isRead = false;

    std::string jsonPath;

    RssBody body;

    std::future<std::string> rssFuture;
    std::future<std::string> imageFuture;

    // 检查是否有更新
    void checkUpdate();
    // 保存为json
    void saveAsJson();
    // 解析rss
    void parseRss();
    // 解析封面图
    void parseImageUrl();
    // 请求封面图
    void requestImage();
    // 推送到torrent下载器
    void postTorrent();

    void setTimeStamp();
    std::string getTimeStamp();
};