#include "httpserwer.h"

#include <QHttpServer>
#include <QHttpServerResponse>
#include <QTcpServer>
#include <QHostAddress>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

HttpSerwer::HttpSerwer(QObject *parent)
    : QObject{parent}
{
}

HttpSerwer::~HttpSerwer()
{
    stop();
}

bool HttpSerwer::start(const QString &homePath, quint16 port)
{
    if (isRunning()) {
        qDebug() << "HttpSerwer: serwer już działa na porcie" << serverPort();
        return true;
    }

    appHomePath = homePath;

    httpServer = new QHttpServer(this);

    // GET / - prosty status, żeby łatwo sprawdzić czy serwer żyje
    httpServer->route("/", QHttpServerRequest::Method::Get, [this]() {
        QString plikPath = appHomePath + "/kamery.dat";
        bool istnieje = QFile::exists(plikPath);
        QString status = QStringLiteral(
            "MultiCamIp - serwer HTTP działa.\n"
            "appHomePath: %1\n"
            "kamery.dat istnieje: %2\n"
            "Pobierz liste kamer: GET /kamery.dat\n"
        ).arg(appHomePath, istnieje ? "tak" : "nie");
        return QHttpServerResponse("text/plain; charset=utf-8", status.toUtf8());
    });

    httpServer->route("/kamery.dat",
                      QHttpServerRequest::Method::Get,
                      [this]() {
                          return odczytajPlikDat("kamery.dat");
                      });

    httpServer->route("/strumienie.dat",
                      QHttpServerRequest::Method::Get,
                      [this]() {
                          return odczytajPlikDat("strumienie.dat");
                      });

    httpServer->route("/strefa.dat",
                      QHttpServerRequest::Method::Get,
                      [this]() {
                          return odczytajPlikDat("strefa.dat");
                      });
    httpServer->route("/kamery.dat",
                      QHttpServerRequest::Method::Put,
                      [this](const QHttpServerRequest &request) {
                          return zapiszPlikDat("kamery.dat", request.body());
                      });

    httpServer->route("/strumienie.dat",
                      QHttpServerRequest::Method::Put,
                      [this](const QHttpServerRequest &request) {
                          return zapiszPlikDat("strumienie.dat", request.body());
                      });

    httpServer->route("/strefa.dat",
                      QHttpServerRequest::Method::Put,
                      [this](const QHttpServerRequest &request) {
                          return zapiszPlikDat("strefa.dat", request.body());
                      });

    // GET /kamery.dat - surowa zawartość pliku (binarny QDataStream)
/*    httpServer->route("/kamery.dat", QHttpServerRequest::Method::Get, [this]() {
        QString plikPath = appHomePath + "/kamery.dat";
        QFileInfo info(plikPath);

        if (!info.exists() || !info.isFile()) {
            qWarning() << "HttpSerwer: plik kamery.dat nie istnieje:" << plikPath;
            QString msg = QStringLiteral("Plik kamery.dat nie istnieje w %1").arg(appHomePath);
            return QHttpServerResponse("text/plain; charset=utf-8", msg.toUtf8(),
                                        QHttpServerResponse::StatusCode::NotFound);
        }

        QFile file(plikPath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "HttpSerwer: nie można otworzyć kamery.dat:" << file.errorString();
            return QHttpServerResponse("text/plain; charset=utf-8",
                                        QByteArrayLiteral("Nie można otworzyć kamery.dat"),
                                        QHttpServerResponse::StatusCode::InternalServerError);
        }

        QByteArray data = file.readAll();
        file.close();

        // application/octet-stream - to plik binarny (QDataStream), klient
        // (inna instancja aplikacji) wie jak go zinterpretować.
        return QHttpServerResponse("application/octet-stream", data);
    });

    // PUT /kamery.dat - zapis binarnego QDataStream
    httpServer->route("/kamery.dat", QHttpServerRequest::Method::Put,
        [this](const QHttpServerRequest &request) {

        QString plikPath = appHomePath + "/kamery.dat";
        QByteArray data = request.body();
        if (data.isEmpty()) {
            qWarning() << "HttpSerwer: otrzymano pusty plik kamery.dat";
            return QHttpServerResponse(
            "text/plain; charset=utf-8",
            QByteArrayLiteral("Pusty plik"),
            QHttpServerResponse::StatusCode::BadRequest
            );
        }
        QFile file(plikPath);
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning() << "HttpSerwer: nie można zapisać kamery.dat:"
                       << file.errorString();
            return QHttpServerResponse(
            "text/plain; charset=utf-8",
            QByteArrayLiteral("Nie można zapisać kamery.dat"),
            QHttpServerResponse::StatusCode::InternalServerError
            );
        }
        qint64 zapisano = file.write(data);
        file.close();
        if (zapisano != data.size()) {
            qWarning() << "HttpSerwer: nie zapisano całych danych:"
                       << zapisano << "/" << data.size();
            return QHttpServerResponse(
            "text/plain; charset=utf-8",
            QByteArrayLiteral("Błąd zapisu pliku"),
            QHttpServerResponse::StatusCode::InternalServerError
            );
        }
        qDebug() << "HttpSerwer: zapisano kamery.dat:"
                 << data.size() << "bajtów";
        return QHttpServerResponse(
        "text/plain; charset=utf-8",
        QByteArrayLiteral("OK")
        );
    });
*/
    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen(QHostAddress::Any, port)) {
        QString err = tcpServer->errorString();
        qWarning() << "HttpSerwer: nie udało się nasłuchiwać na porcie" << port << "-" << err;
        emit serverError(err);
        delete tcpServer;
        tcpServer = nullptr;
        delete httpServer;
        httpServer = nullptr;
        return false;
    }

    if (!httpServer->bind(tcpServer)) {
        qWarning() << "HttpSerwer: bind() nie powiodło się";
        emit serverError("Nie udało się powiązać QHttpServer z QTcpServer");
        delete tcpServer; // httpServer->bind nie przejął własności, bo zwrócił false
        tcpServer = nullptr;
        delete httpServer;
        httpServer = nullptr;
        return false;
    }
    // Po sukcesie bind() Qt zmienia parenta tcpServer na httpServer (zgodnie
    // z dokumentacją QAbstractHttpServer::bind()). To nie ma wpływu na nasze
    // jawne stop()/deleteLater() poniżej - wskaźnik tcpServer pozostaje ważny
    // i bezpieczny do usunięcia niezależnie od tego, kto jest jego rodzicem.

    qDebug() << "HttpSerwer: serwer HTTP wystartował na porcie" << tcpServer->serverPort()
              << "- katalog:" << appHomePath;
    emit serverStarted(tcpServer->serverPort());
    return true;
}

