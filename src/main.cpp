#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

#include "mainwindow.h"

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

  // 获取系统语言环境
  QLocale systemLocale = QLocale::system();
  // 格式如 "zh_CN", "en_US"
  QString languageCode = systemLocale.name();

  qDebug() << "System language:" << languageCode;

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
    qDebug() << "Loaded translation for:" << languageCode;
  } else {
    qDebug() << "Using default language (translation not found for"
             << languageCode << ")";
  }

  MainWindow w;
  w.show();
  return a.exec();
}
