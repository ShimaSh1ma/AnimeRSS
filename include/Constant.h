#pragma once

#include <fstream>
#include <string>

// 根据主屏幕dpi设置缩放倍率
void initResizeRate();

// 转换尺寸
double sizeScale(int size);

void saveFile(const std::string& filePathStr, const std::string& data, std::ios_base::openmode mode = std::ios::out);

inline const char* const _closeNormalIcon = ":/icon/close_normal";
inline const char* const _closeHoverIcon = ":/icon/close_hover";
inline const char* const _minNormalIcon = ":/icon/min_normal";
inline const char* const _minHoverIcon = ":/icon/min_hover";

inline double _cornerRadius = sizeScale(5);

inline double _borderWidth = sizeScale(6);
inline double _titleBarHeight = sizeScale(42) + 2 * _borderWidth;
inline double _iconButtonSize = _titleBarHeight;
inline double _iconSize = sizeScale(38);
inline double _tableSpacing = 2 * _borderWidth;

inline double _rssItemWidthMax = sizeScale(450);
inline double _rssItemWidthMin = sizeScale(400);
inline double _rssItemHeight = sizeScale(600);

inline double _chosenOpacity = 0.1;
inline double _unchosenOpacity = 0.8;

constexpr double _settingWidth = 3.0 / 8.0;