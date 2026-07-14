#include "findnewcamera.h"
#include "mainwindow.h"

#include <QDebug>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QGuiApplication>
#include <QScreen>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QSettings>
#include <QMessageBox>
#include <QTimer>
#include <QInputDialog>
#include <QStandardItem>
#include <QDir>
#include <QFile>
#include <QDataStream>
#include <QTextStream>
#include <QThreadPool>

#include <QSpinBox>
#include <QLineEdit>
#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>

// ============================================================
// CameraDataReader – lekki helper (bez UI) do odczytu kamer
// Bezpieczny w wątkach roboczych (nie dziedziczy po QWidget)
// ============================================================
CameraDataReader::CameraDataReader()
    : ItemModel(new QStandardItemModel())
{

}

CameraDataReader::~CameraDataReader()
{
    delete ItemModel;
}

void CameraDataReader::czytajListaKamer()
{
    ItemModel->clear();
    QString path = QDir::homePath() + "/CameraDir/";
    path = path.simplified();
    path.remove(" ");

    QFile file(path + "kamery.dat");
    if (file.open(QIODevice::ReadOnly))
    {
        QDataStream stream(&file);
        qint32 n, m;
        stream >> n >> m;
        ItemModel->setRowCount(n);
        ItemModel->setColumnCount(m);

        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < m; ++j) {
                QStandardItem *item = new QStandardItem;
                item->setTextAlignment(Qt::AlignCenter);
                item->read(stream);
                ItemModel->setItem(i, j, item);
            }
        }
        file.close();
    }
}

// ============================================================

