#include "ffmpegplayer.h"

#include <QDebug>
#include <QPixmap>
#include <QFile>
#include <QProcess>
#include <QThread>
#include <QImage>
#include <QLabel>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QThreadPool>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cerrno>

// ---------------------------------------------------------------------------
// FfmpegReaderThread
// ---------------------------------------------------------------------------

FfmpegReaderThread::FfmpegReaderThread(const QString &path, QSize size, QObject *parent)
    : QThread(parent), pipePath(path), frameSize(size)
{
}

void FfmpegReaderThread::stop()
{
    running = false;

    // POPRAWKA (błędna semantyka FIFO w poprzedniej wersji): otwarcie FIFO
    // do ODCZYTU z O_NONBLOCK w Linuksie/POSIX UDAJE SIĘ NATYCHMIAST,
    // niezależnie od tego, czy jakikolwiek writer jest podłączony - flaga
    // O_NONBLOCK po stronie READERA wpływa tylko na SAM OPEN, a nie na to,
    // czy writer istnieje. Gorzej: jeśli w danym momencie nie ma jeszcze
    // ŻADNEGO writera, kolejne read() (nawet w trybie blokującym!) zwraca
    // NATYCHMIASTOWE EOF (0 bajtów) zamiast czekać na dane - z punktu
    // widzenia jądra "0 writerów" jest nierozróżnialne od "wszyscy writerzy
    // już się rozłączyli". To właśnie powodowało, że FfmpegReaderThread
    // kończył się od razu, ZANIM ffmpeg zdążył otworzyć FIFO do zapisu.
    //
    // Poprawne rozwiązanie: run() poniżej wraca do PROSTEGO, BLOKUJĄCEGO
    // open(QIODevice::ReadOnly) - to jest właściwa, standardowa
    // synchronizacja FIFO (czeka aż prawdziwy writer, czyli ffmpeg, się
    // podłączy). Problem "wątek blokuje się na zawsze, jeśli ffmpeg nigdy
    // się nie podłączy" rozwiązujemy tutaj, w stop(): otwieramy FIFO na
    // chwilę jako ATRAPOWY writer (O_WRONLY|O_NONBLOCK) i od razu
    // zamykamy. To "budzi" ewentualne zablokowane open() po stronie
    // czytelnika (spełnia warunek "pojawił się writer"), a run() zaraz
    // potem sprawdza flagę `running` i grzecznie kończy wątek - bez
    // ryzyka błędnego natychmiastowego EOF, bo do tego momentu żaden
    // realny odczyt jeszcze się nie odbywał.
    int fd = ::open(pipePath.toLocal8Bit().constData(), O_WRONLY | O_NONBLOCK);
    if (fd >= 0)
        ::close(fd);
    // fd < 0 (zwykle ENXIO) oznacza, że jakiś writer - najpewniej sam
    // ffmpeg - już jest podłączony, więc czytelnik i tak nie jest
    // zablokowany w open().
}

void FfmpegReaderThread::run()
{
    const qint64 bytesPerFrame = (qint64)frameSize.width() * frameSize.height() * 3;
    if (bytesPerFrame <= 0) {
        qWarning() << "FfmpegReaderThread: nieprawidłowy rozmiar ramki" << frameSize;
        return;
    }

    // Blokujące open() - poprawna synchronizacja z writerem (ffmpeg).
    // Patrz obszerny komentarz w FfmpegReaderThread::stop() wyjaśniający,
    // dlaczego wersja z O_NONBLOCK po stronie czytelnika była błędna.
    QFile fifo(pipePath);
    if (!fifo.open(QIODevice::ReadOnly)) {
        qWarning() << "FfmpegReaderThread: nie udało się otworzyć FIFO:" << pipePath;
        return;
    }
    if (!running) {
        // stop() zdążyło zadziałać (i "obudzić" open() atrapowym writerem)
        // zanim zaczęliśmy realnie czytać - normalne, czyste zakończenie.
        fifo.close();
        qDebug() << "FfmpegReaderThread: przerwano przed odczytem (stop())";
        return;
    }

    QByteArray frameBuffer(bytesPerFrame, Qt::Uninitialized);

    while (running) {
        qint64 bytesRead = 0;
        while (running && bytesRead < bytesPerFrame) {
            qint64 n = fifo.read(frameBuffer.data() + bytesRead,
                                 bytesPerFrame - bytesRead);
            if (n < 0) goto done;
            if (n > 0) {
                bytesRead += n;
            } else {
                // n == 0: EOF - proces FFmpeg zamknął swój koniec FIFO
                // (padł/zakończył się). BEZ TEGO WARUNKU pętla kręciłaby
                // się w nieskończoność zjadając cały rdzeń CPU, bo kolejne
                // odczyty po EOF też natychmiast zwracają 0 - a wątek
                // dowiaduje się o potrzebie zatrzymania (running=false)
                // dopiero asynchronicznie z wątku GUI po onVideoProcessFinished().
                goto done;
            }
        }
        if (bytesRead < bytesPerFrame) break;

        QImage img(
            reinterpret_cast<const uchar*>(frameBuffer.constData()),
            frameSize.width(), frameSize.height(),
            frameSize.width() * 3,
            QImage::Format_RGB888
        );
        emit frameReady(img.copy());
    }
    done:
    fifo.close();
    qDebug() << "FfmpegReaderThread: wątek zakończony";
}

