#include <mainwindow.h>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif  //_WIN32
#include <qfontdatabase.h>
#include <qsurfaceformat.h>
#include <src/log/colorful-log.h>

#include <QApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(65001);
#endif  //_WIN32
    std::setlocale(LC_ALL, ".UTF-8");

    XLogger::init("MMM");

    // 自动共享gl上下文
    // QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication a(argc, argv);

    // 获取系统语言环境
    auto systemLocale = QLocale::system();

    // 格式如 "zh_CN", "en_US"
    auto languageCode = systemLocale.name();

    // 初始化 Qt 自带的标准对话框翻译
    if (QTranslator qtTranslator;
        qtTranslator.load("qt_" + languageCode,
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
            QDir(QApplication::applicationDirPath())
                .filePath("translations/MusicMapMaker_" + languageCode + ".qm");
        loaded = appTranslator.load(localPath);
    }

    if (loaded) {
        QApplication::installTranslator(&appTranslator);
        XINFO("Loaded translation for:" + languageCode.toStdString());
    } else {
        XINFO("Using default language (translation not found for" +
              languageCode.toStdString() + ")");
    }

    // 初始化gl版本
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    // gl4.1版本
    format.setVersion(4, 1);
    // gl核心模式
    format.setProfile(QSurfaceFormat::CoreProfile);
    // 禁用用V-Sync
    format.setSwapInterval(0);
    // 应用gl设置
    QSurfaceFormat::setDefaultFormat(format);

    MainWindow w;
    // 跟随系统主题
    if (QApplication::palette().window().color().lightness() < 128) {
        w.use_theme(GlobalTheme::COLIN_DARK);
    } else {
        w.use_theme(GlobalTheme::COLIN_LIGHT);
    }
    w.show();

    // TODO
    /*
     * 1: 修复windows进程残留
     * 2: 继续实现预览区和点击跳转功能
     * 3: 实现滚动吸附拍线模式切换
     * 4: 实现mmm文件格式和直接保存操作(c-s保存为mmm)
     * 5: 实现项目配置文件格式和对应序列化与反序列化
     * 6: 实现直接打开谱面/音频文件的项目引导ui和交互[依赖todo5,6的完整架构]
     * */

    return QApplication::exec();
}