FindNewCamera::FindNewCamera(QWidget *parent)
    : QWidget{parent}
{
    // Bez tego, close() (wywołane w buttonZamknij_cliced) tylko chowa widget,
    // nie usuwa go z pamięci. Przy każdym "Szukaj kamer" powstawała nowa
    // instancja, a stare (zamknięte, ale wciąż żywe) zostawały w pamięci
    // razem z otwartymi cv::VideoCapture i działającymi QTimer-ami - to było
    // główną przyczyną spowalniania/zawieszania się programu po kilku
    // powtórzeniach skanowania.
    setAttribute(Qt::WA_DeleteOnClose);

    // Użyj istniejącego MainWindow (przekazanego jako parent) zamiast tworzyć nowy.
    // Tworzenie "new MainWindow()" tutaj powodowało wyciek pamięci - dodatkowe,
    // nigdy niepokazywane i nigdy nieusuwane okno główne.
    mainwindow = qobject_cast<MainWindow*>(parent);
    if (mainwindow) {
        qDebug() << "homePath" << mainwindow->appHomePath;
    } else {
        qWarning() << "FindNewCamera: parent nie jest MainWindow - appHomePath niedostępny";
    }

    // QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    // int width = screenGeometry.width() * 0.8;
    // int height = screenGeometry.height() * 0.7;
    // this->resize(width,height);
    // this->setMinimumWidth(width);
    // this->setMinimumHeight(height);

    QString stylesheetPushButton =
        "QPushButton {"
        "   background: none;"
        "   border: 2px solid #0078D7;"
        "   border-radius: 6px;"
        "   background-color: #0078D7;"
        "   color: white;"
        "   padding: 6px;"
        "   font-weight: bold;"
        "   font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #3399FF;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #005999;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #FFFFFF;"
        "}";
    QString stylesheetLabel =
        "border: 2px solid #0078D7;"
        "border-radius: 3px;"
        "padding: 4px;"
        "background-color: #F0F8FF;"
        "font-weight: bold;"
        "font-size: 14px;";

    // ===============================
    // --- GŁÓWNY UKŁAD ---
    // ===============================
    QHBoxLayout *centralLayout = new QHBoxLayout(this);

    // --- LEWY UKŁAD ---
    QVBoxLayout *layoutLewy = new QVBoxLayout();
    // ===============================
    // Poziom 1 – Adres IP kamery
    // ===============================
    QHBoxLayout *layoutPoziom1 = new QHBoxLayout();
    labelAdresIpKamery = new QLabel("Wpisz adres IP kamery:", this);
    labelAdresIpKamery->setAlignment(Qt::AlignCenter);
    labelAdresIpKamery->setStyleSheet(stylesheetLabel);
    //labelAdresIpKamery->setFixedWidth(200);
    LineEditAdresIpKamery = new QLineEdit(this);
    LineEditAdresIpKamery->setPlaceholderText("np: 192.168.2.111");
    LineEditAdresIpKamery->setStyleSheet(stylesheetLabel);
    layoutPoziom1->addWidget(labelAdresIpKamery,6);
    layoutPoziom1->addWidget(LineEditAdresIpKamery,4);
    // ===============================
    // Poziom 1_1 – PORTY RTSP
    // ===============================
    QHBoxLayout *layoutPoziom1_1 = new QHBoxLayout();
    labelPortyRTSP = new QLabel("Po przecinku wpisz porty rtsp:", this);
    labelPortyRTSP->setAlignment(Qt::AlignCenter);
    labelPortyRTSP->setStyleSheet(stylesheetLabel);
    //labelPortyRTSP->setFixedWidth(200);
    LineEditPortyRTSP = new QLineEdit(this);
    LineEditPortyRTSP->setPlaceholderText("np: 554, 10554");
    LineEditPortyRTSP->setStyleSheet(stylesheetLabel);
    layoutPoziom1_1->addWidget(labelPortyRTSP,6);
    layoutPoziom1_1->addWidget(LineEditPortyRTSP,4);
    // ===============================
    // Poziom 1_2 – PORTY HTTP
    // ===============================
    QHBoxLayout *layoutPoziom1_2 = new QHBoxLayout();
    labelPortyHTTP = new QLabel("Po przecinku wpisz porty http:", this);
    labelPortyHTTP->setAlignment(Qt::AlignCenter);
    labelPortyHTTP->setStyleSheet(stylesheetLabel);
    //labelPortyRTSP->setFixedWidth(200);
    LineEditPortyHTTP = new QLineEdit(this);
    LineEditPortyHTTP->setPlaceholderText("np: 80, 8080, 8000, 8888");
    LineEditPortyHTTP->setStyleSheet(stylesheetLabel);
    layoutPoziom1_2->addWidget(labelPortyHTTP,6);
    layoutPoziom1_2->addWidget(LineEditPortyHTTP,4);
    // ===============================
    // Poziom 2 – Login
    // ===============================
    QHBoxLayout *layoutPoziom2 = new QHBoxLayout();
    labelCameraLogin = new QLabel("Login:", this);
    labelCameraLogin->setAlignment(Qt::AlignCenter);
    labelCameraLogin->setStyleSheet(stylesheetLabel);
    //labelCameraLogin->setFixedWidth(200);
    LineEditCameraLogin = new QLineEdit(this);
    LineEditCameraLogin->setPlaceholderText("admin");
    LineEditCameraLogin->setStyleSheet(stylesheetLabel);
    layoutPoziom2->addWidget(labelCameraLogin,6);
    layoutPoziom2->addWidget(LineEditCameraLogin,4);
    // ===============================
    // Poziom 3 – Hasło
    // ===============================
    QHBoxLayout *layoutPoziom3 = new QHBoxLayout();
    labelCameraPassword = new QLabel("Password:", this);
    labelCameraPassword->setAlignment(Qt::AlignCenter);
    labelCameraPassword->setStyleSheet(stylesheetLabel);
    //labelCameraPassword->setFixedWidth(200);
    LineEditCameraPassword = new QLineEdit(this);
    LineEditCameraPassword->setPlaceholderText("123456");
    LineEditCameraPassword->setStyleSheet(stylesheetLabel);
    layoutPoziom3->addWidget(labelCameraPassword,6);
    layoutPoziom3->addWidget(LineEditCameraPassword,4);
    // ===============================
    // Poziom 4 Przycisk i lista
    // ===============================
    buttonSkanuj = new QPushButton("Skanuj", this);
    buttonSkanuj->setStyleSheet(stylesheetPushButton);
    buttonSkanuj->setMinimumWidth(400); // wymusza minimalną szerokość layoutLewy
    widgetListListaStrumieni = new QListWidget(this);
    // ===============================
    // Poziom 5 Przyciski play zapisz zamknij
    // ===============================
    QHBoxLayout *layoutPoziom4 = new QHBoxLayout();
    buttonPlay = new QPushButton("Play", this);
    buttonPlay->setStyleSheet(stylesheetPushButton);
    buttonZapisz = new QPushButton("Zapisz", this);
    buttonZapisz->setStyleSheet(stylesheetPushButton);
    buttonZapisz->setEnabled(false);
    buttonZamknij = new QPushButton("Zamknij", this);
    buttonZamknij->setStyleSheet(stylesheetPushButton);
    layoutPoziom4->addWidget(buttonPlay);
    layoutPoziom4->addWidget(buttonZapisz);
    layoutPoziom4->addWidget(buttonZamknij);
    // ===============================
    // Dodanie do lewego layoutu
    // ===============================
    layoutLewy->addLayout(layoutPoziom1);
    layoutLewy->addLayout(layoutPoziom1_1);
    layoutLewy->addLayout(layoutPoziom1_2);
    layoutLewy->addLayout(layoutPoziom2);
    layoutLewy->addLayout(layoutPoziom3);
    layoutLewy->addWidget(buttonSkanuj);
    layoutLewy->addWidget(widgetListListaStrumieni);
    layoutLewy->addLayout(layoutPoziom4);
    // ===============================
    // --- LEWY UKŁAD ---
    // ===============================
    labelPlay = new QLabel("Player of video",this);
    labelPlay->setAlignment(Qt::AlignCenter);
    // ===============================
    // --- CENTRALNY UKŁAD ---
    // ===============================
    centralLayout->addLayout(layoutLewy,3);
    centralLayout->addWidget(labelPlay,7);
    //centralLayout->setStretch(0, 30);  //to samo co: centralLayout->addLayout(layoutLewy,3);
    //centralLayout->setStretch(1, 70);  //to samo co: centralLayout->addWidget(labelPlay,7);
    setLayout(centralLayout);

    // ===============================
    // --- ZAPAMIĘTAJ LAST VALUE W LINE EDIT ---
    // ===============================
    QSettings lastValue("MojaFirma", "CameraSerwer");
    LineEditAdresIpKamery->setText(lastValue.value("adresIpKamery", "192.168.0.100").toString());
    LineEditCameraLogin->setText(lastValue.value("login", "admin").toString());
    LineEditCameraPassword->setText(lastValue.value("password", "").toString());
    connect(LineEditAdresIpKamery, &QLineEdit::textChanged, this, [](const QString &text){
        QSettings lastValueWrite("MojaFirma", "CameraSerwer");
        lastValueWrite.setValue("adresIpKamery", text);
    });
    connect(LineEditCameraLogin, &QLineEdit::textChanged, this, [](const QString &text){
        QSettings lastValueWrite("MojaFirma", "CameraSerwer");
        lastValueWrite.setValue("login", text);
    });
    connect(LineEditCameraPassword, &QLineEdit::textChanged, this, [](const QString &text){
        QSettings lastValueWrite("MojaFirma", "CameraSerwer");
        lastValueWrite.setValue("password", text);
    });
    // ===============================
    // ---QPushButton *buttonZamknij CLICED ---
    // ===============================
    connect(buttonZamknij, &QPushButton::clicked, this, &FindNewCamera::buttonZamknij_cliced);
    connect(buttonSkanuj, &QPushButton::clicked, this, &FindNewCamera::buttonSkanuj_clicked);
    connect(buttonPlay, &QPushButton::clicked, this, &FindNewCamera::buttonPlay_clicked);
    connect(buttonZapisz, &QPushButton::clicked, this , &FindNewCamera::buttonZapisz_cliced);
    connect(widgetListListaStrumieni, &QListWidget::currentItemChanged, this, [this] {
        // Inwalidujemy ewentualne trwające w tle otwieranie strumienia (Play),
        // żeby jego wynik nie nadpisał stanu po zmianie wyboru.
        ++playRequestId;
        if (cap.isOpened()) {
            timer.stop();
            cap.release();
            labelPlay->clear();
        }
        buttonZapisz->setEnabled(false);
        buttonPlay->setEnabled(true);
        buttonPlay->setText("Play");
    });

    connect(&timer, &QTimer::timeout, this, &FindNewCamera::updateFrame);
}