// ---------------------------------------------------------------------------
// FfmpegPlayer
// ---------------------------------------------------------------------------

FfmpegPlayer::FfmpegPlayer(QObject *parent)
    : QObject(parent)
{
}

FfmpegPlayer::~FfmpegPlayer()
{
    stop();
}

void FfmpegPlayer::setLabel(QLabel *label) { targetLabel = label; }
void FfmpegPlayer::setUrl(const QString &url) { rtspUrl = url; }
bool FfmpegPlayer::isPlaying() const { return playing; }

void FfmpegPlayer::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    aspectRatioMode = mode;
}

void FfmpegPlayer::setAudioEnabled(bool enabled)
{
    audioEnabled = enabled;
    if (playing) {
        if (audioProcess) {
            audioProcess->blockSignals(true);
            audioProcess->terminate();
            audioProcess->waitForFinished(2000);
            audioProcess->deleteLater();
            audioProcess = nullptr;
        }
        if (audioEnabled){
            startAudioProcess();
            qDebug()<< "AUDIO START";
        }
    }
}

void FfmpegPlayer::play()
{
    if (playing) return;
    if (rtspUrl.isEmpty())  { emit error("Brak URL strumienia"); return; }
    if (!targetLabel)       { emit error("Brak docelowego QLabel"); return; }

    // Resetujemy liczniki reconnect przy nowym play()
    reconnectAttempt = 0;

    // Kopiujemy URL na wątku GUI, ZANIM wyślemy zadanie do QThreadPool.
    // KRYTYCZNA POPRAWKA (use-after-free): poniższa lambda działa na wątku
    // tła i NIE WOLNO jej odwoływać się do `this` (poza przekazaniem go
    // jako "context" do QMetaObject::invokeMethod poniżej) - gdyby
    // FfmpegPlayer został usunięty, zanim to zadanie w tle się zakończy
    // (np. użytkownik szybko usuwa kamerę), odczyt this->rtspUrl albo
    // wywołanie metody składowej byłoby użyciem zwolnionej pamięci.
    // QMetaObject::invokeMethod(this, functor, Qt::QueuedConnection) jest
    // bezpieczne nawet po zniszczeniu `this` - Qt śledzi cykl życia obiektu
    // kontekstowego i po prostu nie wywoła funktora, jeśli obiekt already
    // nie istnieje.
    const QString urlForProbe = rtspUrl;

    QThreadPool::globalInstance()->start([this, urlForProbe]() {
        QSize size = probeFrameSize(urlForProbe);
        if (!size.isValid() || size.isEmpty()) {
            size = QSize(640, 480);
            qWarning() << "FfmpegPlayer: nie udało się wykryć rozdzielczości, używam 640x480";
        }
        QMetaObject::invokeMethod(this, [this, size]() {
            if (playing) return;
            frameSize = size;
            qDebug() << "FfmpegPlayer: rozdzielczość strumienia:" << frameSize;

            pipePath = QString("/tmp/ffplayer_%1_%2")
                           .arg(QCoreApplication::applicationPid())
                           .arg(reinterpret_cast<qintptr>(this));
            QFile::remove(pipePath);
            if (mkfifo(pipePath.toLocal8Bit().constData(), 0600) != 0) {
                emit error(QString("Nie udało się utworzyć FIFO: %1").arg(pipePath));
                return;
            }

            startVideoProcess();
            if (audioEnabled)
                startAudioProcess();
        }, Qt::QueuedConnection);
    });
}

