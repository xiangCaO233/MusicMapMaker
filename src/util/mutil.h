#ifndef M_UTIL_H
#define M_UTIL_H

#include <qcolor.h>
#include <qcontainerfwd.h>
#include <qmenu.h>
#include <qpushbutton.h>
#include <qsize.h>
#include <qtoolbutton.h>
#include <unicode/ucnv.h>
#include <unicode/ustring.h>

#include <QFile>
#include <QPainter>
#include <QSvgRenderer>
#include <QtSvgWidgets/QSvgWidget>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#endif

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
inline void string2u32sring(const std::string& src, std::u32string& des) {
  UErrorCode status = U_ZERO_ERROR;
  UConverter* conv = ucnv_open("UTF-8", &status);

  // 计算所需缓冲区大小
  int32_t destLength = 0;
  u_strFromUTF8(nullptr, 0, &destLength, src.c_str(), src.length(), &status);
  // 重置状态
  status = U_ZERO_ERROR;

  // 分配缓冲区并转换
  u_strFromUTF8(reinterpret_cast<UChar*>(&des[0]), destLength, nullptr,
                src.c_str(), src.length(), &status);
  ucnv_close(conv);
}
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
