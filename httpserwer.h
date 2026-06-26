#ifndef HTTPSERWER_H
#define HTTPSERWER_H

#include <QObject>
#include <QString>

class QHttpServer;
class QTcpServer;

/**
 * Prosty serwer HTTP udostępniający plik kamery.dat z katalogu appHomePath.
 *
 * Endpointy:
 *   GET /            - prosty status tekstowy (czy serwer żyje, ścieżka do pliku)
 *   GET /kamery.dat  - surowa zawartość pliku kamery.dat (application/octet-stream)
 *
 * Plik kamery.dat jest plikiem binarnym (QDataStream / QStandardItem), więc
 * jest serwowany bajt-w-bajt, bez parsowania po stronie serwera - klient
 * (np. inna instancja aplikacji) może go pobrać i wczytać tym samym
 * mechanizmem co lokalnie (CameraDataReader::czytajListaKamer()).
 */
class HttpSerwer : public QObject
{
    Q_OBJECT
public:
    explicit HttpSerwer(QObject *parent = nullptr);
    ~HttpSerwer() override;

    // Uruchamia serwer na podanym porcie, czytając kamery.dat z homePath.
    // Zwraca true, jeśli serwer wystartował poprawnie.
    bool start(const QString &homePath, quint16 port = 8080);

    // Zatrzymuje serwer (zwalnia port). Bezpieczne wywołanie nawet jeśli
    // serwer nie był uruchomiony.
    void stop();

    bool isRunning() const;
    quint16 serverPort() const;

signals:
    void serverStarted(quint16 port);
    void serverStopped();
    void serverError(const QString &message);

private:
    QHttpServer *httpServer = nullptr;
    QTcpServer *tcpServer = nullptr;
    QString appHomePath;
};

#endif // HTTPSERWER_H