void FfmpegPlayer::stop()
{
    // POWAŻNA POPRAWKA (możliwy crash przy Stop): wcześniejszy warunek
    // `if (!playing && !reconnectTimer) return;` mógł w rzadkim oknie
    // czasowym (np. tuż po onVideoProcessFinished(), zanim stan się w pełni
    // ustabilizował) pominąć wywołanie stopAll() mimo że readerThread lub
    // videoProcess wciąż istniały i działały. Skutek: przy zniszczeniu
    // `this` (np. usunięcie kamery) Qt kaskadowo usuwał wciąż działający,
    // wciąż będący dzieckiem `this` obiekt QThread - stąd "QThread:
    // Destroyed while thread is still running" i crash. Sprawdzamy teraz
    // też stan realnych zasobów, nie tylko flag.
    if (!playing && !reconnectTimer && !readerThread && !videoProcess) return;

    playing = false;

    // Zatrzymujemy timer reconnect jeśli działał
    if (reconnectTimer) {
        reconnectTimer->stop();
        reconnectTimer->deleteLater();
        reconnectTimer = nullptr;
    }

    stopAll();

    // Czyścimy label z komunikatu reconnect
    if (targetLabel) {
        targetLabel->clear();
        targetLabel->setText("BRAK OBRAZU");
    }

    emit playbackStopped();
}

// ---------------------------------------------------------------------------
// Obsługa zakończenia procesu / reconnect
// ---------------------------------------------------------------------------

void FfmpegPlayer::onVideoProcessError(QProcess::ProcessError /*err*/)
{
    if (!playing) return;
    QString msg = videoProcess ? videoProcess->errorString() : "Nieznany błąd";
    qWarning() << "FfmpegPlayer: błąd procesu video:" << msg;
    // Nie emitujemy error() - traktujemy to jak zerwanie sieci i próbujemy reconnect
    startReconnect();
}

void FfmpegPlayer::onVideoProcessFinished(int exitCode, QProcess::ExitStatus /*status*/)
{
    if (!playing) return;
    qDebug() << "FfmpegPlayer: proces video zakończony, kod:" << exitCode;
    if (videoProcess) {
        QByteArray errOut = videoProcess->readAllStandardError();
        if (!errOut.isEmpty())
            qDebug() << "FFmpeg stderr:" << errOut.left(300);
    }
    // Nieoczekiwane zakończenie - próbujemy reconnect
    startReconnect();
}

void FfmpegPlayer::startReconnect()
{
    // Zatrzymujemy stare procesy i wątek (bez ustawiania playing=false).
    // POWAŻNA POPRAWKA: stopAll(false) - NIE czyścimy pipePath, bo za
    // chwilę attemptReconnect() będzie chciało go ponownie użyć (patrz
    // komentarz przy deklaracji stopAll() w nagłówku).
    stopAll(false);

    if (!playing) return; // stop() już wywołany przez użytkownika

    reconnectAttempt++;
    reconnectCountdown = RECONNECT_INTERVAL;

    qDebug() << "FfmpegPlayer: strumień zerwany, próba reconnect za"
             << reconnectCountdown << "s (próba" << reconnectAttempt << ")";

    showReconnectLabel(reconnectCountdown);

    // Timer co 1s - odlicza i co RECONNECT_INTERVAL próbuje połączyć
    if (!reconnectTimer) {
        reconnectTimer = new QTimer(this);
        reconnectTimer->setInterval(1000);
        connect(reconnectTimer, &QTimer::timeout,
                this, &FfmpegPlayer::onReconnectTick);
    }
    reconnectTimer->start();
}

void FfmpegPlayer::onReconnectTick()
{
    reconnectCountdown--;

    if (reconnectCountdown > 0) {
        // Odliczamy - aktualizujemy label
        showReconnectLabel(reconnectCountdown);
        return;
    }

    // Czas na próbę połączenia
    reconnectCountdown = RECONNECT_INTERVAL;
    attemptReconnect();
}

