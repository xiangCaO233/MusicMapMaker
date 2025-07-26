#include "mainwindow.h"
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif  //_WIN32
#include <qfontdatabase.h>
#include <qsurfaceformat.h>

#include <QApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(65001);
    std::setlocale(LC_ALL, ".UTF-8");
#endif  //_WIN32

    // 自动共享gl上下文
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication a(argc, argv);

    // 获取系统语言环境
    auto systemLocale = QLocale::system();

    // 格式如 "zh_CN", "en_US"
    auto languageCode = systemLocale.name();

    // 初始化gl版本
    QSurfaceFormat format;

    // gl4.1版本
    format.setVersion(4, 1);
    // gl核心模式
    format.setProfile(QSurfaceFormat::CoreProfile);
    // 禁用用V-Sync
    format.setSwapInterval(0);
    // 应用gl设置
    QSurfaceFormat::setDefaultFormat(format);

    MainWindow w;
    w.show();

    return QApplication::exec();
}
