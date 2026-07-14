#include "mediamtxmanager.h"
#include "mainwindow.h"
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProgressDialog>
#include <archive.h>
#include <archive_entry.h>

MediaMTXManager::MediaMTXManager(QObject *parent)
    : QObject{parent}
{
    mainwindow = qobject_cast<MainWindow*>(parent);
    if (mainwindow) {
        installDirMtx = mainwindow->appHomePath + "/mediamtx";
        binaryPath = installDirMtx + "/mediamtx";
        versionPath = installDirMtx + "/versionMtx";
        qDebug() << "installDirMtx" << installDirMtx << "binaryPath" << binaryPath
                 << "versja MTX" << versionPath;
    } else {
        qWarning() << "FindNewCamera: parent nie jest MainWindow - appHomePath niedostępny";
    }
}
MediaMTXManager::~MediaMTXManager()
{
    if (process && process->state() != QProcess::NotRunning) {
        process->blockSignals(true);
        process->terminate();
        if (!process->waitForFinished(3000))
            process->kill();
    }
}

void MediaMTXManager::ensureInstalled()
{
    if (QFile::exists(binaryPath)) {
        qDebug() << "MediaMTX jest zainstalowany, Sprawdzam uruchomienie";
        pobierzUrlMtx();
//        start();
        return;
    }
    qDebug() << "MediaMTX nie jest zainstalowany, Zainstaluj go";
    QDir dir(installDirMtx);
    if(!dir.exists()){
        dir.mkdir(installDirMtx);
    }
    pobierzUrlMtx();
}

void MediaMTXManager::pobierzUrlMtx()
{

    QString os = QSysInfo::productType();
    QString arch = QSysInfo::currentCpuArchitecture();

    qDebug() << "OS:" << os;
    qDebug() << "Architektura:" << arch;

    QString suffix;

    if (os == "windows" && arch == "x86_64")
        suffix = "windows_amd64.zip";
    else if (arch == "x86_64")
        suffix = "linux_amd64.tar.gz";
    else if (arch == "aarch64" || arch == "arm64")
        suffix = "linux_arm64.tar.gz";
    else if (arch == "armv7" || arch == "armv7l")
        suffix = "linux_armv7.tar.gz";
    else if (arch == "armv6" || arch == "armv6l")
        suffix = "linux_armv6.tar.gz";
    else {
        qDebug() << "Nieobsługiwana architektura:" << arch;
        return;
    }

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    connect(manager, &QNetworkAccessManager::finished,
            this,
            [this,suffix](QNetworkReply *reply)
            {
                // Błąd połączenia
                if (reply->error() != QNetworkReply::NoError) {
                    qDebug() << "Błąd sieci:" << reply->errorString();
                    QMessageBox::information(nullptr,"INFO","Błąd sieci:" + reply->errorString());
                    reply->deleteLater();
                    return;
                }

                // Parsowanie JSON
                QJsonParseError parseError;
                QByteArray data = reply->readAll();

                QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

                if (parseError.error != QJsonParseError::NoError) {
                    qDebug() << "Błąd JSON:" << parseError.errorString();
                    QMessageBox::information(nullptr,"INFO","Błąd sieci:" + parseError.errorString());
                    reply->deleteLater();
                    return;
                }

                QJsonObject obj = doc.object();

                version = obj["tag_name"].toString();

                qDebug() << "Dostępna wersja:" << version;

                qDebug()<< "Moja wersja" << czytajMtxVersion();

                if(version == czytajMtxVersion()){
                    startMtx();
                    //mainwindow->czytajKameryDat();
                    //generateConfig(mainwindow->ItemModel);
                    return;
                }

                QJsonArray assets = obj["assets"].toArray();

                bool znaleziono = false;

                for (const QJsonValue &v : std::as_const(assets))
                {
                    QJsonObject a = v.toObject();

                    nazwaPliku = a["name"].toString();

                    if (nazwaPliku.endsWith(suffix))
                    {
                        downloadUrl = a["browser_download_url"].toString();

                        qDebug() << "Plik:" << nazwaPliku;
                        qDebug() << "URL :" << downloadUrl;
                        znaleziono = true;
                        QVector<QString> wynik;
                        wynik << version
                              << nazwaPliku
                              << downloadUrl;

                        emit urlMtxGotowe(wynik);

                        return;
                        break;
                    }
                }

                if (!znaleziono)
                    qDebug() << "Nie znaleziono pliku:" << suffix;

                reply->deleteLater();
            });
    manager->get(QNetworkRequest(
        QUrl("https://api.github.com/repos/bluenviron/mediamtx/releases/latest")));

}

