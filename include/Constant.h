#pragma once

#include <string>

// 根据主屏幕dpi设置缩放倍率
void initResizeRate();

// 转换尺寸
double sizeScale(int size);

inline const char* const _closeNormalIcon = ":/icon/close_normal";
inline const char* const _closeHoverIcon = ":/icon/close_hover";
inline const char* const _minNormalIcon = ":/icon/min_normal";
inline const char* const _minHoverIcon = ":/icon/min_hover";

inline double _titleBarHeight = sizeScale(60);
inline double _iconButtonSize = sizeScale(60);
inline double _iconSize = sizeScale(56);