FindNewCamera::~FindNewCamera()
{
    // Zabezpieczenie defensywne: nawet gdyby destruktor został wywołany
    // bez przejścia przez buttonZamknij_cliced (np. zamknięcie aplikacji
    // w trakcie podglądu), upewniamy się że strumień video, timer
    // odświeżania i wątek skanowania są poprawnie zatrzymane.
    ++playRequestId; // inwaliduj ewentualne trwające w tle otwieranie strumienia
    timer.stop();
    if (cap.isOpened()) {
        cap.release();
    }
    if (watcher.isRunning()) {
        watcher.cancel();
        watcher.waitForFinished();
    }
    // progressDialog jest dziecko-QWidget (this), więc Qt usunąłby go
    // automatycznie - ale jeśli skanowanie zostało przerwane w trakcie,
    // jawne usunięcie od razu zwalnia pamięć bez czekania na event loop.
    delete progressDialog;
    progressDialog = nullptr;
}

void FindNewCamera::buttonZamknij_cliced()
{
    emit requestCloseFrame();
    timer.stop();
    cap.release();
    labelPlay->clear();
    widgetListListaStrumieni->clear();
//    listaKamer.clear();
    close();
}

void FindNewCamera::buttonSkanuj_clicked()
{
    // Jeśli poprzednie skanowanie wciąż trwa, nie zaczynamy nowego -
    // dzielimy jeden watcher między wywołania, więc nadpisanie progressDialog
    // i podłączenie nowego future zostawiałoby stare skanowanie działające
    // "w tle" bez żadnego sposobu na jego obserwację, co kumulowało obciążenie
    // CPU/sieci po kilku kolejnych skanowaniach.
    if (watcher.isRunning()) {
        QMessageBox::information(this, "Skanowanie w toku",
            "Poprzednie skanowanie jeszcze się nie zakończyło. Poczekaj na jego koniec lub kliknij Anuluj.");
        return;
    }

    widgetListListaStrumieni->clear();
    kamera.clear();
    //listaKamer.clear();


    QString ip = LineEditAdresIpKamery->text().trimmed();
    QString user = LineEditCameraLogin->text().trimmed();
    QString pass = LineEditCameraPassword->text().trimmed();

    if (ip.isEmpty()) {
        QMessageBox::warning(this, "Błąd", "Podaj adres IP kamery!");
        return;
    }

    //QList<int> rtspPorts = {554, 8554, 10554};
    QString portsRTSPText = LineEditPortyRTSP->text();
    QList<int> rtspPorts;
    for (const QString &portRtsp : portsRTSPText.split(',', Qt::SkipEmptyParts)) {
        bool ok = false;
        int port = portRtsp.trimmed().toInt(&ok);
        if (ok){
            rtspPorts.append(port);
        }
    }
    QStringList rtspPaths = {
        "/live/ch0",
        "/live/ch1",
        "/h264",
        "/stream1",
        "/stream2",
        "/av_stream",
        "/tcp/av0_1",
        "/tcp/av0_0",
        "/Streaming/Channels/1",
        "/cam/realmonitor?channel=1&subtype=0",
        "/h264/ch1/main/av_stream",
        "/h264/ch1/sub/av_stream",
        "/h264Preview_01_main",
        "/Streaming/Channels/101",
        "/live.sdp",
        "/axis-media/media.amp",
        "/h264_stream",
        "/videoMain",
        "/onvif1",
        "/live"
    };

    //QList<int> httpPorts = {80, 8080, 8000, 8888};
    QString portsHTTPText = LineEditPortyHTTP->text();  // poprawka: było LineEditPortyRTSP
    QList<int> httpPorts;
    for (const QString &portHttp : portsHTTPText.split(',', Qt::SkipEmptyParts)) {
        bool ok = false;
        int port = portHttp.trimmed().toInt(&ok);
        if (ok)
            httpPorts.append(port);
    }

    QStringList httpPaths = {
        "/video",
        "/video.mjpg",
        "/mjpg/video.mjpg",
        "/mjpeg",
        "/mjpegstream.cgi",
        "/videostream.cgi?loginuse=admin&loginpas=",
        "/cgi-bin/mjpg/video.cgi",
        "/axis-cgi/mjpg/video.cgi",
        "/live",
        "/snapshot.jpg"
    };

    // Tworzymy listę wszystkich potencjalnych URL
    QStringList urls;
    for (int port : rtspPorts)
        for (const QString &path : rtspPaths)
            urls << QString("rtsp://%1:%2@%3:%4%5").arg(user, pass, ip).arg(port).arg(path);

    for (int port : httpPorts)
        for (const QString &path : httpPaths)
            urls << (user.isEmpty()
                         ? QString("http://%1:%2%3").arg(ip).arg(port).arg(path)
                         : QString("http://%1:%2@%3:%4%5").arg(user, pass, ip).arg(port).arg(path));

    // Pasek postępu - usuń poprzedni dialog (jeśli z jakiegoś powodu
    // wciąż istnieje) przed utworzeniem nowego, żeby nie kumulować obiektów.
    // Zerujemy pole klasy NAJPIERW (patrz wyjaśnienie przy handlerze finished
    // poniżej - close() może synchronicznie wyemitować canceled()).
    if (progressDialog) {
        QProgressDialog *oldDialog = progressDialog;
        progressDialog = nullptr;
        oldDialog->close();
        oldDialog->deleteLater();
    }
    progressDialog = new QProgressDialog("Skanowanie strumieni...", "Anuluj", 0, urls.size(), this);
    progressDialog->setWindowModality(Qt::ApplicationModal);
    progressDialog->setMinimumDuration(0);
    progressDialog->setValue(0);
    progressDialog->setMinimumWidth(400);

    //    ui->statusbar->showMessage("Skanowanie RTSP i HTTP w toku...");

    // Uruchamiamy skanowanie równolegle
    // Rozłącz stare połączenia watcher-a przed podłączeniem nowych
    disconnect(&watcher, nullptr, this, nullptr);
    disconnect(&watcher, nullptr, progressDialog, nullptr);

    // Ograniczamy liczbę równoczesnych wątków na czas skanowania. Otwieranie
    // wielu cv::VideoCapture (backend FFmpeg) naraz w wielu wątkach jest
    // znanym źródłem niestabilności OpenCV/FFmpeg - przy skanowaniu setek
    // kombinacji port×ścieżka na domyślnym QThreadPool (tyle wątków co
    // rdzeni CPU, wszystkie zajęte równocześnie) dochodziło do crashu w
    // momencie, gdy duża liczba prób open()/release() kończyła się jednocześnie.
    // Zapamiętujemy oryginalny limit, żeby przywrócić go po zakończeniu
    // skanowania - nie chcemy na trwałe ograniczać innych operacji QtConcurrent
    // w aplikacji (np. otwierania strumienia w buttonPlay_clicked).
    QThreadPool *pool = QThreadPool::globalInstance();
    const int originalMaxThreadCount = pool->maxThreadCount();
    pool->setMaxThreadCount(4);

    // Właściwe równoległe przetwarzanie
    QFuture<QString> scanFuture = QtConcurrent::mapped(urls, [](const QString &url) -> QString {
        cv::VideoCapture cap(url.toStdString());
        bool ok = cap.isOpened();
        if (ok) cap.release();
        return ok ? url : QString();
    });

    watcher.setFuture(scanFuture);

    connect(&watcher, &QFutureWatcher<QString>::progressRangeChanged,
            progressDialog, &QProgressDialog::setRange);
    connect(&watcher, &QFutureWatcher<QString>::progressValueChanged,
            progressDialog, &QProgressDialog::setValue);
    connect(&watcher, &QFutureWatcher<QString>::finished, this, [this, urls, originalMaxThreadCount]() {
        // Przywracamy oryginalny limit wątków od razu po zakończeniu skanowania.
        QThreadPool::globalInstance()->setMaxThreadCount(originalMaxThreadCount);

        QStringList validStreams;
        for (int i = 0; i < urls.size(); ++i) {
            QString result = watcher.future().resultAt(i);
            if (!result.isEmpty())
                validStreams << result;
        }

        widgetListListaStrumieni->addItems(validStreams);

        // progressDialog->close() może synchronicznie wyemitować canceled()
        // (Qt robi to przy programowym zamykaniu modalnego QDialog), co
        // odpalałoby handler connect(progressDialog, &QProgressDialog::canceled, ...)
        // PRZED powrotem do tej lambdy - ten handler usuwał progressDialog
        // i zerował wskaźnik, więc kolejna linia tutaj operowała na nullptr
        // albo wołała deleteLater() drugi raz na tym samym obiekcie (crash).
        // Rozwiązanie: zerujemy pole klasy NAJPIERW, operujemy dalej na
        // lokalnej kopii wskaźnika - handler canceled() widzi już nullptr
        // w polu klasy i nic nie robi.
        QProgressDialog *dialogToClose = progressDialog;
        progressDialog = nullptr;
        if (dialogToClose) {
            dialogToClose->close();
            dialogToClose->deleteLater();
        }

        //qDebug()<<;
        //        ui->statusbar->clearMessage();
        QMessageBox::information(this, "Zakończono",QString("Znaleziono %1 działających strumieni.").arg(validStreams.size()));

    });

    connect(progressDialog, &QProgressDialog::canceled, this, [this]() {
        watcher.cancel();
        watcher.waitForFinished();
        //        ui->statusbar->showMessage("Skanowanie przerwane");
        // Jeśli progressDialog wciąż istnieje (czyli ten canceled() przyszedł
        // z normalnego kliknięcia "Anuluj" przez użytkownika, nie z close()
        // wołanego przez handler finished() powyżej), usuwamy go tutaj.
        if (this->progressDialog) {
            QProgressDialog *dialogToClose = this->progressDialog;
            this->progressDialog = nullptr;
            dialogToClose->deleteLater();
        }
    });
}

