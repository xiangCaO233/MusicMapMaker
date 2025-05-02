#ifndef M_UTIL_H
#define M_UTIL_H

#include <qcolor.h>
#include <qcontainerfwd.h>
#include <qmenu.h>
#include <qpushbutton.h>
#include <qsize.h>
#include <qtoolbutton.h>
#include <unicode/ucnv.h>
#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include <unicode/ustring.h>

#include <QFile>
#include <QPainter>
#include <QSvgRenderer>
#include <QtSvgWidgets/QSvgWidget>
#include <memory>
#include <string>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#include "../mmm/Beat.h"
#include "../mmm/hitobject/HitObject.h"

namespace mutil {
#ifdef _WIN32
// UTF-8 → UTF-16
inline std::wstring utf8_to_utf16(const std::string& utf8) {
  if (utf8.empty()) return L"";
  int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                        (int)utf8.size(), nullptr, 0);
  std::wstring utf16(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), utf16.data(),
                      size_needed);
  return utf16;
}

// UTF-16 → UTF-32
inline std::u32string utf16_to_utf32(const std::wstring& utf16) {
  std::u32string utf32;
  for (wchar_t c : utf16) {
    utf32.push_back(static_cast<char32_t>(c));  // 简单转换（假设没有代理对）
  }
  return utf32;
}

// 组合函数：UTF-8 → UTF-32
inline std::u32string utf8_to_utf32(const std::string& utf8) {
  std::wstring utf16 = utf8_to_utf16(utf8);
  return utf16_to_utf32(utf16);
}
#endif
// 替换弃用的 wstring_convert
inline std::u32string cu32(const std::string& utf8) {
  icu_76::UnicodeString utf16 = icu_76::UnicodeString::fromUTF8(utf8);
  std::u32string utf32;
  utf32.reserve(utf16.length());

  for (int32_t i = 0; i < utf16.length();) {
    UChar32 c = utf16.char32At(i);
    utf32.push_back(c);
    i += U16_LENGTH(c);
  }
  return utf32;
}

inline bool isApproxEqual(double a, double b, double tolerance) {
  return std::abs(a - b) <= tolerance;
}
// 计算分音策略(2~64)并在找到有效策略时设置divpos
inline int calculateDivisionStrategy(
    std::multiset<std::shared_ptr<HitObject>, HitObjectComparator>& hitobjects,
    const std::shared_ptr<Beat>& beat, double tolerance) {
  auto beat_length = beat->end_timestamp - beat->start_timestamp;
  if (hitobjects.empty() || beat_length <= 0) return 1;

  // 收集当前拍内的所有物件(包括重复时间戳的) [beat_start, beat_end)
  std::vector<std::shared_ptr<HitObject>> current_beat_objects;
  for (const auto& obj : hitobjects) {
    if (obj->timestamp >= beat->start_timestamp &&
        obj->timestamp < beat->end_timestamp) {
      current_beat_objects.push_back(obj);
    }
  }
  if (current_beat_objects.empty()) return 1;

  // 从最小分音数开始检查（2到64）
  for (int n = 2; n <= 64; ++n) {
    bool valid = true;
    double sub_beat = beat_length / n;

    // 临时存储divpos值，验证通过后再设置
    std::unordered_map<std::shared_ptr<HitObject>, int32_t> divpos_map;

    for (const auto& obj : current_beat_objects) {
      double ts = obj->timestamp;
      // 计算最接近的分音点位置
      double pos = (ts - beat->start_timestamp) / sub_beat;
      int32_t rounded_pos = static_cast<int32_t>(std::round(pos));
      double closest_sub = beat->start_timestamp + rounded_pos * sub_beat;

      if (isApproxEqual(ts, closest_sub, tolerance)) {
        divpos_map[obj] = rounded_pos;
      } else {
        valid = false;
        break;
      }
    }

    if (valid) {
      // 设置所有匹配物件的divpos
      for (const auto& [obj, pos] : divpos_map) {
        obj->divpos = pos;
      }
      return n;
    }
  }

  return 1;  // 无有效分音策略
}

