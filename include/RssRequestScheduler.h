#pragma once
#include <deque>
#include <functional>
#include <memory>
#include <mutex>

class RssData;

class RssRequestScheduler {
  public:
    using WeakRssPtr = std::weak_ptr<RssData>;

    static RssRequestScheduler& instance();
    void addTask(WeakRssPtr data);

  private:
    RssRequestScheduler() = default;
    RssRequestScheduler(const RssRequestScheduler&) = delete;
    RssRequestScheduler& operator=(const RssRequestScheduler&) = delete;

    void tryRunNext();

    std::deque<WeakRssPtr> taskQueue;
    bool running = false;

    std::mutex mutex_;
};