void HttpSerwer::stop()
{
    if (tcpServer) {
        tcpServer->close();
    }
    if (httpServer) {
        httpServer->deleteLater();
        httpServer = nullptr;
    }
    if (tcpServer) {
        tcpServer->deleteLater();
        tcpServer = nullptr;
    }
    emit serverStopped();
}

bool HttpSerwer::isRunning() const
{
    return tcpServer != nullptr && tcpServer->isListening();
}

quint16 HttpSerwer::serverPort() const
{
    return tcpServer ? tcpServer->serverPort() : 0;
}

QHttpServerResponse HttpSerwer::odczytajPlikDat(const QString &nazwaPliku)
{
    const QStringList dozwolonePliki = {
        "kamery.dat",
        "strumienie.dat",
        "strefa.dat"
    };

    if (!dozwolonePliki.contains(nazwaPliku)) {

        qWarning() << "HttpSerwer: niedozwolony plik:"
                   << nazwaPliku;

        return QHttpServerResponse(
            "text/plain; charset=utf-8",
            QByteArrayLiteral("Niedozwolony plik"),
            QHttpServerResponse::StatusCode::NotFound
            );
    }

    QString plikPath = appHomePath + "/" + nazwaPliku;

    QFileInfo info(plikPath);

    if (!info.exists() || !info.isFile()) {

        qWarning() << "HttpSerwer: plik nie istnieje:"
                   << plikPath;

        QString msg = QStringLiteral(
                          "Plik %1 nie istnieje w %2"
                          ).arg(nazwaPliku, appHomePath);

        return QHttpServerResponse(
            "text/plain; charset=utf-8",
            msg.toUtf8(),
            QHttpServerResponse::StatusCode::NotFound
            );
    }

    QFile file(plikPath);

    if (!file.open(QIODevice::ReadOnly)) {

        qWarning() << "HttpSerwer: nie można otworzyć:"
                   << plikPath
                   << file.errorString();

        return QHttpServerResponse(
            "text/plain; charset=utf-8",
            QByteArrayLiteral("Nie można otworzyć pliku"),
            QHttpServerResponse::StatusCode::InternalServerError
            );
    }

    QByteArray data = file.readAll();

    file.close();

    qDebug() << "HttpSerwer: wysłano"
             << nazwaPliku
             << data.size()
             << "bajtów";

    return QHttpServerResponse(
        "application/octet-stream",
        data
        );
}

QHttpServerResponse HttpSerwer::zapiszPlikDat(
    const QString &nazwaPliku,
    const QByteArray &data)
{
    const QStringList dozwolonePliki = {
        "kamery.dat",
        "strumienie.dat",
        "strefa.dat"
    };

    if (!dozwolonePliki.contains(nazwaPliku)) {

        qWarning() << "HttpSerwer: niedozwolony plik:"
                   << nazwaPliku;

        return QHttpServerResponse(
            "text/plain; charset=utf-8",
            QByteArrayLiteral("Niedozwolony plik"),
            QHttpServerResponse::StatusCode::NotFound
            );
    }

    if (data.isEmpty()) {

        qWarning() << "HttpSerwer: otrzymano pusty plik:"
                   << nazwaPliku;

        return QHttpServerResponse(
            "text/plain; charset=utf-8",
            QByteArrayLiteral("Pusty plik"),
            QHttpServerResponse::StatusCode::BadRequest
            );
    }

    QString plikPath = appHomePath + "/" + nazwaPliku;

    QFile file(plikPath);

    if (!file.open(QIODevice::WriteOnly)) {

        qWarning() << "HttpSerwer: nie można zapisać:"
                   << plikPath
                   << file.errorString();

        return QHttpServerResponse(
            "text/plain; charset=utf-8",
            QByteArrayLiteral("Nie można zapisać pliku"),
            QHttpServerResponse::StatusCode::InternalServerError
            );
    }

    qint64 zapisano = file.write(data);

    file.close();

    if (zapisano != data.size()) {

        qWarning() << "HttpSerwer: nie zapisano całych danych:"
                   << nazwaPliku
                   << zapisano
                   << "/"
                   << data.size();

        return QHttpServerResponse(
            "text/plain; charset=utf-8",
            QByteArrayLiteral("Błąd zapisu pliku"),
            QHttpServerResponse::StatusCode::InternalServerError
            );
    }

    qDebug() << "HttpSerwer: zapisano"
             << nazwaPliku
             << data.size()
             << "bajtów";

    return QHttpServerResponse(
        "text/plain; charset=utf-8",
        QByteArrayLiteral("OK")
        );
}