inline void format_music_time2u32(std::u32string& res, double srcmilliseconds) {
  // 确保毫秒数为非负数
  if (srcmilliseconds < 0) {
    srcmilliseconds = 0;
  }

  // 计算分钟、秒和毫秒
  uint32_t total_ms = static_cast<uint32_t>(std::round(srcmilliseconds));
  uint32_t minutes = total_ms / 60000;
  uint32_t remaining_ms = total_ms % 60000;
  uint32_t seconds = remaining_ms / 1000;
  uint32_t milliseconds = remaining_ms % 1000;
  // 格式化为 "mm:ss:mmm"
  std::string temp;

  // 分钟部分（两位数）
  if (minutes < 10) {
    temp += '0';
  }
  temp += std::to_string(minutes);
  temp += ':';

  // 秒部分（两位数）
  if (seconds < 10) {
    temp += '0';
  }
  temp += std::to_string(seconds);
  temp += ':';

  // 毫秒部分（三位数）
  if (milliseconds < 100) {
    temp += '0';
    if (milliseconds < 10) {
      temp += '0';
    }
  }
  temp += std::to_string(milliseconds);

  res.clear();
#ifdef _WIN32
  res = utf8_to_utf32(temp);
#else
  res = cu32(temp);
#endif  //_WIN32
}

inline void format_music_time2milliseconds(const std::string& src,
                                           double& desmilliseconds) {}

inline void get_colored_icon_pixmap(QPixmap& pixmap, const char* svgpath,
                                    QColor& color, QSize& size) {
  // 加载SVG文件
  QSvgWidget svgWidget(svgpath);
  QByteArray svgData;
  QFile file(svgpath);
  if (file.open(QIODevice::ReadOnly)) {
    svgData = file.readAll();
    file.close();
  }

  // 添加或替换fill属性
  QString svgString = QString::fromUtf8(svgData);
  if (svgString.contains("fill=\"\"")) {
    // 替换为指定颜色
    svgString.replace("fill=\"\"", "fill=\"" + color.name() + "\"");
  } else {
    // 如果没有fill属性，直接插入
    svgString.replace("<path ", "<path fill=\"" + color.name() + "\" ");
  }

  // 创建QPixmap
  QSvgRenderer renderer(svgString.toUtf8());
  pixmap = QPixmap(size);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  renderer.render(&painter);
}

inline void set_button_svgcolor(QPushButton* button, const char* svgpath,
                                QColor& color, int32_t w, int32_t h) {
  // 创建QPixmap
  QPixmap pixmap;
  QSize size(w, h);
  get_colored_icon_pixmap(pixmap, svgpath, color, size);

  // 设置图标
  button->setIcon(QIcon(pixmap));
}
inline void set_toolbutton_svgcolor(QToolButton* button, const char* svgpath,
                                    QColor& color, int32_t w, int32_t h) {
  // 创建QPixmap
  QPixmap pixmap;
  QSize size(w, h);
  get_colored_icon_pixmap(pixmap, svgpath, color, size);

  // 设置图标
  button->setIcon(QIcon(pixmap));
}
inline void set_action_svgcolor(QAction* action, const char* svgpath,
                                QColor& color) {
  // 创建QPixmap
  QPixmap pixmap;
  QSize size(28, 28);
  get_colored_icon_pixmap(pixmap, svgpath, color, size);

  // 设置图标
  action->setIcon(QIcon(pixmap));
}

inline void set_menu_svgcolor(QMenu* menu, const char* svgpath, QColor& color) {
  // 创建QPixmap
  QPixmap pixmap;
  QSize size(28, 28);
  get_colored_icon_pixmap(pixmap, svgpath, color, size);

  // 设置图标
  menu->setIcon(QIcon(pixmap));
}
};  // namespace mutil

#endif  // M_UTIL_H