void FfmpegPlayer::attemptReconnect()
{
    qDebug() << "FfmpegPlayer: próba reconnect" << reconnectAttempt << "->" << rtspUrl;

    // KRYTYCZNA POPRAWKA (use-after-free / wyścig danych): kopiujemy URL na
    // wątku GUI - poniższa lambda działa na wątku tła (QThreadPool) i nie
    // może bezpiecznie czytać this->rtspUrl (mogłoby się zmienić przez
    // setUrl() w międzyczasie, albo `this` mogłoby zostać już usunięte).
    const QString urlForCheck = rtspUrl;

    // Szybkie sprawdzenie czy host jest dostępny (TCP na port RTSP)
    // Robimy to w tle żeby nie blokować GUI
    QThreadPool::globalInstance()->start([this, urlForCheck]() {
        // Wyciągamy host i port z URL RTSP
        QUrl url(urlForCheck);
        QString host = url.host();
        int port = url.port(8554);

        QTcpSocket socket;
        socket.connectToHost(host, static_cast<quint16>(port));
        bool dostepny = socket.waitForConnected(3000);
        socket.disconnectFromHost();

        QMetaObject::invokeMethod(this, [this, dostepny]() {
            if (!playing) return; // stop() wywołany w międzyczasie

            if (!dostepny) {
                qDebug() << "FfmpegPlayer: host niedostępny, czekam...";
                showReconnectLabel(reconnectCountdown);
                return;
            }

            // Host dostępny - próbujemy uruchomić FFmpeg
            qDebug() << "FfmpegPlayer: host dostępny, restartuję FFmpeg";

            // KRYTYCZNA POPRAWKA (możliwy null-deref): jeśli w międzyczasie
            // stop()+play() zresetowały cykl zanim to opóźnione zadanie w
            // tle wróciło, reconnectTimer może jeszcze nie istnieć.
            if (reconnectTimer)
                reconnectTimer->stop();

            // Tworzymy nowe FIFO
            QFile::remove(pipePath);
            if (mkfifo(pipePath.toLocal8Bit().constData(), 0600) != 0) {
                // FIFO failed - tworzymy z nową nazwą
                pipePath = QString("/tmp/ffplayer_%1_%2_r%3")
                               .arg(QCoreApplication::applicationPid())
                               .arg(reinterpret_cast<qintptr>(this))
                               .arg(reconnectAttempt);
                if (mkfifo(pipePath.toLocal8Bit().constData(), 0600) != 0) {
                    qWarning() << "FfmpegPlayer: nie udało się utworzyć FIFO przy reconnect";
                    if (reconnectTimer)
                        reconnectTimer->start(); // próbujemy dalej
                    return;
                }
            }

            startVideoProcess();
            if (audioEnabled)
                startAudioProcess();
            // Uwaga: reconnectAttempt NIE jest już zerowane tutaj - patrz
            // komentarz w onFrameReady() (zerujemy dopiero przy
            // potwierdzonym sukcesie, czyli odebraniu pierwszej klatki).
        }, Qt::QueuedConnection);
    });
}

void FfmpegPlayer::showReconnectLabel(int secondsLeft)
{
    if (!targetLabel) return;
    targetLabel->clear();
    targetLabel->setText(
        QString("⟳ Reconnecting...\nPróba %1 za %2s")
            .arg(reconnectAttempt)
            .arg(secondsLeft)
    );
    emit reconnecting(reconnectAttempt);
}

// ---------------------------------------------------------------------------
// Prywatne metody
// ---------------------------------------------------------------------------

QSize FfmpegPlayer::probeFrameSize(const QString &rtspUrl)
{
    // POWAŻNA POPRAWKA (błąd funkcjonalny): jak w startVideoProcess() -
    // "-rtsp_transport" jest opcją tylko demuxera RTSP i dla adresów
    // HTTP/MJPEG mogłaby (w zależności od wersji ffprobe) spowodować błąd
    // otwarcia strumienia.
    QStringList probeArgs = {"-v", "error"};
    if (rtspUrl.startsWith("rtsp://", Qt::CaseInsensitive))
        probeArgs << "-rtsp_transport" << "tcp";
    probeArgs += QStringList{
        "-select_streams", "v:0",
        "-show_entries", "stream=width,height",
        "-of", "csv=s=x:p=0",
        rtspUrl
    };

    QProcess probe;
    probe.start("ffprobe", probeArgs);

    if (!probe.waitForFinished(8000)) {
        probe.kill();
        return {};
    }

    QString output = probe.readAllStandardOutput().trimmed();
    qDebug() << "ffprobe output:" << output;

    QRegularExpression re(R"((\d+)x(\d+))");
    auto match = re.match(output);
    if (match.hasMatch())
        return QSize(match.captured(1).toInt(), match.captured(2).toInt());
    return {};
}

