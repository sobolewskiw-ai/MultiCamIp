#ifndef FFMPEGPLAYER_H
#define FFMPEGPLAYER_H

#include <QObject>
#include <QProcess>
#include <QLabel>
#include <QThread>
#include <QImage>
#include <QSize>
#include <QTimer>
#include <atomic>

/**
 * FfmpegPlayer — odtwarza strumień RTSP przez zewnętrzny proces FFmpeg.
 *
 * Architektura:
 *   - FFmpeg zapisuje rawvideo RGB24 do named pipe (FIFO) w /tmp
 *   - FfmpegReaderThread czyta bezpośrednio z FIFO (QFile, thread-safe)
 *   - Ramki QImage emitowane sygnałem frameReady → QLabel::setPixmap
 *   - Osobny proces FFmpeg obsługuje audio → PulseAudio
 *   - Automatyczny reconnect po zerwaniu: co 10s próba, licznik na labelu
 *
 * Użycie:
 *   FfmpegPlayer *player = new FfmpegPlayer(this);
 *   player->setLabel(labelVideoVector[i]);
 *   player->setUrl("rtsp://localhost:8554/kamera1");
 *   player->play();   // play() wraca natychmiast (ffprobe w tle)
 *   player->stop();   // zatrzymuje odtwarzanie i reconnect
 */

class FfmpegReaderThread : public QThread
{
    Q_OBJECT
public:
    explicit FfmpegReaderThread(const QString &pipePath, QSize frameSize,
                                QObject *parent = nullptr);
    void stop();

signals:
    void frameReady(const QImage &frame);

protected:
    void run() override;

private:
    QString pipePath;
    QSize   frameSize;
    std::atomic<bool> running{true};
};

class FfmpegPlayer : public QObject
{
    Q_OBJECT
public:
    explicit FfmpegPlayer(QObject *parent = nullptr);
    ~FfmpegPlayer() override;

    void setLabel(QLabel *label);
    void setUrl(const QString &url);
    void setAudioEnabled(bool enabled);
    bool isPlaying() const;

    void play();
    void stop();
    void setAspectRatioMode(Qt::AspectRatioMode mode);

signals:
    void playbackStarted();
    void playbackStopped();
    void reconnecting(int attempt);   // emitowany przy każdej próbie reconnect
    void error(const QString &message);

private slots:
    void onFrameReady(const QImage &frame);
    void onVideoProcessError(QProcess::ProcessError err);
    void onVideoProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onReconnectTick();           // odpala się co 1s (odliczanie + próba co 10s)

private:
    void startVideoProcess();
    void startAudioProcess();
    void stopAll();
    void startReconnect();            // inicjuje tryb reconnect
    void attemptReconnect();          // jedna próba połączenia
    QSize probeFrameSize();
    void showReconnectLabel(int secondsLeft);

    QLabel              *targetLabel   = nullptr;
    QProcess            *videoProcess  = nullptr;
    QProcess            *audioProcess  = nullptr;
    FfmpegReaderThread  *readerThread  = nullptr;

    QString  rtspUrl;
    QString  pipePath;
    bool     audioEnabled  = false;
    QSize    frameSize;
    std::atomic<bool> playing{false};
    Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio;

    // Reconnect
    QTimer  *reconnectTimer  = nullptr;   // tyka co 1s
    int      reconnectCountdown = 0;      // sekundy do następnej próby
    int      reconnectAttempt   = 0;      // numer próby (do logowania)
    static constexpr int RECONNECT_INTERVAL = 10; // sekund między próbami

    static constexpr const char *FFMPEG_BIN = "ffmpeg";
};

#endif // FFMPEGPLAYER_H
