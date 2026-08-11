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
#include <QUrl>

#include <unistd.h>
#include <sys/stat.h>

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
}

void FfmpegReaderThread::run()
{
    const qint64 bytesPerFrame = (qint64)frameSize.width() * frameSize.height() * 3;
    if (bytesPerFrame <= 0) {
        qWarning() << "FfmpegReaderThread: nieprawidłowy rozmiar ramki" << frameSize;
        return;
    }

    QFile fifo(pipePath);
    int waited = 0;
    while (!fifo.open(QIODevice::ReadOnly) && waited < 100) {
        msleep(100);
        waited++;
    }
    if (!fifo.isOpen()) {
        qWarning() << "FfmpegReaderThread: nie udało się otworzyć FIFO:" << pipePath;
        return;
    }

    QByteArray frameBuffer(bytesPerFrame, Qt::Uninitialized);

    while (running) {
        qint64 bytesRead = 0;
        while (running && bytesRead < bytesPerFrame) {
            qint64 n = fifo.read(frameBuffer.data() + bytesRead,
                                 bytesPerFrame - bytesRead);
            if (n < 0) goto done;
            if (n > 0) bytesRead += n;
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

    // probeFrameSize() w tle - nie blokuje GUI
    QThreadPool::globalInstance()->start([this]() {
        QSize size = probeFrameSize();
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
    if (!playing && !reconnectTimer) return;

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
    // Zatrzymujemy stare procesy i wątek (bez ustawiania playing=false)
    stopAll();

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

    // Szybkie sprawdzenie czy host jest dostępny (TCP na port RTSP)
    // Robimy to w tle żeby nie blokować GUI
    QThreadPool::globalInstance()->start([this]() {
        // Wyciągamy host i port z URL RTSP
        QUrl url(rtspUrl);
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
                    reconnectTimer->start(); // próbujemy dalej
                    return;
                }
            }

            startVideoProcess();
            if (audioEnabled)
                startAudioProcess();

            reconnectAttempt = 0; // sukces - resetujemy licznik
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

QSize FfmpegPlayer::probeFrameSize()
{
    QProcess probe;
    probe.start("ffprobe", {
        "-v", "error",
        "-rtsp_transport", "tcp",
        "-select_streams", "v:0",
        "-show_entries", "stream=width,height",
        "-of", "csv=s=x:p=0",
        rtspUrl
    });

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

    QStringList args = {
        "-nostats",
        "-y",
        "-fflags",        "nobuffer+discardcorrupt",
        "-flags",         "low_delay",
        "-strict",        "experimental",
        "-avioflags",     "direct",
        "-rtsp_transport","tcp",
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
    readerThread->start();

    QThread::msleep(200);

    videoProcess->start(FFMPEG_BIN, args);

    if (!videoProcess->waitForStarted(5000)) {
        emit error("Nie udało się uruchomić ffmpeg");
        readerThread->stop();
        readerThread->wait(1000);
        videoProcess->deleteLater();
        videoProcess = nullptr;
        QFile::remove(pipePath);
        return;
    }

    playing = true;
    emit playbackStarted();
    qDebug() << "FfmpegPlayer: odtwarzanie startuje -" << rtspUrl;
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

    audioProcess->start(FFMPEG_BIN, {
        "-fflags", "nobuffer",
        "-rtsp_transport", "tcp",
        "-i", rtspUrl,
        "-vn",
        "-af", QString("volume=%1").arg(vol),
        "-acodec", "pcm_s16le",
        "-ar", "44100",
        "-ac", "2",
        "-f", "pulse",
        "default"
    });

    if (!audioProcess->waitForStarted(3000)) {
        qWarning() << "Nie udało się uruchomić audio";
        return;
    }

    qDebug() << "PID audio:" << audioProcess->processId() << "volume:" << vol;
}

void FfmpegPlayer::stopAll()
{
    if (readerThread) {
        readerThread->stop();
        readerThread->wait(2000);
        readerThread = nullptr;
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
    if (!pipePath.isEmpty()) {
        QFile::remove(pipePath);
        pipePath.clear();
    }
    qDebug() << "FfmpegPlayer: zatrzymano";
}

void FfmpegPlayer::onFrameReady(const QImage &frame)
{
    if (!targetLabel || !playing) return;
    QPixmap pix = QPixmap::fromImage(frame).scaled(
        targetLabel->size(),
        //Qt::KeepAspectRatio,
        aspectRatioMode,
        Qt::SmoothTransformation
    );
    targetLabel->setPixmap(pix);
}
