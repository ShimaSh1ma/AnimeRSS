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

    std::string lastUpdate;
    std::string lastRead;

    std::string timeStamp;
};

class RssData {
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

    std::string getTitle();
    std::string getImage();
    std::string getUrl();
    std::string getSavePath();

    void setRead();
    bool getRead();

    void setImage(const std::string& path);

    void saveAsJson();
    void loadFromJson(const std::string& _json);
    void deleteData();

  private:
    std::string jsonPath;
    RssBody body;

    std::atomic_bool isRequesting = false;
    std::atomic_bool isImageRequesting = false;
    std::atomic_bool isDeleted = false;

    bool isRead = false;

    void setTimeStamp();
    std::string getTimeStamp();

    friend class RssDataFunc;
};