void FfmpegPlayer::startVideoProcess()
{
    videoProcess = new QProcess(this);

    connect(videoProcess, &QProcess::readyReadStandardError, this, [this](){
        if (videoProcess)
            qDebug() << "FFmpeg:" << videoProcess->readAllStandardError().left(300);
    });
    connect(videoProcess, &QProcess::errorOccurred,
            this, &FfmpegPlayer::onVideoProcessError);
    connect(videoProcess,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &FfmpegPlayer::onVideoProcessFinished);

    // POWAŻNA POPRAWKA (błąd funkcjonalny): "-rtsp_transport tcp" jest
    // opcją WYŁĄCZNIE demuxera RTSP. Dla kamer HTTP/MJPEG (np.
    // "http://user:pass@ip/videostream.cgi") ffmpeg natychmiast kończy się
    // błędem "Option rtsp_transport not found" i NIGDY nie otwiera FIFO do
    // zapisu - w efekcie FfmpegReaderThread czeka w nieskończoność na
    // writer'a, który nigdy się nie pojawi (patrz też poprawka w
    // FfmpegReaderThread::run() poniżej). Dodajemy tę opcję tylko wtedy,
    // gdy URL faktycznie jest strumieniem RTSP.
    const bool isRtsp = rtspUrl.startsWith("rtsp://", Qt::CaseInsensitive);

    QStringList args = {
        "-nostats",
        "-y",
        "-fflags",        "nobuffer+discardcorrupt",
        "-flags",         "low_delay",
        "-strict",        "experimental",
        "-avioflags",     "direct",
    };
    if (isRtsp)
        args << "-rtsp_transport" << "tcp";
    args += QStringList{
        "-i",             rtspUrl,
        "-an",
        "-vf",            QString("scale=%1:%2,setpts=0").arg(frameSize.width()).arg(frameSize.height()),
        "-f",             "rawvideo",
        "-pix_fmt",       "rgb24",
        "-fps_mode",      "passthrough",
        "-flush_packets", "1",
        pipePath
    };

    qDebug() << "FfmpegPlayer: uruchamiam ffmpeg →" << pipePath;

    // Wątek czyta z FIFO - startuje przed FFmpeg (patrz komentarz w poprzedniej wersji)
    readerThread = new FfmpegReaderThread(pipePath, frameSize, this);
    connect(readerThread, &FfmpegReaderThread::frameReady,
            this, &FfmpegPlayer::onFrameReady, Qt::QueuedConnection);
    connect(readerThread, &FfmpegReaderThread::finished,
            readerThread, &QObject::deleteLater);
    // POWAŻNA POPRAWKA (dangling pointer / crash): jeśli wątek zakończy się
    // SAM (np. natychmiastowe, "czyste" EOF zaraz po podłączeniu writera,
    // albo błąd odczytu) - a nie przez FfmpegPlayer::stopAll() - do tej
    // pory NIC nie zerowało składowej `readerThread`. Obiekt kasował się
    // przez deleteLater() z połączenia powyżej, a `readerThread` zostawał
    // zwisającym wskaźnikiem do (wkrótce) usuniętego obiektu. Kolejne
    // wywołanie stopAll() (np. po kliknięciu Stop) robiło
    // `readerThread->stop()` na tym zwisającym wskaźniku → crash. Warunek
    // `readerThread == rt` chroni przed wyzerowaniem NOWSZEGO wątku, gdyby
    // w międzyczasie zdążył już powstać kolejny (np. przy szybkim
    // reconnect).
    connect(readerThread, &FfmpegReaderThread::finished, this,
            [this, rt = readerThread]() {
                if (readerThread == rt)
                    readerThread = nullptr;
            });
    readerThread->start();

    // POWAŻNA POPRAWKA (blokowanie GUI): QThread::msleep(200) blokował
    // wątek GUI przy KAŻDYM starcie kamery - w aplikacji uruchamiającej
    // wiele kamer naraz (MultiCamIp!) sumowało się to do zauważalnego
    // zamrożenia interfejsu. Ta pauza ma tylko dać wątkowi czytającemu
    // szansę otworzyć FIFO, zanim ffmpeg zacznie do niego pisać - robimy to
    // teraz asynchronicznie przez QTimer::singleShot, nie blokując GUI.
    QTimer::singleShot(200, this, [this, args]() {
        if (!videoProcess)
            return; // stop() zostało wywołane zanim minęło 200ms

        videoProcess->start(FFMPEG_BIN, args);

        if (!videoProcess->waitForStarted(5000)) {
            emit error("Nie udało się uruchomić ffmpeg");
            if (readerThread) {
                readerThread->stop();
                if (readerThread->wait(1000))
                    readerThread = nullptr;
                else
                    detachRunningReaderThread();
            }
            videoProcess->deleteLater();
            videoProcess = nullptr;
            QFile::remove(pipePath);
            return;
        }

        playing = true;
        emit playbackStarted();
        qDebug() << "FfmpegPlayer: odtwarzanie startuje -" << rtspUrl;
    });
}

