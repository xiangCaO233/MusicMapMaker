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
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return QApplication::exec();
}
