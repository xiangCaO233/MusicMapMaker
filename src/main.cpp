#ifdef _WIN32
#include <windows.h>
#endif  //_WIN32
#include <qsurfaceformat.h>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

#include "log/colorful-log.h"
#include "ui/mainwindow.h"

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  XLogger::init("MMM");

  // 获取系统语言环境
  QLocale systemLocale = QLocale::system();
  // 格式如 "zh_CN", "en_US"
  QString languageCode = systemLocale.name();

  XINFO("System language:" + languageCode.toStdString());

  // 初始化 Qt 自带的标准对话框翻译
  QTranslator qtTranslator;
  if (qtTranslator.load("qt_" + languageCode,
                        QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
    a.installTranslator(&qtTranslator);
  }

  // 加载应用程序的自定义翻译
  QTranslator appTranslator;

  // 从资源文件加载
  bool loaded = appTranslator.load(":/translations/MusicMapMaker_" +
                                   languageCode + ".qm");

  // 如果资源加载失败，尝试从文件系统加载
  if (!loaded) {
    QString localPath =
        QDir(qApp->applicationDirPath())
            .filePath("translations/MusicMapMaker_" + languageCode + ".qm");
    loaded = appTranslator.load(localPath);
  }

  if (loaded) {
    a.installTranslator(&appTranslator);
    XINFO("Loaded translation for:" + languageCode.toStdString());
  } else {
    XWARN("Using default language (translation not found for" +
          languageCode.toStdString() + ")");
  }

#ifdef __APPLE__
  // 初始化gl版本
  QSurfaceFormat format;
  // gl4.1版本
  format.setVersion(4, 1);
  // gl核心模式
  format.setProfile(QSurfaceFormat::CoreProfile);
  // 应用gl设置
  QSurfaceFormat::setDefaultFormat(format);
#else
  // 初始化gl版本
  QSurfaceFormat format;
  // gl4.1版本
  format.setVersion(4, 5);
  // gl核心模式
  format.setProfile(QSurfaceFormat::CoreProfile);
  // 应用gl设置
  QSurfaceFormat::setDefaultFormat(format);
#endif  //__APPLE__

  MainWindow w;
  w.show();

  return a.exec();
}
