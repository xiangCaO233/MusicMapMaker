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

#include "log/colorful-log.h"
#include "ui/mainwindow.h"

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(65001);
    std::setlocale(LC_ALL, ".UTF-8");
#endif  //_WIN32
    QApplication a(argc, argv);
    // 设置应用图标
    QApplication::setWindowIcon(QIcon(":/icons/icon.png"));
    XLogger::init("MMM");

    // 获取系统语言环境
    const QLocale systemLocale = QLocale::system();
    // 格式如 "zh_CN", "en_US"
    const QString languageCode = systemLocale.name();

    XINFO("System language:" + languageCode.toStdString());

    // 设置app默认字体
    // 1. 加载字体文件
    const int fontId =
        QFontDatabase::addApplicationFont(":/font/ComicMono-Bold.ttf");
    if (fontId == -1) {
        qWarning() << "Failed to load font file";
        return -1;
    }

    // 2. 获取字体族名
    const QStringList fontFamilies =
        QFontDatabase::applicationFontFamilies(fontId);
    if (fontFamilies.empty()) {
        qWarning() << "No font families found in the font file";
        return -1;
    }

    // 3. 创建字体对象并设置为应用程序默认字体
    // 使用第一个字体族，大小12
    const QFont defaultFont(fontFamilies.at(0), 12);
    QApplication::setFont(defaultFont);

    // 初始化 Qt 自带的标准对话框翻译
    QTranslator qtTranslator;
    if (qtTranslator.load("qt_" + languageCode,
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        QApplication::installTranslator(&qtTranslator);
    }

    // 加载应用程序的自定义翻译
    QTranslator appTranslator;

    // 从资源文件加载
    bool loaded = appTranslator.load(":/translations/MusicMapMaker_" +
                                     languageCode + ".qm");

    // 如果资源加载失败，尝试从文件系统加载
    if (!loaded) {
        const QString localPath =
            QDir(qApp->applicationDirPath())
                .filePath("translations/MusicMapMaker_" + languageCode + ".qm");
        loaded = appTranslator.load(localPath);
    }

    if (loaded) {
        QApplication::installTranslator(&appTranslator);
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
    if (QApplication::palette().window().color().lightness() < 128) {
        w.use_theme(GlobalTheme::DARK);
    } else {
        w.use_theme(GlobalTheme::LIGHT);
    }
    w.show();

    return QApplication::exec();
}
