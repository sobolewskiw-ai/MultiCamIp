#pragma once
#include <QObject>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QAbstractItemModel>

class MainWindow; // forward declaration - pełna definicja potrzebna tylko w .cpp


class MediaMTXManager : public QObject
{
    Q_OBJECT
public:
    explicit MediaMTXManager(QObject *parent = nullptr);
    ~MediaMTXManager() override;
    void ensureInstalled();
    void pobieramMtxmanager(const QString &url, const QString &zapisz);
    void generateConfig(QAbstractItemModel* model);
    void stopMtx();

signals:
    void urlMtxGotowe(const QVector<QString> &wynik);
    void jestUruchomiony();

private:
    void pobierzUrlMtx();
    bool rozpakuj(const QString &archiwum,const QString &katalog);
    void zapiszMtxVersion();
    QString czytajMtxVersion();
    void startMtx();
    MainWindow *mainwindow = nullptr;
    QString installDirMtx;
    QString binaryPath;
    QString versionPath;
    QString version;
    QString nazwaPliku;
    QString downloadUrl;
    QProcess *process = nullptr;
};


