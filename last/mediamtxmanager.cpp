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
#include <QRandomGenerator>
#include <QTextStream>
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
        adminPasswordPath = installDirMtx + "/adminPassword";
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
            [this,suffix,manager](QNetworkReply *reply)
            {
                // KRYTYCZNA... nie, tu POWAŻNA POPRAWKA (wyciek zasobów):
                // wcześniej reply->deleteLater() był wywoływany tylko na
                // części ścieżek wyjścia z tej lambdy - dwie gałęzie
                // (wersja aktualna / znaleziono pasujący plik) kończyły się
                // `return;` bez sprzątnięcia repliki. Zbieramy sprzątanie w
                // jednym miejscu (RAII-podobny wzorzec) tak, żeby każde
                // wyjście z lambdy je wykonywało. Sprzątamy też
                // jednorazowy QNetworkAccessManager (wcześniej pozostawał
                // jako "wieczne" dziecko `this` po każdym wywołaniu
                // pobierzUrlMtx()).
                auto sprzatnij = [reply, manager]() {
                    reply->deleteLater();
                    manager->deleteLater();
                };

                // Błąd połączenia
                if (reply->error() != QNetworkReply::NoError) {
                    qDebug() << "Błąd sieci:" << reply->errorString();
                    QMessageBox::information(nullptr,"INFO","Błąd sieci:" + reply->errorString());
                    sprzatnij();
                    return;
                }

                // Parsowanie JSON
                QJsonParseError parseError;
                QByteArray data = reply->readAll();

                QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

                if (parseError.error != QJsonParseError::NoError) {
                    qDebug() << "Błąd JSON:" << parseError.errorString();
                    QMessageBox::information(nullptr,"INFO","Błąd sieci:" + parseError.errorString());
                    sprzatnij();
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
                    sprzatnij();
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

                        sprzatnij();
                        return;
                    }
                }

                if (!znaleziono)
                    qDebug() << "Nie znaleziono pliku:" << suffix;

                sprzatnij();
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
    if(mainwindow->czytajKameryDat("http://localhost:8080/kamery.dat")){
    generateConfig(mainwindow->ItemModel);

    if(process && process->state() == QProcess::Running) {
        qDebug() << "MediaMTX jest już uruchomiony.";
        emit jestUruchomiony();
        return;
    }
    if (process) {
        // POWAŻNA POPRAWKA (zarządzanie zasobami): wcześniej `process` był
        // tu bezwarunkowo nadpisywany nowym QProcess, jeśli poprzedni
        // proces już się zakończył (state() != Running) - stary obiekt
        // pozostawał "osierocony" jako dziecko `this` aż do zniszczenia
        // MediaMTXManager, a stopMtx() tracił możliwość jego kontrolowania.
        process->deleteLater();
        process = nullptr;
    }
    process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);

    // POPRAWKA (diagnostyka): log MediaMTX (w tym błędy jego WEWNĘTRZNYCH
    // skryptów runOnInit - czyli ffmpeg publikującego kamerę do MTX) był
    // dotąd całkowicie wyciszony (`//qDebug() << ...`). Bez tego nie da się
    // zdiagnozować np. dlaczego dana kamera nigdy nie zaczyna publikować
    // (co objawia się jako "404 Not Found" po stronie odtwarzacza, mimo że
    // sama kamera źródłowa działa poprawnie).
    connect(process, &QProcess::readyRead, this, [=]() {
        const QByteArray data = process->readAll();
        for (const QByteArray &line : data.split('\n')) {
            if (!line.trimmed().isEmpty())
                qDebug().noquote() << "MediaMTX:" << line;
        }
    });
    process->setWorkingDirectory(installDirMtx);
    process->start(binaryPath);

    if(!process->waitForStarted(3000))
        qWarning() << "Błąd: MediaMTX nie uruchomiony!";
    else
        qDebug() << "MediaMTX uruchamiam.";
    }else{
        QMessageBox::information(nullptr,"INFO","SERWER HTTP NIE DZIAŁA");
    }
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

QString MediaMTXManager::shellEscapeSingleQuoted(const QString &input)
{
    // KRYTYCZNA POPRAWKA BEZPIECZEŃSTWA (wstrzykiwanie poleceń powłoki):
    // adres/login/hasło kamery pochodzą od użytkownika i są osadzane
    // wewnątrz bloku `bash -c '...'` generowanego dla MediaMTX. Cały ten
    // blok jest jednym apostrofowanym łańcuchem z punktu widzenia powłoki,
    // która go uruchamia - apostrof (') w adresie/haśle kamery przedwcześnie
    // kończył ten łańcuch i pozwalał wstrzyknąć dowolne polecenie systemowe
    // wykonywane przy starcie strumienia (niezależnie od otaczających go w
    // wygenerowanym tekście cudzysłowów - te są dla powłoki zwykłymi,
    // nieznaczącymi znakami dopóki jesteśmy w trybie apostrofowanym).
    //
    // Standardowy, bezpieczny sposób osadzenia dowolnego tekstu wewnątrz
    // apostrofowanego łańcucha powłoki: zamknij apostrof, wstaw escapowany
    // apostrof, otwórz apostrof z powrotem:  '  ->  '\''
    QString escaped = input;
    escaped.replace(QLatin1Char('\''), QLatin1String("'\\''"));
    return escaped;
}

