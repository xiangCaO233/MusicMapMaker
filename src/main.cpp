
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif  //_WIN32
#include <qsurfaceformat.h>

#include <QApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QStyleFactory>
#include <QTranslator>

#include "log/colorful-log.h"
#include "ui/mainwindow.h"

bool isDarkMode() {
    // 获取应用程序的调色板
    const QPalette palette = QApplication::palette();
    // 检查窗口背景颜色的亮度
    return palette.window().color().lightness() < 128;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(65001);
    std::setlocale(LC_ALL, ".UTF-8");
#endif  //_WIN32
    QApplication a(argc, argv);
    // 设置应用图标
#ifdef _WIN32
    a.setWindowIcon(QIcon(":/icons/icon.ico"));
#else
    a.setWindowIcon(QIcon(":/icons/icon.png"));
#endif  //_WIN32
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

    // 初始化gl版本
    QSurfaceFormat format;
#ifdef __APPLE__
    // gl4.1版本
    format.setVersion(4, 1);
#else
    // gl4.1版本
    format.setVersion(4, 5);
#endif  //__APPLE__
    // gl核心模式
    format.setProfile(QSurfaceFormat::CoreProfile);
    // 禁用用V-Sync
    format.setSwapInterval(0);
    // 应用gl设置
    QSurfaceFormat::setDefaultFormat(format);

    MainWindow w;
    // 跟随系统主题
    if (isDarkMode()) {
        w.use_theme(GlobalTheme::DARK);
    } else {
        w.use_theme(GlobalTheme::LIGHT);
    }
    w.show();

    return a.exec();
}