// void FfmpegPlayer::startAudioProcess()
// {
//     audioProcess = new QProcess(this);
//     audioProcess->start(FFMPEG_BIN, {
//         "-fflags",  "nobuffer",
//         "-rtsp_transport", "tcp",
//         "-i",       rtspUrl,
//         "-vn",
//         "-acodec",  "pcm_s16le",
//         "-ar",      "44100",
//         "-ac",      "2",
//         "-f",       "pulse",
//         "default"
//     });
//     if (!audioProcess->waitForStarted(3000)) {
//         qWarning() << "FfmpegPlayer: nie udało się uruchomić audio FFmpeg";
//         audioProcess->deleteLater();
//         audioProcess = nullptr;
//     }
// }

void FfmpegPlayer::setVolume(int poziom)
{
    currentVolume = qBound(0, poziom, 10);
    if (!playing || !audioEnabled) return;

    // Debounce - czekamy 300ms od ostatniej zmiany zanim restartujemy audio.
    // Bez tego przeciąganie slidera generuje dziesiątki restartów na sekundę.
    if (!volumeDebounceTimer) {
        volumeDebounceTimer = new QTimer(this);
        volumeDebounceTimer->setSingleShot(true);
        volumeDebounceTimer->setInterval(300);
        connect(volumeDebounceTimer, &QTimer::timeout, this, [this]() {
            if (!playing || !audioEnabled) return;
            if (audioProcess) {
                audioProcess->blockSignals(true);
                if (audioProcess->state() != QProcess::NotRunning) {
                    audioProcess->terminate();
                    audioProcess->waitForFinished(1000);
                }
                audioProcess->deleteLater();
                audioProcess = nullptr;
            }
            if (currentVolume > 0)
                startAudioProcess();
        });
    }
    volumeDebounceTimer->start(); // restart timera przy każdej zmianie
}

void FfmpegPlayer::startAudioProcess()
{
    audioProcess = new QProcess(this);

    connect(audioProcess, &QProcess::readyReadStandardError,
            this, [this]()
            {
                qDebug() << "AUDIO:"
                         << audioProcess->readAllStandardError();
            });

    // Slider 0-10 → filtr volume FFmpeg 0.0-2.0
    // poziom 5 = 1.0 (normalna głośność), 10 = 2.0 (podwójna)
    QString vol = QString::number(currentVolume / 5.0, 'f', 2);

    // POWAŻNA POPRAWKA (błąd funkcjonalny): jak w startVideoProcess() -
    // "-rtsp_transport" tylko dla rtsp://, inaczej ffmpeg dla kamer
    // HTTP/MJPEG natychmiast kończy się błędem "Option rtsp_transport not
    // found" i proces audio wcale nie startuje.
    QStringList audioArgs = {"-fflags", "nobuffer"};
    if (rtspUrl.startsWith("rtsp://", Qt::CaseInsensitive))
        audioArgs << "-rtsp_transport" << "tcp";
    audioArgs += QStringList{
        "-i", rtspUrl,
        "-vn",
        "-af", QString("volume=%1").arg(vol),
        "-acodec", "pcm_s16le",
        "-ar", "44100",
        "-ac", "2",
        "-f", "pulse",
        "default"
    };
    audioProcess->start(FFMPEG_BIN, audioArgs);

    if (!audioProcess->waitForStarted(3000)) {
        qWarning() << "Nie udało się uruchomić audio";
        return;
    }

    qDebug() << "PID audio:" << audioProcess->processId() << "volume:" << vol;
}

