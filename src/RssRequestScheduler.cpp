#include "RssRequestScheduler.h"
#include "RssData.h"

RssRequestScheduler& RssRequestScheduler::instance() {
    static RssRequestScheduler scheduler;
    return scheduler;
}

void RssRequestScheduler::addTask(WeakRssPtr data) {
    {
        std::lock_guard lock(mutex_);
        taskQueue.push_back(std::move(data));
    }
    tryRunNext();
}

void RssRequestScheduler::tryRunNext() {
    std::lock_guard lock(mutex_);
    if (running)
        return;

    while (!taskQueue.empty()) {
        WeakRssPtr weak = taskQueue.front();
        taskQueue.pop_front();

        auto shared = weak.lock();
        if (shared) {
            shared->callScheduler = [this]() {
                {
                    std::lock_guard lock(this->mutex_);
                    running = false;
                }
                tryRunNext();
            };

            running = true;
            shared->requestRss();
            return;
        }
    }
}
