#include <mainwindow.h>
#ifdef _WIN32
#define NOMINMAX
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <windows.h>

#include <iostream>

#endif  //_WIN32
#include <colorful-log.h>
#include <qfontdatabase.h>
#include <qsurfaceformat.h>

#include <QApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

void attachConsole() {
#if defined _WIN32
    // 1. 分配一个新的控制台
    if (AllocConsole()) {
        // 2. 设置控制台输出编码为 UTF-8
        //    这是解决中文乱码的关键
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        // 3. 启用虚拟终端处理，以支持ANSI颜色代码
        //    这是让qDebug的颜色生效的关键
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD dwMode = 0;
            if (GetConsoleMode(hOut, &dwMode)) {
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            }
        }

        // 4. 将标准输入/输出/错误流重定向到新的控制台
        FILE *fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$", "r", stdin);

        // 5. （可选）如果你也想使用 iostream
        std::cout.clear();
        std::cerr.clear();
        std::cin.clear();

        std::wcout.clear();
        std::wcerr.clear();
        std::wcin.clear();
    }
#endif  //_WIN32
}

int main(int argc, char *argv[]) {
#ifdef _WIN32
    // attachConsole();
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
    XINFO("System theme lightness:" +
          std::to_string(QApplication::palette().window().color().lightness()));
    if (QApplication::palette().window().color().lightness() < 128) {
        w.use_theme(GlobalTheme::COLIN_DARK);
    } else {
        w.use_theme(GlobalTheme::COLIN_LIGHT);
    }
    w.show();

    // TODO
    /*
     // * 1: win退不干净的bug
     // * 2: timing编辑过程的段错误
     // * 3: 小轨道布局丢失特效渲染(InMaintrack组件丢失)
     // * 4: 0x倍速的错误时间线映射
     *
     * 6: 实现直接打开谱面/音频文件的项目引导ui和交互
     */
    // 执行Qt事件循环，并将退出码保存起来
    int exitCode = a.exec();

    // 只在Windows，程序退出前执行以下暂停操作
#if defined _WIN32
    // if (exitCode == 0) {
    //     XINFO("程序正常退出.");
    // } else {
    //     XERROR("程序非正常退出,错误码: " + std::to_string(exitCode));
    // }
    // std::cout << "按任意键关闭终端..." << std::endl;

    // // 等待用户按回车键
    // getchar();
#endif  //_WIN32

    return exitCode;
}
