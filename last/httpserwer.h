#ifndef HTTPSERWER_H
#define HTTPSERWER_H

#include <QObject>
#include <QString>
#include <QHttpServerResponse>

class QHttpServer;
class QTcpServer;
class QHttpServerRequest;

/**
 * Prosty serwer HTTP udostępniający plik kamery.dat z katalogu appHomePath.
 *
 * Endpointy:
 *   GET /            - prosty status tekstowy (czy serwer żyje, ścieżka do pliku),
 *                       nie wymaga uwierzytelnienia.
 *   GET  /kamery.dat, /strumienie.dat, /strefa.dat - surowa zawartość pliku
 *   PUT  /kamery.dat, /strumienie.dat, /strefa.dat - nadpisanie pliku
 *
 * Pliki .dat zawierają m.in. adresy RTSP/HTTP kamer wraz z loginem i hasłem
 * w postaci jawnej, dlatego odczyt i zapis tych endpointów WYMAGA nagłówka
 * "X-Auth-Token" zgodnego z tokenem wygenerowanym przy pierwszym starcie
 * serwera i zapisanym w <homePath>/.http_auth_token. Bez tego każdy w tej
 * samej sieci mógłby odczytać hasła kamer albo podmienić listę kamer innej
 * instancji aplikacji.
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

    // Token wymagany w nagłówku "X-Auth-Token" przez inne instancje, żeby
    // mogły odczytać/zapisać pliki .dat. Trzeba go im przekazać ręcznie
    // (np. skopiować z pliku .http_auth_token).
    QString authToken() const { return authToken_; }

    QHttpServerResponse odczytajPlikDat(const QString &nazwaPliku);
    QHttpServerResponse zapiszPlikDat(
        const QString &nazwaPliku,
        const QByteArray &data);

signals:
    void serverStarted(quint16 port);
    void serverStopped();
    void serverError(const QString &message);

private:
    QString loadOrCreateAuthToken(const QString &homePath);
    bool isAuthorized(const QHttpServerRequest &request) const;

    QHttpServer *httpServer = nullptr;
    QTcpServer *tcpServer = nullptr;
    QString appHomePath;
    QString authToken_;
};

#endif // HTTPSERWER_H