void FindNewCamera::buttonPlay_clicked()
{
    if (cap.isOpened()) {
        timer.stop();
        cap.release();
    }
    QListWidgetItem *item = widgetListListaStrumieni->currentItem();
    if (!item) {
        QMessageBox::warning(this, "Błąd", "Nie wybrano strumienia!");
        return;
    }
    std::string url = item->text().toStdString();

    // cap.open() na RTSP/HTTP może blokować na minuty, jeśli adres nie
    // odpowiada (czeka na timeout połączenia TCP) - robione synchronicznie
    // na wątku GUI to właśnie zawieszało program przy testowaniu kolejnych
    // adresów IP. Otwieramy strumień w tle (QtConcurrent::run) i przenosimy
    // gotowy uchwyt do `cap` po powrocie na wątek GUI.
    buttonPlay->setEnabled(false);
    buttonPlay->setText("Łączenie...");

    const quint64 myRequestId = ++playRequestId;

    QFuture<cv::VideoCapture> openFuture = QtConcurrent::run([url]() {
        cv::VideoCapture localCap(url);
        return localCap;
    });

    auto *openWatcher = new QFutureWatcher<cv::VideoCapture>(this);
    connect(openWatcher, &QFutureWatcher<cv::VideoCapture>::finished, this, [this, openWatcher, myRequestId]() {
        cv::VideoCapture opened = openWatcher->result();
        openWatcher->deleteLater();

        // Jeśli w międzyczasie użytkownik kliknął Play ponownie (inny stream
        // albo ten sam) - to żądanie jest już nieaktualne. Zwalniamy uchwyt
        // i nie dotykamy `cap`/`timer`, żeby nie nadpisać nowszego połączenia.
        if (myRequestId != playRequestId) {
            if (opened.isOpened()) opened.release();
            return;
        }

        buttonPlay->setEnabled(true);
        buttonPlay->setText("Play");

        if (!opened.isOpened()) {
            QMessageBox::critical(this, "Błąd", "Nie można otworzyć strumienia!");
            return;
        }

        cap = std::move(opened);
        timer.start(33); // ~30 FPS
        buttonZapisz->setEnabled(true);
    });
    openWatcher->setFuture(openFuture);
}