bool MediaMTXManager::rozpakuj(const QString &archiwum,const QString &katalog)
{
    struct archive *a = archive_read_new();

    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    if (archive_read_open_filename(a,
                                   archiwum.toUtf8().constData(),
                                   10240) != ARCHIVE_OK)
    {
        qDebug() << archive_error_string(a);
        archive_read_free(a);
        return false;
    }

    struct archive *ext = archive_write_disk_new();

    archive_write_disk_set_options(ext,
                                   ARCHIVE_EXTRACT_TIME |
                                       ARCHIVE_EXTRACT_PERM |
                                       ARCHIVE_EXTRACT_ACL |
                                       ARCHIVE_EXTRACT_FFLAGS);

    archive_write_disk_set_standard_lookup(ext);

    struct archive_entry *entry;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        QString path =
            QDir(katalog).filePath(
                archive_entry_pathname(entry));

        archive_entry_set_pathname(entry,
                                   path.toUtf8().constData());

        if (archive_write_header(ext, entry) == ARCHIVE_OK)
        {
            const void *buff;
            size_t size;
            la_int64_t offset;

            while (archive_read_data_block(a,
                                           &buff,
                                           &size,
                                           &offset) == ARCHIVE_OK)
            {
                archive_write_data_block(ext,
                                         buff,
                                         size,
                                         offset);
            }
        }

        archive_write_finish_entry(ext);
    }

    archive_write_free(ext);
    archive_read_free(a);

    QFile::remove(archiwum);

    zapiszMtxVersion();
    startMtx();

    return true;
}

void MediaMTXManager::zapiszMtxVersion()
{
    QFile file(versionPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&file);
        out << version;
        file.close();
    }
}

QString MediaMTXManager::czytajMtxVersion()
{
    QString versionMoja;

    QFile file(versionPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        versionMoja = QTextStream(&file).readAll().trimmed();
        file.close();
    }
    return versionMoja;
}

void MediaMTXManager::startMtx()
{
    mainwindow->czytajKameryDat("http://localhost:8080/kamery.dat");
    generateConfig(mainwindow->ItemModel);

    if(process && process->state() == QProcess::Running) {
        qDebug() << "MediaMTX jest już uruchomiony.";
        emit jestUruchomiony();
        return;
    }
    process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    connect(process, &QProcess::readyRead, this, [=]() {
        //qDebug() << process->readAll();
    });
    process->setWorkingDirectory(installDirMtx);
    process->start(binaryPath);

    if(!process->waitForStarted(3000))
        qWarning() << "Błąd: MediaMTX nie uruchomiony!";
    else
        qDebug() << "MediaMTX uruchamiam.";
}

void MediaMTXManager::stopMtx()
{
    if(process && process->state() == QProcess::Running) {
        process->terminate();
        if(!process->waitForFinished(3000)) {
            process->kill();
            process->waitForFinished();
        }
        qDebug() << "MediaMTX zatrzymany.";
    }
}

void MediaMTXManager::pobieramMtxmanager(const QString &url, const QString &zapisz)
{
    qDebug()<<"ZAPISZ = "<< zapisz;
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(url)));

    QFile *file = new QFile(zapisz);

    if (!file->open(QIODevice::WriteOnly))
    {
        QMessageBox::critical(nullptr,
                              "Błąd",
                              "Nie można utworzyć pliku:\n" + zapisz);

        file->deleteLater();
        reply->abort();
        reply->deleteLater();
        manager->deleteLater();
        return;
    }

    QProgressDialog *progress =
        new QProgressDialog("Pobieranie MediaMTX...",
                            "Anuluj",
                            0,
                            100,
                            mainwindow);

    progress->setWindowTitle("Pobieranie");
    progress->setWindowModality(Qt::ApplicationModal);
    progress->setMinimumDuration(0);
    progress->setAutoClose(true);
    progress->show();

    connect(progress, &QProgressDialog::canceled,
            reply, &QNetworkReply::abort);

    connect(reply, &QNetworkReply::downloadProgress,
            this,
            [progress](qint64 received, qint64 total)
            {
                if (total <= 0)
                    return;

                progress->setValue(int(received * 100 / total));

                progress->setLabelText(
                    QString("Pobrano %1 / %2 MB")
                        .arg(received / 1024.0 / 1024.0, 0, 'f', 2)
                        .arg(total / 1024.0 / 1024.0, 0, 'f', 2));
            });

    connect(reply, &QNetworkReply::readyRead,
            this,
            [reply,file]()
            {
                file->write(reply->readAll());
            });

    connect(reply, &QNetworkReply::finished,
            this,
            [=]()
            {
                file->close();

                if (reply->error() == QNetworkReply::NoError)
                {
                    progress->setValue(100);
                    rozpakuj(zapisz,installDirMtx);
                    QMessageBox::information(nullptr,
                                             "Informacja",
                                             "Plik został pobrany.");
                }
                else
                {
                    file->remove();

                    QMessageBox::critical(nullptr,
                                          "Błąd",
                                          reply->errorString());
                }

                progress->deleteLater();
                file->deleteLater();
                reply->deleteLater();
                manager->deleteLater();
            });
}

