#include "mainwindow.h"

#include <QApplication>
#include <QScreen>
#include <QTranslator>
#include <QLibraryInfo>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;

    if (translator.load(
            QLocale("pl_PL"),
            "qtbase",
            "_",
            QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
    {
        a.installTranslator(&translator);
    }
    MainWindow w;
    QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();

    w.resize(screenGeometry.size()*0.6);
    //w.move(screenGeometry.width()*0.1,screenGeometry.height()*0.07);
    w.move((screenGeometry.width()-w.width())/2,0);
    w.setMinimumSize(screenGeometry.size()*0.6);
    w.show();
    return QApplication::exec();
}