void FfmpegPlayer::detachRunningReaderThread()
{
    // KRYTYCZNA POPRAWKA: Qt jawnie ostrzega, że usunięcie QThread, który
    // wciąż działa (isFinished() == false), prawdopodobnie kończy się
    // crashem. Wcześniej kod po nieudanym wait() po prostu zerował
    // wskaźnik readerThread, a sam obiekt (dziecko `this`) zostałby mimo
    // to usunięty razem z `this` w destruktorze QObject - jeśli wątek nadal
    // by działał, byłby to crash. Zamiast tego odłączamy go od `this`
    // (setParent(nullptr)), więc zniszczenie `this` już go nie dotyczy -
    // wątek sam się posprząta przez deleteLater() podpięte pod finished()
    // (patrz startVideoProcess()) gdy faktycznie zakończy pracę. Odpinamy
    // też frameReady, żeby "osierocony" wątek nie wołał już onFrameReady()
    // na tym playerze.
    if (!readerThread)
        return;
    qWarning() << "FfmpegPlayer: wątek czytający FIFO nie zakończył się w oczekiwanym czasie - odłączam go do samodzielnego zakończenia";
    disconnect(readerThread, &FfmpegReaderThread::frameReady, this, &FfmpegPlayer::onFrameReady);
    readerThread->setParent(nullptr);
    readerThread = nullptr;
}

void FfmpegPlayer::stopAll(bool clearPipePath)
{
    if (readerThread) {
        readerThread->stop();
        if (readerThread->wait(2000))
            readerThread = nullptr;
        else
            detachRunningReaderThread();
    }
    if (videoProcess) {
        videoProcess->blockSignals(true);
        if (videoProcess->state() != QProcess::NotRunning) {
            videoProcess->terminate();
            if (!videoProcess->waitForFinished(2000))
                videoProcess->kill();
        }
        videoProcess->deleteLater();
        videoProcess = nullptr;
    }
    if (audioProcess) {
        audioProcess->blockSignals(true);
        if (audioProcess->state() != QProcess::NotRunning) {
            audioProcess->terminate();
            if (!audioProcess->waitForFinished(2000))
                audioProcess->kill();
        }
        audioProcess->deleteLater();
        audioProcess = nullptr;
    }
    if (clearPipePath && !pipePath.isEmpty()) {
        QFile::remove(pipePath);
        pipePath.clear();
    }
    qDebug() << "FfmpegPlayer: zatrzymano";
}

void FfmpegPlayer::onFrameReady(const QImage &frame)
{
    if (!targetLabel || !playing) return;

    // POWAŻNA POPRAWKA (błędna logika licznika prób): wcześniej
    // reconnectAttempt było zerowane zaraz po samym WYWOŁANIU
    // startVideoProcess() w attemptReconnect(), niezależnie od tego, czy
    // ffmpeg faktycznie zaczął dostarczać obraz. Gdy ffmpeg padał od razu
    // (np. zły URL, brak kodeka), licznik i tak wracał do 0, więc każda
    // kolejna próba pokazywała się użytkownikowi jako "próba 1" w
    // nieskończoność, mimo że reconnectów było już wiele. Zerujemy licznik
    // dopiero tutaj - przy odbiorze PIERWSZEJ realnej klatki, czyli
    // potwierdzonym sukcesie połączenia.
    if (reconnectAttempt != 0)
        reconnectAttempt = 0;

    QPixmap pix = QPixmap::fromImage(frame).scaled(
        targetLabel->size(),
        //Qt::KeepAspectRatio,
        aspectRatioMode,
        Qt::SmoothTransformation
    );
    targetLabel->setPixmap(pix);
}