void MediaMTXManager::generateConfig(QAbstractItemModel* model)
{
    QString configPath = installDirMtx + "/mediamtx.yml";
    QFile file(configPath);
    qDebug()<<"plik konfiguracji"<<configPath;
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);

    // --- GLOBAL SETTINGS ---;
    out << "logLevel: info\n";
    out << "api: yes\n";
    out << "apiAddress: :9997\n";
    out << "rtspAddress: :8554\n";
    out << "protocols: [tcp,udp]\n";
    out << "readTimeout: 30s\n";
    out <<  "writeTimeout: 30s\n";
    out << "readBufferCount: 2048\n";
    //out << "readBufferCount: 32\n";
    out << "\n";

    out << "authMethod: internal\n";
    out << "authInternalUsers:\n";
    out << "- user: any\n";
    out << "  pass:\n";
    out << "  ips: []\n";
    out << "  permissions:\n";
    out << "  - action: publish\n";
    //out << "  path:\n";
    out << "  - action: read\n";
    //out << "  path:\n";
    out << "  - action: playback\n";
    //out << "  path:\n";

    out << "- user: admin\n";
    out << "  pass: mocny123456\n";
    out << "  ips: []\n";
    out << "  permissions:\n";
    out << "  - action: api\n";
    out << "  - action: metrics\n";
    out << "  - action: pprof\n";
    out << "\n";

    QString recordPathMTX = mainwindow->appHomePath+"/tmp/%path";//QDir::homePath()+"/CameraDir/tmp/%path";

    out << "paths:\n";
    for (int i = 0; i < model->rowCount(); ++i)
    {
        QString name = model->index(i, 1).data().toString();
        QString url  = model->index(i, 2).data().toString();
        QDir dir(mainwindow->appHomePath+"/tmp/"+name);
        if(!dir.exists()){
            dir.mkpath(mainwindow->appHomePath+"/tmp/"+name);
        }

        qDebug()<< name<< url;
        out << "  " << name << ":\n";
        out << "    source: " << url << "\n";
        out << "    sourceProtocol: tcp\n";
        out << "    sourceOnDemand: no\n";
        out << "    sourceOnDemandCloseAfter: 10s\n";
        //stara część
        out << "    record: yes\n";
        out << "    recordPath: "+recordPathMTX+"/%Y-%m-%d_%H-%M-%S\n";
        out << "    recordFormat: fmp4\n";
        out << "    recordPartDuration: 1s\n";
        out << "    recordSegmentDuration: 60s\n";
        //out << "    recordSegmentAlignClock: yes\n";
        out << "    recordDeleteAfter: 360s \n";
        out << "\n";

        //nowa część
        // out << "    runOnReady: |-\n";
        // out << "       bash -c '\n";
        // out << "       # trap SIGTERM – zakończy ffmpeg przy zamknięciu MediaMTX\n";
        // out << "       trap \"echo SIGTERM received, killing ffmpeg; kill $FFMPEG_PID 2>/dev/null; exit 0\" SIGTERM\n";
        // out << "       # czekamy aż stream będzie gotowy\n";
        // out << "       until ffprobe -v error rtsp://127.0.0.1:8554/$MTX_PATH > /dev/null 2>&1; do\n";
        // out << "       echo \"$MTX_PATH: czekam na stream...\"\n";
        // out << "       sleep 2\n";
        // out << "       done\n";
        // out << "       echo \"$MTX_PATH: stream gotowy, start ffmpeg\"\n";
        // out << "       ffmpeg -loglevel error -rtsp_transport tcp \\\n";
        // out << "       -i rtsp://127.0.0.1:8554/$MTX_PATH \\\n";
        // out << "       -c:v copy -c:a aac -b:a 64k \\\n";
        // out << "       -f segment \\\n";
        // out << "       -segment_time 60 \\\n";
        // out << "       -segment_atclocktime 1 \\\n";
        // out << "       -reset_timestamps 1 \\\n";
        // out << "       -strftime 1 \\\n";
        // out << "       "+QDir::homePath()+"/CameraDir/tmp/$MTX_PATH/%Y-%m-%d_%H-%M-%S.ts &\n";
        // out << "       FFMPEG_PID=$!\n";
        // out << "       wait $FFMPEG_PID\n";
        // out << "       echo \"%path: ffmpeg zakończony\"\n";
        // out << "       '\n";
    }
    //dostęp do archiwum
    // out << "  ~^archive/.+/.+$:\n";
    // out << "     runOnDemand: /home/sobolewski/mediamtx/archive.sh\n";
    // out << "     runOnDemandCloseAfter: 5s\n";

    file.close();
}