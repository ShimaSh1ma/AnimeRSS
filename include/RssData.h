#pragma once

class RssData {
  public:
    RssData() = default;
    RssData(const RssData&) = default;
    RssData(RssData&&) noexcept = default;
    RssData& operator=(const RssData&) = default;
    RssData& operator=(RssData&&) noexcept = default;
    ~RssData() = default;
};