void FindNewCamera::buttonZapisz_cliced()
{
    if (!cap.isOpened()) {
        QMessageBox::warning(this, "Błąd", "Najpierw uruchom podgląd (Play), żeby zapisać kamerę!");
        return;
    }
    if (!widgetListListaStrumieni->currentItem()) {
        QMessageBox::warning(this, "Błąd", "Nie wybrano strumienia z listy!");
        return;
    }

    czytajListaKamer();

    QString nazwaKamery = "";
    bool falsetrue = false;
//    int FileDurationMin = 0;
    int IleDayMax = 0;
    int MinArea = 0;

    QDialog *dialog = new QDialog(this);
    dialog->setMinimumSize(600,300);

    QHBoxLayout *layoutH1 = new QHBoxLayout;
    QLabel *textEditLabel = new QLabel("Nazwa kamery: ");
    QLineEdit *textEdit = new QLineEdit("", dialog);
    textEdit->setPlaceholderText("wpisz nazwę dla kamery");
    layoutH1->addWidget(textEditLabel,3);
    layoutH1->addWidget(textEdit,1);

    QCheckBox *boolCheck = new QCheckBox("Włącz: Detekcja ruchu przy zapisie", dialog);

    QHBoxLayout *layoutH2 = new QHBoxLayout;
    //QLabel *int1SpinLabel = new QLabel("Długość nagrania domyślnie 10 min");
    //QSpinBox *int1Spin = new QSpinBox(dialog);
    //layoutH2->addWidget(int1SpinLabel,3);
    //layoutH2->addWidget(int1Spin,1);
    QLabel *int1SpinLabel = new QLabel("Zapisz do: ");
    QLabel *zapiszDoLineEdit = new QLabel(mainwindow->appHomePath + "/Video/");
    QPushButton *przegladajBtn = new QPushButton("Przeglądaj", dialog);
    layoutH2->addWidget(int1SpinLabel,2);
    layoutH2->addWidget(zapiszDoLineEdit,4);
    layoutH2->addWidget(przegladajBtn, 1);
    connect(przegladajBtn, &QPushButton::clicked, this, [=]()
            {
                QFileDialog dialog(this);
                dialog.setOption(QFileDialog::DontUseNativeDialog, true);
                dialog.setOption(QFileDialog::ShowDirsOnly, true);
                dialog.setFileMode(QFileDialog::Directory);
                dialog.setDirectory(QDir::homePath());

                if(dialog.exec())
                {
                    const QList<QString> selected = dialog.selectedFiles();
                    if (!selected.isEmpty())
                        zapiszDoLineEdit->setText(selected.first() + "/Video/");
                }
            });

    QHBoxLayout *layoutH3 = new QHBoxLayout;
    QLabel *int2SpinLabel = new QLabel("Ile dni przechowywać nagrania ?");
    QSpinBox *int2Spin = new QSpinBox(dialog);
    layoutH3->addWidget(int2SpinLabel,3);
    layoutH3->addWidget(int2Spin,1);

    QHBoxLayout *layoutH4 = new QHBoxLayout;
    QLabel *comboBoxLabel = new QLabel("Określ wielkość przedmiotu przy detekcji ruchu");
    QComboBox *comboBox = new QComboBox(dialog);
    layoutH4->addWidget(comboBoxLabel,3);
    layoutH4->addWidget(comboBox,1);

    QPushButton *okButton = new QPushButton("OK", dialog);

    //int1Spin->setRange(1, 60);
    //int1Spin->setValue(10);
    int2Spin->setRange(1, 14);
    int2Spin->setValue(2);
    comboBox->setStyleSheet(
        "QComboBox {"
        "    background-color: #FFFFFF;"
        "    color: #000000;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #D0D0D0;"
        "    color: #000000;"
        "}"
        );
    comboBox->insertItem(0,"Mały");
    comboBox->insertItem(1,"Średni");
    comboBox->insertItem(2,"Duży");
    boolCheck->setCheckState(Qt::CheckState::Checked);

    connect(okButton, &QPushButton::clicked, dialog, &QDialog::accept);

    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->addLayout(layoutH1);
    layout->addWidget(boolCheck);
    layout->addLayout(layoutH2);
    layout->addLayout(layoutH3);
    layout->addLayout(layoutH4);;
    layout->addStretch(0);
    layout->addWidget(okButton);

    // --- Modalne wyświetlenie, czekamy aż user kliknie OK
    if (dialog->exec() == QDialog::Accepted)
    {
        nazwaKamery = textEdit->text();
        falsetrue = boolCheck->isChecked();
//        FileDurationMin = int1Spin->value();
        IleDayMax = int2Spin->value();
        int dl = cap.get(cv::CAP_PROP_FRAME_WIDTH);
        int hl = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
        int area = dl*hl;
        if(comboBox->currentIndex() == 0){
            MinArea = area/100;
        }else if(comboBox->currentIndex() == 1)
        {
            MinArea = area/50;
        }else if(comboBox->currentIndex() == 2)
        {
            MinArea = area/30;
        }
        QString nazwaKameryWbazie = "";
        for (int row = 0; row < ItemModel->rowCount(); ++row) {
            QModelIndex index;
            index = ItemModel->index(row,1,QModelIndex());
            nazwaKameryWbazie = ItemModel->data(index).toString();
            qDebug()<< ItemModel->rowCount()<<ItemModel->columnCount()<<nazwaKameryWbazie << nazwaKamery;
            if(nazwaKameryWbazie == nazwaKamery){
                QMessageBox::information(this,"UWAGA","Kamera o tej nazwie już istnieje\n Wprowadź inną nazwę");
                return;
            }
            if( nazwaKamery.trimmed().isEmpty()){
                QMessageBox::information(this,"UWAGA","Wprowadź poprawną nazwę");
                return;
            }
        }
        qDebug() << area << area/100 << area/50 << area/30 <<falsetrue;

        kamera.clear();
    //    listaKamer.clear();
        kamera.append(QString::number(ileWierszy));
        kamera.append(nazwaKamery);
        kamera.append(widgetListListaStrumieni->currentItem()->text());
        qDebug()<<"text " << widgetListListaStrumieni->currentItem()->text();
        kamera.append("Size:"+QString::number(cap.get(cv::CAP_PROP_FRAME_WIDTH))+"x"+QString::number(cap.get(cv::CAP_PROP_FRAME_HEIGHT)));
        kamera.append("FPS:"+QString::number(cap.get(cv::CAP_PROP_FPS))+"/s");
        kamera.append(zapiszDoLineEdit->text()+textEdit->text().trimmed()+"/");  //(QString::number(FileDurationMin));
        kamera.append(QString::number(IleDayMax));
        kamera.append(QString::number(MinArea));
        kamera.append(falsetrue ? "true" : "false");
    //    listaKamer.append(kamera);

        // for(int i = 0; i<listaKamer.size();i++){
        //     int newRow = ItemModel->rowCount();
        //     for(int j = 0; j < listaKamer[i].size(); j++){
        //         QString string = listaKamer[i][j];
        //         QStandardItem * item =new QStandardItem();
        //         item->setText(string);
        //         item->setTextAlignment(Qt::AlignCenter);
        //         ItemModel->setItem(newRow,j,item);
        //         qDebug()<<j<< string;
        //     }
        // }
        for(int i = 0; i<kamera.size();i++){
            QString string = kamera[i];
            QStandardItem * item =new QStandardItem();
            item->setText(string);
            item->setTextAlignment(Qt::AlignCenter);
            ItemModel->setItem(ileWierszy,i,item);
            qDebug()<<i<< string;
        }
        zapiszListaKamer();
    }
}