QString MediaMTXManager::sanitizeYamlKey(const QString &input)
{
    // Nazwa kamery trafia jako klucz mapy YAML (ścieżka MediaMTX) oraz jako
    // nazwa katalogu nagrań. Dopuszczamy tylko bezpieczny zestaw znaków -
    // inaczej dwukropek, cudzysłów albo znak nowej linii w nazwie mogłyby
    // zniekształcić strukturę wygenerowanego mediamtx.yml albo dopisać
    // dodatkowe, nieautoryzowane klucze konfiguracyjne.
    QString safe;
    safe.reserve(input.size());
    for (const QChar &c : input) {
        if (c.isLetterOrNumber() || c == QLatin1Char('_') || c == QLatin1Char('-'))
            safe.append(c);
    }
    if (safe.isEmpty())
        safe = QStringLiteral("kamera");
    return safe;
}

QString MediaMTXManager::generateOrLoadAdminPassword()
{
    // KRYTYCZNA POPRAWKA BEZPIECZEŃSTWA: wcześniej hasło administratora API
    // MediaMTX było zaszyte na stałe w kodzie źródłowym ("mocny123456"),
    // widoczne dla każdego w publicznym repozytorium - w praktyce brak
    // jakiejkolwiek ochrony API/metrics/pprof MediaMTX. Zamiast tego
    // generujemy losowe hasło przy pierwszym uruchomieniu i zapisujemy je
    // lokalnie (poza repozytorium), z uprawnieniami tylko dla właściciela.
    QFile file(adminPasswordPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString saved = QTextStream(&file).readAll().trimmed();
        file.close();
        if (saved.size() >= 16)
            return saved;
    }

    const QString chars =
        "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789!@#%^&*";
    QString generated;
    generated.reserve(24);
    for (int i = 0; i < 24; ++i)
        generated.append(chars.at(QRandomGenerator::global()->bounded(chars.size())));

    QDir dir(installDirMtx);
    if (!dir.exists())
        dir.mkpath(installDirMtx);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream(&file) << generated;
        file.close();
        QFile::setPermissions(adminPasswordPath,
                              QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    } else {
        qWarning() << "MediaMTXManager: nie udało się zapisać hasła admina do"
                   << adminPasswordPath;
    }
    return generated;
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
    //out << "protocols: [tcp,udp]\n";  "rtspTransports: [tcp, udp]\n";
    out << "rtspTransports: [tcp, udp]\n";

    out << "readTimeout: 30s\n";
    out <<  "writeTimeout: 30s\n";
    //out << "readBufferCount: 2048\n";  "writeQueueSize: 2048\n";
    out << "writeQueueSize: 2048\n";
    //out << "readBufferCount: 32\n";
    out << "\n";

    out << "authMethod: internal\n";
    out << "authInternalUsers:\n";
    // UWAGA (bezpieczeństwo): użytkownik "any" bez hasła z pełnymi prawami
    // publish/read/playback jest tu celowy - to sposób, w jaki aplikacja
    // udostępnia strumienie RTSP innym instancjom w sieci LAN. Oznacza to
    // jednak, że KAŻDY w tej samej sieci może odczytać (a nawet nadpisać)
    // strumienie bez uwierzytelnienia. Jeśli to nieakceptowalne w Twoim
    // środowisku, należy tu wprowadzić prawdziwe konta z hasłami i
    // odpowiednio zaktualizować adresy RTSP używane przez FfmpegPlayer
    // oraz skrypty runOnInit poniżej.
    out << "- user: any\n";
    out << "  pass: \"\"\n";
    out << "  ips: []\n";
    out << "  permissions:\n";
    out << "  - action: publish\n";
    //out << "  path:\n";
    out << "  - action: read\n";
    //out << "  path:\n";
    out << "  - action: playback\n";
    //out << "  path:\n";

    // KRYTYCZNA POPRAWKA: hasło administratora API MediaMTX było wcześniej
    // zaszyte na stałe w kodzie źródłowym ("mocny123456") - patrz
    // generateOrLoadAdminPassword().
    adminPassword = generateOrLoadAdminPassword();

    out << "- user: admin\n";
    out << "  pass: " << adminPassword << "\n";
    out << "  ips: []\n";
    out << "  permissions:\n";
    out << "  - action: api\n";
    out << "  - action: metrics\n";
    out << "  - action: pprof\n";
    out << "\n";

    QString recordPathMTX = mainwindow->appHomePath+"/tmp/%path";

    out << "paths:\n";
    for (int i = 0; i < model->rowCount(); ++i)
    {
        QString rawName = model->index(i, 1).data().toString();
        // KRYTYCZNA POPRAWKA: nazwa kamery trafia jako klucz YAML i nazwa
        // katalogu - sanityzujemy ją, żeby nie dało się nią zniekształcić
        // struktury wygenerowanego pliku konfiguracyjnego.
        QString name = sanitizeYamlKey(rawName);
        if (name != rawName)
            qWarning() << "MediaMTXManager: nazwa kamery zawierała niedozwolone znaki, oczyszczono:"
                       << rawName << "->" << name;

        QString url = model->index(i, 2).data().toString();
        // KRYTYCZNA POPRAWKA (wstrzykiwanie poleceń powłoki): adres URL
        // (może zawierać login/hasło kamery wpisane przez użytkownika)
        // musi być bezpiecznie osadzony w apostrofowanym skrypcie bash -
        // patrz shellEscapeSingleQuoted().
        QString safeUrl = shellEscapeSingleQuoted(url);

        // Tworzymy katalog dla KAŻDEJ kamery przed warunkami
        QDir dir(mainwindow->appHomePath + "/tmp/" + name);
        if(!dir.exists()){
            dir.mkpath(mainwindow->appHomePath + "/tmp/" + name); // `name` już oczyszczone
        }

        if(url.startsWith("rtsp://", Qt::CaseInsensitive)){
            qDebug() << "KAMERA RTSP -> SYNCHRONIZACJA CZASU:" << name;

            out << "  " << name << ":\n";
            out << "    source: publisher\n";
            out << "    runOnInit: |\n";
            out << "      bash -c '\n";
            out << "      trap \"kill $FFMPEG_PID 2>/dev/null; exit 0\" SIGTERM\n";

            // Czekaj na sekundę :00 zegara systemowego komputera
            //out << "      while [ $(date +%S) -ne 0 ]; do sleep 0.2; done\n";

            // URUCHOMIENIE FFMPEG Z JAWNYM WYŁĄCZENIEM BUFORÓW NA WEJŚCIU (PRZED -i)
            out << "      ffmpeg -hide_banner -loglevel error -rtsp_transport tcp \\\n";
            out << "      -fflags nobuffer+genpts -flags low_delay -async 1 \\\n"; // <-- DODAJ TĘ LINIĘ
            out << "      -i \"" << safeUrl << "\" \\\n";

            // PROCESOWANIE ZEROLATENCY NA WYJŚCIU
            out << "      -c:v h264 -preset ultrafast -tune zerolatency -g 25 -r 25 \\\n";
            out << "      -c:a copy -f rtsp -rtsp_transport tcp -pkt_size 1400 rtsp://127.0.0.1:8554/$MTX_PATH &\n";

            out << "      FFMPEG_PID=$!\n";
            out << "      wait $FFMPEG_PID\n";
            out << "      '\n";

            out << "    record: yes\n";
            out << "    recordPath: " + recordPathMTX + "/%Y-%m-%d_%H-%M-%S\n";
            out << "    recordFormat: fmp4\n";
            out << "    recordPartDuration: 100ms\n";
            out << "    recordSegmentDuration: 60s\n";
            out << "    recordDeleteAfter: 360s \n";
            out << "\n";
        }
        else if (url.startsWith("http://", Qt::CaseInsensitive)) {
            qDebug() << "KAMERA HTTP -> SYNCHRONIZACJA CZASU:" << name;

            out << "  " << name << ":\n";
            out << "    source: publisher\n";
            out << "    runOnInit: |\n";
            out << "      bash -c '\n";
            out << "      trap \"kill $FFMPEG_PID 2>/dev/null; exit 0\" SIGTERM\n";

            // Czekaj na sekundę :00 zegara systemowego komputera
            //out << "      while [ $(date +%S) -ne 0 ]; do sleep 0.2; done\n";

            out << "      ffmpeg -hide_banner -loglevel error \\\n";
            out << "      -fflags +genpts+nobuffer \\\n";
            out << "      -use_wallclock_as_timestamps 1 \\\n";
            out << "      -i \"" << safeUrl << "\" \\\n";
            out << "      -c:v copy -c:a aac -b:a 64k -fflags +genpts+flush_packets \\\n";
            out << "      -f rtsp -rtsp_transport tcp -pkt_size 1400 rtsp://127.0.0.1:8554/$MTX_PATH &\n";
            out << "      FFMPEG_PID=$!\n";
            out << "      wait $FFMPEG_PID\n";
            out << "      '\n";

            out << "    record: yes\n";
            out << "    recordPath: " << recordPathMTX << "/%Y-%m-%d_%H-%M-%S\n";
            out << "    recordFormat: fmp4\n";
            out << "    recordPartDuration: 100ms\n";
            out << "    recordSegmentDuration: 60s\n";
            out << "    recordDeleteAfter: 360s\n";
            out << "\n";
        }
    }

}
