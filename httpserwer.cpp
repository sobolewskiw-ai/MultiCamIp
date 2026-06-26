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

    // GET /kamery.dat - surowa zawartość pliku (binarny QDataStream)
    httpServer->route("/kamery.dat", QHttpServerRequest::Method::Get, [this]() {
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
