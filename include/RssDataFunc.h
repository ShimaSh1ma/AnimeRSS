#pragma once
#include <memory>
#include <string>

class RssData;

class RssDataFunc {
  public:
    // 异步请求rss
    static void requestRss(std::shared_ptr<RssData> data);

  private:
    RssDataFunc() = delete;
    ~RssDataFunc() = delete;

    // 检查是否有更新
    static void checkUpdate(std::shared_ptr<RssData> data);
    // 解析rss
    static void parseRss(std::shared_ptr<RssData> data);
    // 解析封面图
    static void parseImageUrl(std::shared_ptr<RssData> data);
    // 请求封面图
    static void requestImage(std::shared_ptr<RssData> data);
    // 推送到torrent下载器
    static void postTorrent(std::shared_ptr<RssData> data);
};