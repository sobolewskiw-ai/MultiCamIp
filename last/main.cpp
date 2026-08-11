#include "mainwindow.h"

#include <QApplication>
#include <QScreen>
#include <QTranslator>
#include <QLibraryInfo>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setStyleSheet(R"(
    QMenu {
        background: #2b2b2b;
        color: white;
        border: 1px solid #555;
    }
    QMenu::item {
        background: #9ACFF0;
        color: #000000;
        padding: 6px 25px;
    }
    QMenu::item:selected {
        background: #3d7eff;
        color: white;
    }
    QMenu::item:disabled {
        background: #9ACFF0;
        color: #777;
    }
    )");

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

    w.resize(screenGeometry.size()*0.8);
    //w.move(screenGeometry.width()*0.1,screenGeometry.height()*0.07);
    w.move((screenGeometry.width()-w.width())/2,0);
    w.setMinimumSize(screenGeometry.size()*0.6);
    w.show();
    return QApplication::exec();
}