void FindNewCamera::updateFrame()
{
    cv::Mat frame , prevGray,blurImg, diff, thresh;
    if (!cap.read(frame)) return;

    cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
    QImage img((uchar*)frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888);
    img = img.scaled(labelPlay->size());//labelPlay->width(),labelPlay->height());
    labelPlay->setPixmap(QPixmap::fromImage(img).scaled(labelPlay->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void FindNewCamera::zapiszListaKamer()
{
    if (!mainwindow) {
        QMessageBox::warning(this, "Błąd", "Nie można zapisać - brak referencji do okna głównego");
        return;
    }
    QString path = (mainwindow->appHomePath+"/");//QDir::homePath()+"/CameraDir/";
    path = path.simplified();
    path.remove(" ");
    //path = path.trimmed();
    QDir dir;
    if (!dir.exists(path))
        dir.mkpath(path);
    QFile file(path+"kamery.dat");
    if (file.open(QIODevice::WriteOnly))
    {
        QDataStream stream(&file);
        stream.setVersion(QDataStream::Qt_6_0);
        qint32 n = ItemModel->rowCount();
        qint32 m = ItemModel->columnCount();
        stream << n << m;
        for (int i=0; i<n; ++i)
        {
            for (int j=0; j<m; j++)
            {
                // Zapisujemy tylko tekst jako QString - nie cały QStandardItem
                // QStandardItem::write() zapisuje też TextAlignmentRole
                // (QFlags<Qt::AlignmentFlag>) który nie może być bezpiecznie
                // deserializowany przez QDataStream w innych kontekstach.
                QString tekst = ItemModel->item(i,j)
                                ? ItemModel->item(i,j)->text()
                                : QString();
                stream << tekst;
            }
        }
        file.close();
        QMessageBox::information(this, "INFO", "Lista kamer zapisana");
    }else{
        QMessageBox::information(this, "INFO", "Lista kamer nie zapisana");
    }
}

void FindNewCamera::czytajListaKamer()
{
    if (!mainwindow) {
        ileWierszy = 0;
        return;
    }
    if (!ItemModel)
        ItemModel = new QStandardItemModel(this);
    else
        ItemModel->clear();
    //ItemModel = new QStandardItemModel();

    QString path = mainwindow->appHomePath+"/";// QDir::homePath()+"/CameraDir/";
    path = path.simplified();
    path.remove(" ");

    QFile file(path+"kamery.dat");
    if (file.open(QIODevice::ReadOnly))
    {
        QDataStream stream(&file);
        stream.setVersion(QDataStream::Qt_6_0);
        qint32 n, m;
        stream >> n >> m;
        ItemModel->setRowCount(n);
        ItemModel->setColumnCount(m);

        for (int i = 0; i < n ; ++i) {
            for (int j = 0; j < m; j++) {
                QString tekst;
                stream >> tekst;
                QStandardItem *item = new QStandardItem(tekst);
                item->setTextAlignment(Qt::AlignCenter);
                ItemModel->setItem(i, j, item);
            }
        }
        file.close();
        ileWierszy = ItemModel->rowCount();
    }else{
        ileWierszy = 0;
    }
}
