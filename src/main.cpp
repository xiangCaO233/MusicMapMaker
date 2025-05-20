#include <memory>

#include "mmm/hitobject/HitObject.h"
#include "mmm/hitobject/Note/Note.h"
#include "mmm/map/osu/OsuMap.h"
#include "mmm/map/rm/RMMap.h"
#include "mmm/timing/Timing.h"
#include "mmm/timing/osu/OsuTiming.h"
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
#include <iostream>

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
    XLogger::init("MMM");
    // 设置KDE风格（如果可用）
    if (QStyleFactory::keys().contains("Breeze")) {
        XINFO("使用kde Breeze主题");
        a.setStyle(QStyleFactory::create("Breeze"));
    }

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
        w.use_theme(GlobalTheme::LIGHT);
    } else {
        w.use_theme(GlobalTheme::DARK);
    }
    w.show();

    // auto map = std::make_shared<RMMap>();
    // map->load_from_file(
    //     "../resources/map/rm/4K-Lv.13-Quattro Elements Dimiourgia/Quattro "
    //     "Elements Dimiourgia_4k_hd.imd");

    // auto map = std::make_shared<OsuMap>();
    // map->load_from_file(
    //     "../resources/map/Haruka Kiritani  Shizuku Hino Mori  Hatsune Miku
    //     - " "shojo rei/Haruka Kiritani  Shizuku Hino Mori  Hatsune Miku -
    //     shojo rei
    //     "
    //     "(xiang_233) [(LN)NM lv.29].osu");

    // std::shared_ptr<Timing> speedtiming = std::make_shared<OsuTiming>();
    //  speedtiming->timestamp = 90473;
    //  speedtiming->is_base_timing = false;
    //  speedtiming->basebpm = 100;
    //  speedtiming->bpm = 3.0;

    // std::shared_ptr<Timing> speedtiming2 = std::make_shared<OsuTiming>();
    // speedtiming2->timestamp = 90623;
    // speedtiming2->is_base_timing = false;
    // speedtiming2->basebpm = 100;
    // speedtiming2->bpm = 2.0;

    // map->insert_timing(speedtiming);
    // map->insert_timing(speedtiming2);

    return a.exec();
}
