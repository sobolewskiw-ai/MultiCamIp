#include "mainwindow.h"
#include "findnewcamera.h"
#include "httpserwer.h"
#include "mediamtxmanager.h"
#include "ffmpegplayer.h"
#include <QDebug>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
#include <QPushButton>
#include <QGroupBox>
#include <QDir>
#include <QMessageBox>
#include <QPainter>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTableWidget>
#include <QHeaderView>
#include <QStackedWidget>
#include <QHostInfo>
#include <QTcpSocket>
#include <QActionGroup>
#include <QComboBox>
#include <QSpinBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QPointer>
#include <QInputDialog>
#include <QLineEdit>
#include <QSettings>
#include <QUrl>
#include <QGuiApplication>
#include <QClipboard>
#include <QToolTip>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    centralWidget(nullptr),
    drawerWidgetSerwer(nullptr),
    drawerWidgetPodglad(nullptr),
    drawerWidgetNagrania(nullptr)
    //toolbar(nullptr)
{
    appHomePath = QDir::homePath()+"/AppMultiCam";
    mtx = new MediaMTXManager(this);
    connect(mtx, &MediaMTXManager::urlMtxGotowe,this,
        [this]( const QVector<QString> &wynik){
        qDebug() << "Wersja :" << wynik[0];
        qDebug() << "Plik    :" << wynik[1];
        qDebug() << "URL     :" << wynik[2];
        mtx->pobieramMtxmanager(wynik[2],appHomePath+"/mediamtx/"+wynik[1]);
    });
    QString kameradata = appHomePath+"/kamery.dat";
    fileWatcher.addPath(kameradata);
    qDebug()<< fileWatcher.files();
    connect(&fileWatcher, &QFileSystemWatcher::fileChanged, this,
        [this,kameradata](){
        qDebug()<< fileWatcher.files();
        mtx->stopMtx();
        menuListPodglad->clear();
        widgetVectr.clear();
        widgetLayutVector.clear();
        itemVector.clear();
        liczba = 0;
        for(int x =0; x < playerVector.size(); x++){
            // KRYTYCZNA POPRAWKA (null-deref): playerVector bywa celowo
            // dopełniany nullptr-ami (rezerwacja slotu przed utworzeniem
            // realnego FfmpegPlayer) - wywołanie ->stop() bez sprawdzenia
            // powodowało crash.
            if (playerVector[x])
                playerVector[x]->stop();
        }
        mtx->startMtx();
        if (!fileWatcher.files().contains(kameradata))
            fileWatcher.addPath(kameradata);
    });

    stylesheetPushButton =
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
    stylesheetPushButtonRed =
        "QPushButton {"
        "   background: none;"
        "   border: 2px solid #0078D7;"
        "   border-radius: 6px;"
        "   background-color: #FF0000;"
        "   color: white;"
        "   padding: 6px;"
        "   font-weight: bold;"
        "   font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #F08080;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #8B0000;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #FFFFFF;"
        "}";
    stylesheetLabelSelectedBlue =
        "QLabel {"
        " border: 2px solid #0078D7;"
        " border-radius: 3px;"
        " padding: 4px;"
        " background-color: #3399FF;"
        " color: white;"
        " font-size: 20px;"
        " font-weight: bold;"
        "}";
    stylesheetListWidgetBlue =
        "QListWidget {border: 2px solid #0078D7;"
        "border-radius: 3px;"
        "padding: 4px;"
        " color: #0078D7;"
        "background-color: #F0F8FF;"
        //"font-weight: bold;"
        "font-size: 20px;}"
        "QListWidget::item:selected {"
        " background-color: #3399FF;"
        " color: white;}";
    stylesheetSliderBlue =
        "QSlider::groove:horizontal {border: 1px solid #3399FF;"
        "height: 6px;"
        "background: #3399FF;"
        "border-radius: 3px;"
        "}"
        "QSlider::sub-page:horizontal {background: #3399FF;"
        "border-radius: 3px;"
        "}"
        "QSlider::add-page:horizontal {background: #D0E8FF;"
        "border-radius: 3px;"
        "}"
        "QSlider::handle:horizontal {background: #3399FF;"
        "border: 2px solid white;"
        "width: 12px;"
        "margin: -6px 0;"
        "border-radius: 9px;"
        "}"
        "QSlider::handle:horizontal:hover { background: #0078D7;"
        "}"
        "QSlider::handle:horizontal:pressed {background: #005A9E;"
        "}";
    stylesheetComboBox = R"(
QComboBox {
    background-color: white;
    color: #003366;
    border: 2px solid #3399FF;
    border-radius: 6px;
    padding: 5px 10px;
    font-size: 18px;
    selection-background-color: #BDE8FF;
}

QComboBox::drop-down {
    width: 42px;
    border: none;
    border-left: 1px solid #99CCEE;
    background-color: #DFF2FF;
}

QComboBox::down-arrow {
    image: none;
    width: 0px;
    height: 0px;
    border-left: 7px solid transparent;
    border-right: 7px solid transparent;
    border-top: 8px solid #0078D7;
}
)";
    stylesheetTable = R"(
QTableWidget {
    background-color: white;
    alternate-background-color: #F5F5F5;
    color: black;
    font-size: 18px;
    gridline-color: #C8D6E5;
    border: 1px solid #C8D6E5;
}

QTableWidget::item {
    background: white;
    color: black;
    padding: 4px;
}

QTableWidget::item:selected {
    background: #4A90E2;
    color: white;
}

QTableCornerButton::section {
    background: #BDE8FF;
    border: 1px solid #8EC7E8;
}
)";

    setupUi();
}
//MainWindow::~MainWindow() = default;
MainWindow::~MainWindow() {
    // Zatrzymujemy wszystkie playery przed destrukcją
    for (FfmpegPlayer *player : std::as_const(playerVector)) {
        if (player) player->stop();
    }
    playerVector.clear();

    if (mtx) {
        disconnect(mtx, nullptr, this, nullptr);
        mtx->stopMtx();
    }
    if (httpSerwer) {
        httpSerwer->stop();
    }
    qDebug() << "ZAMYKAM PROGRAM";
}

void MainWindow::setupUi()
{
    centralWidget = new QWidget(this);
    rootLayout = new QHBoxLayout(centralWidget);
    rootLayout->setAlignment(Qt::AlignLeft);
    rootLayout->setContentsMargins(0,0,0,0);
    rootLayout->setSpacing(0);

    centralLabel = new QLabel(
        "Kliknij&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"
        "<span style=\"font-size:16pt; color:blue; font-weight:bold;\">☰ Menu</span> "
        "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;w pasku narzędzi, aby otworzyć wysuwany panel.",
        this
    );
    centralLabel->setAlignment(Qt::AlignCenter);
    drawerWidgetSerwer = new QWidget(this);
    drawerWidgetSerwer->setStyleSheet("background:#F5F5F5; border-right:2px solid blue; border-left:2px solid blue; border-bottom:2px solid blue; border-top:2px solid blue;");
    drawerWidgetSerwer->setFixedWidth(0);
    drawerWidgetPodglad = new QWidget(this);
    drawerWidgetPodglad->setStyleSheet("background:#F5F5F5; border-right:2px solid blue; border-left:2px solid blue; border-bottom:2px solid blue; border-top:2px solid blue;");
    drawerWidgetPodglad->setFixedWidth(0);
    drawerWidgetNagrania = new QWidget(this);
    drawerWidgetNagrania->setStyleSheet("background:#F5F5F5; border-right:2px solid blue; border-left:2px solid blue; border-bottom:2px solid blue; border-top:2px solid blue;");
    drawerWidgetNagrania->setFixedWidth(0);


    rootLayout->addWidget(drawerWidgetSerwer);
    rootLayout->addWidget(drawerWidgetPodglad);
    rootLayout->addWidget(drawerWidgetNagrania);
    rootLayout->addWidget(centralLabel);
    setCentralWidget(centralWidget);

    toolbar = addToolBar("Main");

    // QLabel *przerwa = new QLabel(this);
    // przerwa->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    // toolbar->addWidget(przerwa);

//PANEL BOCZNY SERWER
    QAction *toggleActionSerwer = toolbar->addAction("☰ SERWER");
    toggleActionSerwer->setCheckable(true);
    QLabel *przerwa = new QLabel(this);
    przerwa->setFixedWidth(10);
    toolbar->addWidget(przerwa);
    QToolButton *toolButtonSerwer = qobject_cast<QToolButton*>(toolbar->widgetForAction(toggleActionSerwer));
    toolButtonSerwer->setFocusPolicy(Qt::StrongFocus);  //tutaj
    toolButtonSerwer->setFixedWidth(200);
    toolButtonSerwer->setStyleSheet(
        "QToolButton {"
        "    border: 1px solid black;"
        "    border-radius: 3px;"
        "    padding: 2px;"
        "}"
        "QToolButton:hover {"
        "    border: 1px solid gray;"
        " background-color: #3399FF;"
        " color: white;"
        "}"
        "QToolButton:checked {"
        "    border: 1px solid gray;"
        "    background-color: #3399FF;"
        "    color: white;"
        "}"
        );

    QFont font;
    font.setPointSize(16);
    font.setBold(true);
    QPalette pal;  // = toolButtonSerwer->palette();
    pal.setColor(QPalette::ButtonText, Qt::blue);
    if (toolButtonSerwer) {
        // 3. Ustawiam czcionkę
        // QFont font;// = toolButtonSerwer->font();
        // font.setPointSize(16);  // zmień na dowolny rozmiar w punktach
        // font.setBold(true);     // opcjonalnie pogrubienie
        toolButtonSerwer->setFont(font);
        // 4. Opcjonalnie zmiana koloru tekstu
        // QPalette pal = toolButtonSerwer->palette();
        // pal.setColor(QPalette::ButtonText, Qt::blue);
        toolButtonSerwer->setPalette(pal);
    }
    connect(toggleActionSerwer, &QAction::triggered, this,[this,toolButtonSerwer](){
        ukryjPokazPanelSerwer();
        if(livePodgladWidget){
        livePodgladWidget->hide();
        centralLabel->show();
        toolButtonSerwer->setFocus();   //tutaj
        }
    });
    QVBoxLayout *layoutSerwer = new QVBoxLayout(drawerWidgetSerwer);
    layoutSerwer->setContentsMargins(8,8,8,8);
    layoutSerwer->setSpacing(8);

    QLabel *titleSerwer = new QLabel("<b>SERWER</b>", drawerWidgetSerwer);
    titleSerwer->setStyleSheet(stylesheetLabelSelectedBlue);
    titleSerwer->setAlignment(Qt::AlignCenter);
    layoutSerwer->addWidget(titleSerwer);

    menuListSerwer = new QListWidget(drawerWidgetSerwer);
    menuListSerwer->setStyleSheet(stylesheetListWidgetBlue);
    menuListSerwer->setIconSize(QSize(40, 40));
    QFont itemfont;
    itemfont.setBold(true);
    itemfont.setPointSize(12);
    //Add item1
    QListWidgetItem *item1 = new QListWidgetItem("START SERWER RTSP I HTTP");
    item1->setFont(itemfont);
    item1->setIcon(QIcon(":/icons/httpstart.png"));
    item1->setData(Qt::UserRole, "StartStop");
    menuListSerwer->addItem(item1);
    //Add item2
    QListWidgetItem *item2 = new QListWidgetItem("SZUKAJ KAMER PO ADRESIE IP");
    item2->setFont(itemfont);
    item2->setIcon(QIcon(":/icons/szukaj.png"));
    item2->setData(Qt::UserRole, "szukaj kamer");
    menuListSerwer->addItem(item2);
    //Add item3
    QListWidgetItem *item3 = new QListWidgetItem("USTAWIENIA KAMER");
    item3->setFont(itemfont);
    item3->setIcon(QIcon(":/icons/szukaj.png"));
    item3->setData(Qt::UserRole, "USTAWIENIA");
    menuListSerwer->addItem(item3);
    //Add item4
    QListWidgetItem *item4 = new QListWidgetItem("TOKEN HTTP");
    item4->setFont(itemfont);
    item4->setIcon(QIcon(":/icons/token.svg"));
    item4->setData(Qt::UserRole, "TOKEN");
    menuListSerwer->addItem(item4);
    //Add item5
    QListWidgetItem *item5 = new QListWidgetItem("DODAJ IKONĘ DO PULPITU");
    item5->setFont(itemfont);
    item5->setIcon(QIcon(":/icons/dodajdopulpitu.png"));
    item5->setData(Qt::UserRole, "IKONAPULPITU");
    menuListSerwer->addItem(item5);

    layoutSerwer->addWidget(menuListSerwer);
    menuListSerwer->setFocus();
    menuListSerwer->setCurrentItem(item1);
    connect(menuListSerwer, &QListWidget::itemClicked, this, &MainWindow::onMenuItemSerwerClicked);
    connect(menuListSerwer, &QListWidget::itemActivated, this, &MainWindow::onMenuItemSerwerClicked);

    QPushButton *closeBtnSerwer = new QPushButton("Ukryj", drawerWidgetSerwer);
    closeBtnSerwer->setIcon(QIcon(":/icons/ukryj.svg"));
    closeBtnSerwer->setIconSize(QSize(32,32));
    closeBtnSerwer->setStyleSheet(stylesheetPushButton);
    connect(closeBtnSerwer, &QPushButton::clicked, this, &MainWindow::ukryjPokazPanelSerwer);
    //layoutSerwer->addStretch(1);
    layoutSerwer->addWidget(closeBtnSerwer);

//PANEL BOCZNY PODGLĄD
    QAction *toggleActionPodglad = toolbar->addAction("☰ PODGLĄD");
    toggleActionPodglad->setCheckable(true);
    QLabel *przerwa2 = new QLabel(this);
    przerwa2->setFixedWidth(10);
    toolbar->addWidget(przerwa2);
    QToolButton *toolButtonPodglad = qobject_cast<QToolButton*>(toolbar->widgetForAction(toggleActionPodglad));
    toolButtonPodglad->setFocusPolicy(Qt::StrongFocus);  //tutaj
    toolButtonPodglad->setFixedWidth(200);
    toolButtonPodglad->setStyleSheet(
        "QToolButton {"
        "    border: 1px solid black;"
        "    border-radius: 3px;"
        "    padding: 2px;"
        "}"
        "QToolButton:hover {"
        "    border: 1px solid gray;"
        " background-color: #3399FF;"
        " color: white;"
        "}"
        "QToolButton:checked {"
        "    border: 1px solid gray;"
        "    background-color: #3399FF;"
        "    color: white;"
        "}"
        );
    if (toolButtonPodglad) {
        // 3. Ustawiam czcionkę
        // QFont font = toolButtonPodglad->font();
        // font.setPointSize(16);  // zmień na dowolny rozmiar w punktach
        // font.setBold(true);     // opcjonalnie pogrubienie
        toolButtonPodglad->setFont(font);
        // 4. Opcjonalnie zmiana koloru tekstu
        // QPalette pal = toolButtonPodglad->palette();
        // pal.setColor(QPalette::ButtonText, Qt::blue);
        toolButtonPodglad->setPalette(pal);
    }
    connect(toggleActionPodglad, &QAction::triggered, this,[this,toolButtonPodglad](){
        ukryjPokazPanelPodglad();
        if(livePodgladWidget){
            centralLabel->hide();
            livePodgladWidget->show();
            toolButtonPodglad->setFocus();  //tutaj
        }
    });

    QVBoxLayout *layoutPodglad = new QVBoxLayout(drawerWidgetPodglad);
    layoutPodglad->setContentsMargins(8,8,8,8);
    layoutPodglad->setSpacing(8);

    QLabel *titlePodglad = new QLabel("PODGLĄD", drawerWidgetPodglad);
    titlePodglad->setStyleSheet(stylesheetLabelSelectedBlue);
    titlePodglad->setAlignment(Qt::AlignCenter);
    layoutPodglad->addWidget(titlePodglad);

    menuListPodglad = new QListWidget(drawerWidgetPodglad);
    menuListPodglad->setStyleSheet(stylesheetListWidgetBlue);
    menuListPodglad->setIconSize(QSize(40, 40));
    layoutPodglad->addWidget(menuListPodglad);
    connect(menuListPodglad, &QListWidget::itemClicked,this, &MainWindow::onMenuItemPodgladClicked);

    //Add widget
//    createWidgetListaLivekamery();

    QPushButton *btnSerweryLiveStream = new QPushButton("LIVE SERWERY",drawerWidgetPodglad);
    btnSerweryLiveStream->setIcon(QIcon(":/icons/liveserwery.svg"));
    btnSerweryLiveStream->setIconSize(QSize(32,32));
    btnSerweryLiveStream->setStyleSheet(stylesheetPushButton + "QPushButton { font-size: 24px; }");
    btnSerweryLiveStream->setMinimumHeight(50);
    layoutPodglad->addWidget(btnSerweryLiveStream);
    connect(btnSerweryLiveStream, &QPushButton::clicked, this, [this,font](){
        // menuListPodglad->clear();
        // widgetVectr.clear();
        // widgetLayutVector.clear();
        // itemVector.clear();

        QDialog *dialog = new QDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowTitle("SERWERY LIVE STREAM");
        dialog->resize(900, 500);
        QVBoxLayout *layoutDialog = new QVBoxLayout(dialog);

        QStackedWidget *stackedWidget = new QStackedWidget(dialog);
        QWidget *widget1page = new QWidget();
        QVBoxLayout *layoutPage1 = new QVBoxLayout(widget1page);
        table = new QTableWidget(dialog);
        table->setColumnCount(4);
        table->setHorizontalHeaderLabels(
            {"Lp.", "Nazwa serwera", "Adres","Status"}
            );
        table->verticalHeader()->setVisible(false);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setAlternatingRowColors(true);
        table->setShowGrid(true);

        table->setStyleSheet(
            "QTableWidget{"
            "background:white;"
            "alternate-background-color:#F4F9FF;"
            "gridline-color:#C8D6E5;"
            "selection-background-color:#4A90E2;"
            "selection-color:white;"
            "font-size:12px;"
            "}"
            );

        QHeaderView *header = table->horizontalHeader();
        header->setFixedHeight(40);
        header->setDefaultAlignment(Qt::AlignCenter);
        header->setStyleSheet(
            "QHeaderView::section{"
            "background:#BDE8FF;"
            "color:#003366;"
            "font-weight:bold;"
            "font-size:20px;"
            "border:1px solid #8EC7E8;"
            "padding:6px;"
            "}"
            );
    //    ItemModelSerweryDat->setHorizontalHeaderLabels(header);
        header->setSectionResizeMode(0,QHeaderView::Fixed);
        table->setColumnWidth(0,60);
        header->setSectionResizeMode(1,QHeaderView::Stretch);
        header->setSectionResizeMode(2,QHeaderView::Stretch);
        header->setSectionResizeMode(3,QHeaderView::Fixed);
        table->setColumnWidth(3,100);

        czytajSerweryDat();
        table->clearContents();              // usuń stare komórki
        table->setRowCount(0);             // wyczyść wiersze
        table->setRowCount(ItemModelSerweryDat->rowCount());
        //table->setColumnCount(ItemModelSerweryDat->columnCount());

        QFont font = table->font();
        font.setPixelSize(20);

        for(int row = 0; row < ItemModelSerweryDat->rowCount(); row++){
            for(int col = 0; col < ItemModelSerweryDat->columnCount(); col++){
                QString text = ItemModelSerweryDat->item(row,col)
                               ? ItemModelSerweryDat->item(row,col)->text()
                               : QString();
                QTableWidgetItem *item = new QTableWidgetItem(text);
                item->setTextAlignment(Qt::AlignCenter);
                item->setFont(font);
                if(col == 3){
                    item->setTextAlignment(Qt::AlignLeft);
                }
                table->setItem(row,col, item);
            }
        }
        table->selectRow(0);
        table->setFocus();


        QTimer *timerOnline = new QTimer(dialog);
        auto sprawdzSerwery = [this]()
        {
            for(int row = 0; row < table->rowCount(); row++){
                QString adres = table->item(row, 2) ? table->item(row, 2)->text() : QString();
                if (adres.isEmpty()) continue;

                // POWAŻNA POPRAWKA (use-after-free / crash - głównie w
                // sieci zdalnej): "table" to surowy wskaźnik składowy
                // MainWindow, będący dzieckiem dialogu utworzonego z
                // Qt::WA_DeleteOnClose. Sprawdzenie połączenia
                // (waitForConnected) trwa do 4 sekund NA WĄTKU TŁA - jeśli
                // w tym czasie użytkownik zamknie dialog (np. przyciskiem
                // Anuluj), "table" zostaje zniszczone, ZANIM wątek tła
                // skończy czekać. Przekazanie wtedy tego już-zwisającego
                // wskaźnika do QMetaObject::invokeMethod(table, ...) samo w
                // sobie jest użyciem zwolnionej pamięci. Błąd ujawniał się
                // głównie w sieci zdalnej (wolniejsze połączenia TCP =
                // większe okno czasowe na ten wyścig) i tylko "co któryś
                // raz" - potwierdzone i przetestowane wcześniej.
                //
                // QPointer<QTableWidget> automatycznie zeruje się, gdy
                // wskazywany obiekt zostanie zniszczony. Kontekstem
                // invokeMethod jest `this` (MainWindow) - obiekt żyjący
                // przez cały czas działania aplikacji, a NIE `table`,
                // które mogłoby już nie istnieć.
                QPointer<QTableWidget> tableGuard(table);

                // Sprawdzamy w tle żeby nie blokować GUI przy niedostępnych hostach
                // POPRAWKA (fałszywe "Offline" dla hostname'ów wymagających
                // DNS, np. dynamiczne DNS typu "sobol.duckdns.org"):
                // waitForConnected() obejmuje ZARÓWNO rozwiązanie nazwy DNS,
                // JAK I sam handshake TCP - to nie jest tylko czas
                // połączenia. Dla hostname'u (w odróżnieniu od gołego IP)
                // samo zapytanie DNS (zwłaszcza gdy nie jest jeszcze w
                // lokalnym cache, co jest typowe dla usług dynamicznego DNS)
                // potrafi zająć kilkaset ms, zostawiając za mało czasu na
                // resztę w budżecie 1000ms - serwer bywał więc błędnie
                // pokazywany jako Offline mimo że faktycznie działał.
                // Zwiększamy do 4000ms; to i tak działa w tle (osobny wątek
                // z puli), więc nie blokuje GUI ani pozostałych sprawdzeń.
                QThreadPool::globalInstance()->start([this, row, adres, tableGuard](){
                    QTcpSocket socket;
                    socket.connectToHost(adres, 8554);
                    bool online = socket.waitForConnected(4000);
                    // POPRAWKA (diagnostyka): sam wynik true/false nic nie
                    // mówi DLACZEGO połączenie się nie udało - a przyczyn
                    // może być kilka zupełnie różnych (błędny DNS, port
                    // zablokowany/niezaporwardowany, "hairpin NAT" - typowy
                    // problem domowych routerów, gdzie własny publiczny
                    // adres/DDNS nie jest osiągalny z tej samej sieci
                    // lokalnej mimo że z zewnątrz działa poprawnie, albo
                    // faktyczny brak usługi). Logujemy kod błędu Qt oraz
                    // (dla hostname'ów) czy DNS w ogóle się rozwiązuje, żeby
                    // dało się to precyzyjnie zdiagnozować zamiast zgadywać.
                    if (!online) {
                        qDebug() << "sprawdzSerwery: BRAK POŁĄCZENIA z" << adres
                                 << "błąd Qt:" << socket.error()
                                 << socket.errorString();
                        QHostInfo info = QHostInfo::fromName(adres);
                        if (info.error() != QHostInfo::NoError) {
                            qDebug() << "sprawdzSerwery: DNS NIE ROZWIĄZUJE SIĘ dla"
                                     << adres << "-" << info.errorString();
                        } else {
                            qDebug() << "sprawdzSerwery: DNS OK dla" << adres
                                     << "-> adresy IP:" << info.addresses();
                        }
                    }
                    socket.disconnectFromHost();
                    if (!tableGuard)
                        return; // dialog (i table) już nie istnieje
                    // Aktualizacja UI musi być na głównym wątku
                    QMetaObject::invokeMethod(this, [tableGuard, row, online](){
                        if (!tableGuard)
                            return;
                        if (row < tableGuard->rowCount() && tableGuard->item(row, 3))
                            tableGuard->item(row, 3)->setText(online ? "🟢 Online" : "🔴 Offline");
                    }, Qt::QueuedConnection);
                });
            }
        };

        connect(timerOnline, &QTimer::timeout, dialog, [sprawdzSerwery](){
            sprawdzSerwery();
        });
        sprawdzSerwery();
        timerOnline->start(5000); // co 5s zamiast 1s - sprawdzanie dostępności nie musi być co sekundę

        QHBoxLayout *h1layout = new QHBoxLayout();
        QPushButton *btnDodaj      = new QPushButton("➕ Dodaj");
        QPushButton *btnUsun       = new QPushButton("🗑 Usuń");
        QPushButton *btnModyfikuj  = new QPushButton("✏ Modyfikuj");
        QPushButton *btnPolacz     = new QPushButton("🟢 Połącz");
        QPushButton *btnRozlacz     = new QPushButton("🔴 Rozłącz");
        QPushButton *btnZapisz     = new QPushButton("💾 Zapisz");
        QPushButton *btnAnuluj     = new QPushButton("✖ Zamknij");

        btnDodaj->setStyleSheet(stylesheetPushButton);
        btnUsun->setStyleSheet(stylesheetPushButtonRed);
        btnModyfikuj->setStyleSheet(stylesheetPushButton);
        btnPolacz->setStyleSheet(stylesheetPushButton);
        btnRozlacz->setStyleSheet(stylesheetPushButton);
        btnZapisz->setStyleSheet(stylesheetPushButton);
        btnAnuluj->setStyleSheet(stylesheetPushButtonRed);
        QList<QPushButton*> buttons =
        {
            btnDodaj,
            btnUsun,
            btnModyfikuj,
            btnPolacz,
            btnRozlacz,
            btnZapisz,
            btnAnuluj
        };

        for(QPushButton *b : buttons)
            b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        h1layout->addWidget(btnDodaj,1);
        h1layout->addWidget(btnModyfikuj,1);
        h1layout->addWidget(btnUsun,1);
        h1layout->addWidget(btnZapisz,1);
        h1layout->addWidget(btnPolacz,1);
        h1layout->addWidget(btnRozlacz,1);
        h1layout->addStretch(2);
        h1layout->addWidget(btnAnuluj,1);     // dwa razy szerszy

        layoutPage1->addWidget(table);
        layoutPage1->addLayout(h1layout);

        QWidget *widget2page = new QWidget();
        QVBoxLayout *layoutPage2 = new QVBoxLayout(widget2page);

        QHBoxLayout *h1layoutPage2 = new QHBoxLayout();
        QLabel *numer = new QLabel("Id serwera:");
        numer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        numer->setStyleSheet(stylesheetLabelSelectedBlue);
        QLabel *numer2 = new QLabel("");
        numer2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        numer2->setAlignment(Qt::AlignCenter);
        numer2->setStyleSheet(
            "QLabel {"
            "border: 2px solid blue;"
            "border-radius: 4px;"
            "font-size: 18px;"
            "font-weight: bold;"
            "}");
        h1layoutPage2->addStretch(1);
        h1layoutPage2->addWidget(numer,2);
        h1layoutPage2->addWidget(numer2,2);
        h1layoutPage2->addStretch(1);

        QHBoxLayout *h2layoutPage2 = new QHBoxLayout();
        QLabel *labelNazwa = new QLabel("LOKALIZACJA SERWERA");
        labelNazwa->setStyleSheet(stylesheetLabelSelectedBlue);
        labelNazwa->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        QLineEdit *lineEditNazwa = new QLineEdit();
        lineEditNazwa->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        lineEditNazwa->setAlignment(Qt::AlignCenter);
        lineEditNazwa->setStyleSheet(
            "QLineEdit {"
            "border: 2px solid blue;"
            "border-radius: 4px;"
            "font-size: 18px;"
            "font-weight: bold;"
            "}");
        h2layoutPage2->addStretch(1);
        h2layoutPage2->addWidget(labelNazwa,2);
        h2layoutPage2->addWidget(lineEditNazwa,2);
        h2layoutPage2->addStretch(1);

        QHBoxLayout *h3layoutPage2 = new QHBoxLayout();
        QLabel *labelAdres = new QLabel("ADRES IP SERWERA");
        labelAdres->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        labelAdres->setStyleSheet(stylesheetLabelSelectedBlue);
        QLineEdit * lineEditAdres = new QLineEdit();
        lineEditAdres->setAlignment(Qt::AlignCenter);
        lineEditAdres->setStyleSheet(
            "QLineEdit {"
            "border: 2px solid blue;"
            "border-radius: 4px;"
            "font-size: 18px;"
            "font-weight: bold;"
            "}");
        lineEditAdres->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        h3layoutPage2->addStretch(1);
        h3layoutPage2->addWidget(labelAdres,2);
        h3layoutPage2->addWidget(lineEditAdres,2);
        h3layoutPage2->addStretch(1);

        QHBoxLayout *h4layoutPage2 = new QHBoxLayout();
        QPushButton *btnZapiszPage2 = new QPushButton("💾 Zapisz");
        btnZapiszPage2->setStyleSheet(stylesheetPushButton);
        btnZapiszPage2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        QPushButton *btnAnulujPage2 = new QPushButton("✖ ANULUJ");
        btnAnulujPage2->setStyleSheet(stylesheetPushButtonRed);
        btnAnulujPage2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        h4layoutPage2->addWidget(btnZapiszPage2,1);
        h4layoutPage2->addStretch(3);
        h4layoutPage2->addWidget(btnAnulujPage2,1);

        layoutPage2->addStretch(1);
        layoutPage2->addLayout(h1layoutPage2);
        layoutPage2->addLayout(h2layoutPage2);
        layoutPage2->addLayout(h3layoutPage2);
        layoutPage2->addStretch(1);
        layoutPage2->addLayout(h4layoutPage2);

        stackedWidget->addWidget(widget1page);
        stackedWidget->addWidget(widget2page);
        layoutDialog->addWidget(stackedWidget);

 //       QFont font = table->font();
  //      font.setPixelSize(20);

    //    connecty stackedWidget widget2page
        connect(btnAnulujPage2, &QPushButton::clicked,stackedWidget,[stackedWidget,widget1page](){
            stackedWidget->setCurrentWidget(widget1page);
        });
        connect(btnZapiszPage2, &QPushButton::clicked,stackedWidget,[this,font,stackedWidget,
                widget1page,numer2,lineEditNazwa,lineEditAdres](){
            if(lineEditNazwa->text().trimmed().isEmpty() || lineEditAdres->text().trimmed().isEmpty()){
                QMessageBox::information(this,"INFO","WYPEŁNIJ PUSTE POLA");
                return;
            }
            QString adres = lineEditAdres->text().trimmed();
            QRegularExpression ipv4(
                R"(^(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)\.)"
                R"((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)\.)"
                R"((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)\.)"
                R"((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)$)");

            if (ipv4.match(adres).hasMatch())
            {
                qDebug() << "Poprawny adres IPv4:" << adres;
                int row = numer2->text().toInt()-1;
                QString lp = numer2->text();

                if(lp.toInt() > table->rowCount()){
                    table->insertRow(row);
                }
                QVector<QTableWidgetItem*> items;
                items << new QTableWidgetItem(lp)
                      << new QTableWidgetItem(lineEditNazwa->text().trimmed())
                      << new QTableWidgetItem(lineEditAdres->text().trimmed())
                      << new QTableWidgetItem("🔴 Offline");  //"🔴 Offline""🟢 Online"
                for(int x = 0; x < table->columnCount(); x++){
                    items[x]->setFont(font);
                    items[x]->setTextAlignment(Qt::AlignCenter);
                    table->setItem(row,x, items[x]);
                }
                table->selectRow(0);
                table->setFocus();
                stackedWidget->setCurrentWidget(widget1page);
            }
            else if (adres.compare("localhost", Qt::CaseInsensitive) == 0)
            {
                qDebug() << "localhost";
                int row = numer2->text().toInt()-1;
                QString lp = numer2->text();

                if(lp.toInt() > table->rowCount()){
                    table->insertRow(row);
                }
                QVector<QTableWidgetItem*> items;
                items << new QTableWidgetItem(lp)
                      << new QTableWidgetItem(lineEditNazwa->text().trimmed())
                      << new QTableWidgetItem(lineEditAdres->text().trimmed())
                      << new QTableWidgetItem("🔴 Offline");  //"🔴 Offline""🟢 Online"
                for(int x = 0; x < table->columnCount(); x++){
                    items[x]->setFont(font);
                    items[x]->setTextAlignment(Qt::AlignCenter);
                    table->setItem(row,x, items[x]);
                }
                table->selectRow(0);
                table->setFocus();
                stackedWidget->setCurrentWidget(widget1page);
            }else
            {
                QRegularExpression tylkoCyfryKropki(R"(^[0-9.]+$)");
                if (tylkoCyfryKropki.match(adres).hasMatch())
                {
                    QMessageBox::warning(this,"Błąd","Niepoprawny adres IPv4.");
                    return;
                }
                QString firstLabel = adres.section('.', 0, 0);
                bool tylkoCyfry = std::all_of(firstLabel.begin(), firstLabel.end(),
                                              [](QChar c){ return c.isDigit(); });
                if (tylkoCyfry)
                {
                    QMessageBox::warning(this,
                        "Błąd",
                        "Nazwa hosta nie może zaczynać się od samych cyfr.");
                    return;
                }
            // sprawdzamy nazwę hosta lub domenę
                qDebug()<< "adres" << adres;
            QHostInfo::lookupHost(adres, this,
                [this,numer2,font,lineEditNazwa,lineEditAdres,stackedWidget,widget1page](const QHostInfo &info){
                if (info.error() != QHostInfo::NoError ||info.addresses().isEmpty()){
                    QMessageBox::warning(this,"Błąd","Wprowadź poprawny adres IP lub nazwę hosta.");
                    return;
                }
                qDebug() << "Poprawna nazwa hosta.";

                for (const QHostAddress &ip : info.addresses()){
                    qDebug() << ip.toString();
                }
                int row = numer2->text().toInt()-1;
                QString lp = numer2->text();

                if(lp.toInt() > table->rowCount()){
                    table->insertRow(row);
                }
                QVector<QTableWidgetItem*> items;
                items << new QTableWidgetItem(lp)
                      << new QTableWidgetItem(lineEditNazwa->text().trimmed())
                      << new QTableWidgetItem(lineEditAdres->text().trimmed())
                      << new QTableWidgetItem("🔴 Offline");  //"🔴 Offline""🟢 Online"
                for(int x = 0; x < table->columnCount(); x++){
                    items[x]->setFont(font);
                    items[x]->setTextAlignment(Qt::AlignCenter);
                    table->setItem(row,x, items[x]);
                }
                table->selectRow(0);
                table->setFocus();
                stackedWidget->setCurrentWidget(widget1page);
                });
            }
        });

    //    connecty stackedWidget widget1page
        connect(btnDodaj, &QPushButton::clicked, dialog,[this,stackedWidget,widget2page,
                numer2,lineEditNazwa, lineEditAdres](){
            stackedWidget->setCurrentWidget(widget2page);
            numer2->setText(QString::number(table->rowCount()+1));
            lineEditNazwa->setText("");
            lineEditNazwa->setFocus();
            lineEditAdres->setText("");
        });
        connect(btnModyfikuj, &QPushButton::clicked,dialog,[this,stackedWidget,
                                    widget2page,numer2,lineEditNazwa,lineEditAdres](){
            int row = table->currentRow();
            if(row == -1){
                QMessageBox::information(nullptr,"INFO","WYBIERZ WIERSZ");
                return;
            }else{
            numer2->setText(table->item(row,0) ? table->item(row,0)->text() : QString());
            lineEditNazwa->setText(table->item(row,1) ? table->item(row,1)->text() : QString());
            lineEditAdres->setText(table->item(row,2) ? table->item(row,2)->text() : QString());
            stackedWidget->setCurrentWidget(widget2page);
            }
        });
        connect(btnUsun, &QPushButton::clicked, dialog,[this,font,timerOnline](){
            timerOnline->stop();
            int row = table->currentRow();
            table->removeRow(row);
            for(int x = 0; x < table->rowCount(); x++){
                if (table->item(x,0))
                    table->item(x,0)->setText(QString::number(x+1));
            }
            timerOnline->start();
        });
        connect(btnZapisz, &QPushButton::clicked, dialog,[this](){

            if(!ItemModelSerweryDat){
                ItemModelSerweryDat = new QStandardItemModel();
            }else{
                ItemModelSerweryDat->clear();
            }
            ItemModelSerweryDat->setRowCount(table->rowCount());
            ItemModelSerweryDat->setColumnCount(table->columnCount());
            for(int row = 0; row < table->rowCount(); row++){
                for(int col = 0; col < table->columnCount(); col++){
                    QString text = table->item(row, col)
                                   ? table->item(row, col)->text()
                                   : QString();
                    QStandardItem *item = new QStandardItem(text);
                    ItemModelSerweryDat->setItem(row,col,item);
                }
            }
           zapiszSerweryDat();
           ItemModelSerweryDat->clear();
           ItemModelSerweryDat->deleteLater();
           ItemModelSerweryDat = nullptr;

        //   createWidgetListaLivekamery();
        });
        connect(btnPolacz, &QPushButton::clicked, dialog, [this](){
            int row = table->currentRow();
            QString adres = table->item(row, 2)
                                ? table->item(row, 2)->text()
                                :QString();
            QString status = table->item(row, 3)
                            ? table->item(row, 3)->text()
                            :QString();
            if(status == "🟢 Online"){
            qDebug()<<status;
            qDebug() << "przed createWidgetListaLivekamery";
            createWidgetListaLivekamery();
            qDebug() << "po createWidgetListaLivekamery";
            }else{
                QMessageBox::information(nullptr, "UWAGA", "NIE MOŻNA POŁĄCZYĆ Z"
                " SERWEREM:\n"+adres+"\n1) sprawdź internet\n2) uruchom serwer na "+adres+"\n3) na routerze przekieruj porty\n 8554 i 8080 do "+adres);
            }
        });
        connect(btnRozlacz, &QPushButton::clicked, dialog, [this](){
            menuListPodglad->clear();
            qDebug()<< "btnRozlacz test 1";
            widgetVectr.clear();
            widgetLayutVector.clear();
            itemVector.clear();
            qDebug()<< "btnRozlacz test 2";
            audioEnabledVector.fill(false);
            qDebug()<< "btnRozlacz test 3";
            liczba = 0;
            for(int x =0; x < playerVector.size(); x++){
                // KRYTYCZNA POPRAWKA (null-deref): patrz komentarz przy
                // analogicznej pętli w konstruktorze MainWindow.
                if (playerVector[x]){
                    playerVector[x]->stop();
                    qDebug()<< "btnRozlacz test 4" << x;
                }
                qDebug()<< "btnRozlacz test 5->" << x;
            }
            qDebug()<< "btnRozlacz test 6";
        });
        connect(btnAnuluj,&QPushButton::clicked, dialog,&QDialog::close);

        qDebug()<< "btnAnuluj powoduje linia1";
        dialog->show();
        qDebug()<< "btnAnuluj powoduje linia2";
    });

    QGroupBox *groupBox = new QGroupBox("widok okna liveStream",drawerWidgetPodglad);
    groupBox->setAlignment(Qt::AlignCenter);
    //groupBox->setFixedHeight(80);
    QHBoxLayout *layoutWidokOkien = new QHBoxLayout(groupBox);
    layoutWidokOkien->setContentsMargins(5,20,5,5);
    layoutWidokOkien->setSpacing(5);
    QPushButton *btn4okna = new QPushButton(groupBox);
    btn4okna->setStyleSheet(stylesheetPushButton);
    btn4okna->setIcon(createGridIcon(2, 2));   // 4 okna
    btn4okna->setIconSize(QSize(32, 32));
    connect(btn4okna, &QPushButton::clicked,this ,[this](){
        tworzeWidgetNagrania(4);
    });
    QPushButton *btn6okna = new QPushButton(groupBox);
    btn6okna->setStyleSheet(stylesheetPushButton);
    btn6okna->setIcon(createGridIcon(2, 3));   // 6 okna
    btn6okna->setIconSize(QSize(32, 32));
    connect(btn6okna, &QPushButton::clicked,this ,[this](){
        tworzeWidgetNagrania(6);
    });
    QPushButton *btn9okna = new QPushButton(groupBox);
    btn9okna->setStyleSheet(stylesheetPushButton);
    btn9okna->setIcon(createGridIcon(3, 3));   // 9 okna
    btn9okna->setIconSize(QSize(32, 32));
    connect(btn9okna, &QPushButton::clicked,this ,[this](){
        tworzeWidgetNagrania(9);
    });
    QPushButton *btn12okna = new QPushButton(groupBox);
    btn12okna->setStyleSheet(stylesheetPushButton);
    btn12okna->setIcon(createGridIcon(3, 4));   // 12 okna
    btn12okna->setIconSize(QSize(32, 32));
    connect(btn12okna, &QPushButton::clicked,this ,[this](){
        tworzeWidgetNagrania(12);
    });
    QPushButton *btn16okna = new QPushButton(groupBox);
    btn16okna->setStyleSheet(stylesheetPushButton);
    btn16okna->setIcon(createGridIcon(4, 4));   // 16 okna
    btn16okna->setIconSize(QSize(32, 32));
    connect(btn16okna, &QPushButton::clicked,this ,[this](){
        tworzeWidgetNagrania(16);
    });
    layoutWidokOkien->addWidget(btn4okna);
    layoutWidokOkien->addWidget(btn6okna);
    layoutWidokOkien->addWidget(btn9okna);
    layoutWidokOkien->addWidget(btn12okna);
    layoutWidokOkien->addWidget(btn16okna);
    layoutWidokOkien->addStretch(1);
    groupBox->setLayout(layoutWidokOkien);

    QPushButton *closeBtnPodglad = new QPushButton("Ukryj", drawerWidgetPodglad);
    closeBtnPodglad->setIcon(QIcon(":/icons/ukryj.svg"));
    closeBtnPodglad->setIconSize(QSize(32,32));
    closeBtnPodglad->setStyleSheet(stylesheetPushButton);
    connect(closeBtnPodglad, &QPushButton::clicked, this, &MainWindow::ukryjPokazPanelPodglad);
//    layoutPodglad->addStretch(1);
    layoutPodglad->addWidget(groupBox);
    layoutPodglad->addWidget(closeBtnPodglad);

//PANEL BOCZNY NAGRANIA
    QAction *toggleActionNagrania = toolbar->addAction("☰ NAGRANIA");
    toggleActionNagrania->setCheckable(true);
    QToolButton *toolButtonNagrania = qobject_cast<QToolButton*>(toolbar->widgetForAction(toggleActionNagrania));
    toolButtonNagrania->setFocusPolicy(Qt::StrongFocus);  //tutaj
    toolButtonNagrania->setFixedWidth(200);
    toolButtonNagrania->setStyleSheet(
        "QToolButton {"
        "    border: 1px solid black;"
        "    border-radius: 3px;"
        "    padding: 2px;"
        "}"
        "QToolButton:hover {"
        "    border: 1px solid gray;"
        " background-color: #3399FF;"
        " color: white;"
        "}"
        "QToolButton:checked {"
        "    border: 1px solid gray;"
        "    background-color: #3399FF;"
        "    color: white;"
        "}"
        );
    if (toolButtonNagrania) {
        // 3. Ustawiam czcionkę
        // QFont font = toolButtonNagrania->font();
        // font.setPointSize(16);  // zmień na dowolny rozmiar w punktach
        // font.setBold(true);     // opcjonalnie pogrubienie
        toolButtonNagrania->setFont(font);
        // 4. Opcjonalnie zmiana koloru tekstu
        // QPalette pal = toolButtonNagrania->palette();
        // pal.setColor(QPalette::ButtonText, Qt::blue);
        toolButtonNagrania->setPalette(pal);
    }
    connect(toggleActionNagrania, &QAction::triggered, this,[this,toolButtonNagrania](){
        ukryjPokazPanelNagrania();
        if(livePodgladWidget){
            livePodgladWidget->hide();
            centralLabel->show();
            toolButtonNagrania->setFocus();    //tutal
        }
    });
    QVBoxLayout *layoutNagrania = new QVBoxLayout(drawerWidgetNagrania);
    layoutNagrania->setContentsMargins(8,8,8,8);
    layoutNagrania->setSpacing(8);

    QLabel *titleNagrania = new QLabel("<b>NAGRANIA</b>", drawerWidgetNagrania);
    titleNagrania->setStyleSheet(stylesheetLabelSelectedBlue);
    titleNagrania->setAlignment(Qt::AlignCenter);
    layoutNagrania->addWidget(titleNagrania);

    QPushButton *closeBtnNagrania = new QPushButton("Ukryj", drawerWidgetNagrania);
    closeBtnNagrania->setIcon(QIcon(":/icons/ukryj.svg"));
    closeBtnNagrania->setIconSize(QSize(32,32));
    closeBtnNagrania->setStyleSheet(stylesheetPushButton);
    connect(closeBtnNagrania, &QPushButton::clicked, this, &MainWindow::ukryjPokazPanelNagrania);
    layoutNagrania->addStretch(1);
    layoutNagrania->addWidget(closeBtnNagrania);

    QActionGroup *group = new QActionGroup(this);
    group->setExclusive(true);
    group->addAction(toggleActionSerwer);
    group->addAction(toggleActionPodglad);
    group->addAction(toggleActionNagrania);
}

void MainWindow::ukryjPokazPanelSerwer()
{
    drawerWidgetPodglad->setFixedWidth(0);
    statusUkrytyPodglad = true;
    drawerWidgetNagrania->setFixedWidth(0);
    statusUkrytyNagrania = true;

    // Zatrzymaj poprzednią animację tego panelu, jeśli wciąż trwa
    // (zapobiega wyciekowi i "ścinaniu się" animacji przy szybkim klikaniu)
    if (animTimerSerwer) {
        animTimerSerwer->stop();
        animTimerSerwer->deleteLater();
        animTimerSerwer = nullptr;
    }

    animTimerSerwer = new QTimer(this);
    animTimerSerwer->setInterval(10);
    int start = 0;
    int end = 400;
    int krokOd = 0;
    if(statusUkrytySerwer == true){
        krokOd = start;
        connect(animTimerSerwer, &QTimer::timeout,this,[this, krokOd, end]()mutable {
            drawerWidgetSerwer->setFixedWidth(krokOd);
            krokOd += 10;
            if(krokOd >= end){
                this->animTimerSerwer->stop();
                this->animTimerSerwer->deleteLater();
                this->animTimerSerwer = nullptr;
            }
        });
        animTimerSerwer->start();
        statusUkrytySerwer = false;
    }else if(statusUkrytySerwer == false){
        krokOd = end;
        connect(animTimerSerwer, &QTimer::timeout,this,[this, krokOd, start]()mutable {
            drawerWidgetSerwer->setFixedWidth(krokOd);
            krokOd -= 10;
            if(krokOd < start){
                this->animTimerSerwer->stop();
                this->animTimerSerwer->deleteLater();
                this->animTimerSerwer = nullptr;
            }
        });
        animTimerSerwer->start();
        statusUkrytySerwer = true;
    }
}

void MainWindow::ukryjPokazPanelPodglad()
{
    drawerWidgetSerwer->setFixedWidth(0);
    statusUkrytySerwer = true;
    drawerWidgetNagrania->setFixedWidth(0);
    statusUkrytyNagrania = true;

    if (animTimerPodglad) {
        animTimerPodglad->stop();
        animTimerPodglad->deleteLater();
        animTimerPodglad = nullptr;
    }

    animTimerPodglad = new QTimer(this);
    animTimerPodglad->setInterval(10);
    int start = 0;
    int end = 400;
    int krokOd = 0;
    if(statusUkrytyPodglad == true){
        krokOd = start;
        connect(animTimerPodglad, &QTimer::timeout,this,[this, krokOd, end]()mutable {
            drawerWidgetPodglad->setFixedWidth(krokOd);
            krokOd += 10;
            if(krokOd >= end){
                this->animTimerPodglad->stop();
                this->animTimerPodglad->deleteLater();
                this->animTimerPodglad = nullptr;
            }
        });
        animTimerPodglad->start();
        statusUkrytyPodglad = false;
    }else if(statusUkrytyPodglad == false){
        krokOd = end;
        connect(animTimerPodglad, &QTimer::timeout,this,[this, krokOd, start]()mutable {
            drawerWidgetPodglad->setFixedWidth(krokOd);
            krokOd -= 10;
            if(krokOd < start){
                this->animTimerPodglad->stop();
                this->animTimerPodglad->deleteLater();
                this->animTimerPodglad = nullptr;
            }
        });
        animTimerPodglad->start();
        statusUkrytyPodglad = true;
    }
}

void MainWindow::ukryjPokazPanelNagrania()
{
    drawerWidgetSerwer->setFixedWidth(0);
    drawerWidgetPodglad->setFixedWidth(0);
    statusUkrytySerwer = true;
    statusUkrytyPodglad = true;

    if (animTimerNagrania) {
        animTimerNagrania->stop();
        animTimerNagrania->deleteLater();
        animTimerNagrania = nullptr;
    }

    animTimerNagrania = new QTimer(this);
    animTimerNagrania->setInterval(10);
    int start = 0;
    int end = 400;
    int krokOd = 0;
    if(statusUkrytyNagrania == true){
        krokOd = start;
        connect(animTimerNagrania, &QTimer::timeout,this,[this, krokOd, end]()mutable {
            drawerWidgetNagrania->setFixedWidth(krokOd);
            krokOd += 10;
            if(krokOd >= end){
                this->animTimerNagrania->stop();
                this->animTimerNagrania->deleteLater();
                this->animTimerNagrania = nullptr;
            }
        });
        animTimerNagrania->start();
        statusUkrytyNagrania = false;
    }else if(statusUkrytyNagrania == false){
        krokOd = end;
        connect(animTimerNagrania, &QTimer::timeout,this,[this, krokOd, start]()mutable {
            drawerWidgetNagrania->setFixedWidth(krokOd);
            krokOd -= 10;
            if(krokOd < start){
                this->animTimerNagrania->stop();
                this->animTimerNagrania->deleteLater();
                this->animTimerNagrania = nullptr;
            }
        });
        animTimerNagrania->start();
        statusUkrytyNagrania = true;
    }
}

void MainWindow::tworzeWidgetNagrania(int ileKamer)
{
    labelVideoVector.clear();
    kameraWidgetVector.clear();
    sliderVector.clear();
    btnAudioOnVector.clear();

    // Zatrzymujemy wszystkie aktywne playery - ich labele zostaną zaraz zniszczone
    for (FfmpegPlayer *player : std::as_const(playerVector)) {
            player->stop();
    }
    playerVector.clear();
    ignoreAspectRatio.clear();
    ignoreAspectRatio.resize(ileKamer, false); // jeden element per kamera, domyślnie KeepAspectRatio

    if (livePodgladWidget) {
        rootLayout->removeWidget(livePodgladWidget);
        livePodgladWidget->deleteLater();
        livePodgladWidget = nullptr;
    }
    powiekszonyLabel = nullptr; // reset trybu powiększenia przy tworzeniu nowej siatki

    int cols = qMax(1, (int)std::ceil(std::sqrt(ileKamer)));
    int row = 0;
    int col = 0;
    centralLabel->hide();
    livePodgladWidget = new QWidget(this);
    grid = new QGridLayout(livePodgladWidget); // pole klasy - dostępne w eventFilter
    grid->setContentsMargins(2,2,2,2);
    grid->setSpacing(2);
    // Ignoruj minimumSizeHint dzieci (QLabel z pixmapą rozszerza minimum przez setPixmap)
    grid->setSizeConstraint(QLayout::SetNoConstraint);


    for(int x = 0; x < ileKamer; x++){
        kameraWidgetVector.append(nullptr);
        kameraWidgetVector[x] = new QWidget(livePodgladWidget);
        kameraWidgetVector[x]->setProperty("powieksz", false);
        QVBoxLayout *layoutKameraWidget = new QVBoxLayout(kameraWidgetVector[x]);
        layoutKameraWidget->setContentsMargins(0,0,0,0);
        qDebug()<< kameraWidgetVector.count();
        QLabel *labelVideo = new QLabel("KAMERA Nr: "+QString::number(x+1)+"\nBRAK OBRAZU", kameraWidgetVector[x]);
        labelVideo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        labelVideo->setMinimumSize(1, 1);
        labelVideoVector.append(labelVideo);
        labelVideo->setAlignment(Qt::AlignCenter);
        labelVideo->setStyleSheet("background: black;color: white;font-size:26px");
        labelVideo->setProperty("indexKamery", x);
        labelVideo->setProperty("rowKamery", row);
        labelVideo->setProperty("colKamery", col);
        layoutKameraWidget->addWidget(labelVideo);

        kameraWidgetVector[x]->setProperty("kameraNumber", x);
        kameraWidgetVector[x]->setProperty("rowKamery", row);
        kameraWidgetVector[x]->setProperty("colKamery", col);
        grid->addWidget(kameraWidgetVector[x], row, col);

        QGroupBox *gboxPasekWlasciwosci = new QGroupBox(kameraWidgetVector[x]);
        gboxPasekWlasciwosci->raise();
        qDebug() <<kameraWidgetVector[x]->width();
        gboxPasekWlasciwosci->setMaximumHeight(50);
        QHBoxLayout *layoutResize = new QHBoxLayout(gboxPasekWlasciwosci);
        layoutResize->setContentsMargins(0,0,0,0);
        layoutResize->setSpacing(0);
        QPushButton *btnResize = new QPushButton(gboxPasekWlasciwosci);//"⤡"
        btnResize->setIconSize(QSize(28,28));
        btnResize->setMaximumWidth(50);
        btnResize->setIcon(QIcon(":/icons/resize.svg"));
        btnResize->setStyleSheet(stylesheetPushButton);
        btnResize->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        connect(btnResize, &QPushButton::clicked, kameraWidgetVector[x],[x,this](){
            if (x >= playerVector.size() || !playerVector[x])
                return;

            ignoreAspectRatio[x] = !ignoreAspectRatio[x];

            playerVector[x]->setAspectRatioMode(
                ignoreAspectRatio[x] ? Qt::IgnoreAspectRatio
                                     : Qt::KeepAspectRatio);
        });
    //    QPushButton *btnAudioOn = new QPushButton(gboxPasekWlasciwosci);//"🔊"
        btnAudioOnVector.append(nullptr);
        btnAudioOnVector[x] = new QPushButton(gboxPasekWlasciwosci);
        btnAudioOnVector[x]->setIconSize(QSize(28,28));
        btnAudioOnVector[x]->setMaximumWidth(50);
        btnAudioOnVector[x]->setIcon(QIcon(":/icons/speaker-muted.svg"));
        btnAudioOnVector[x]->setStyleSheet(stylesheetPushButton);
        btnAudioOnVector[x]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        connect(btnAudioOnVector[x], &QPushButton::clicked, kameraWidgetVector[x],
                [this,x](){
            bool wlacz = !audioEnabledVector[x];
            // wyłącz wszystkie pozostałe kamery
            for (int i = 0; i < playerVector.size(); ++i)
            {
                if (i == x)
                    continue;
                audioEnabledVector[i] = false;

                if (playerVector[i])
                    playerVector[i]->setAudioEnabled(false);

                sliderVector[i]->blockSignals(true);
                sliderVector[i]->setValue(0);
                sliderVector[i]->blockSignals(false);
                btnAudioOnVector[i]->setIcon(QIcon(":/icons/speaker-muted.svg"));
            }
            // ustaw stan klikniętej kamery
            audioEnabledVector[x] = wlacz;

            if (playerVector[x])
                playerVector[x]->setAudioEnabled(wlacz);

            sliderVector[x]->blockSignals(true);
            sliderVector[x]->setValue(wlacz ? 6 : 0);
        //    btnAudioOnVector[x]->setIcon(QIcon(":/icons/speaker-medium.svg"));
            sliderVector[x]->blockSignals(false);
            btnAudioOnVector[x]->setIcon(QIcon(
                wlacz ? ":/icons/speaker-medium.svg"
                      : ":/icons/speaker-muted.svg"));
            // Ustawiamy głośność na wartość slidera (6) gdy włączamy audio
            if (wlacz && playerVector[x])
                playerVector[x]->setVolume(6);
        });

        sliderVector.append(nullptr);
        sliderVector[x] = new QSlider(gboxPasekWlasciwosci);
        sliderVector[x]->setStyleSheet(stylesheetSliderBlue);
        sliderVector[x]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sliderVector[x]->setOrientation(Qt::Horizontal);
        sliderVector[x]->setRange(0, 10);
        sliderVector[x]->setValue(0);
        connect(sliderVector[x], &QSlider::valueChanged, kameraWidgetVector[x],[this,x](){
            int val = sliderVector[x]->value();

            // Aktualizuj ikonę
            if(val == 0)
                btnAudioOnVector[x]->setIcon(QIcon(":/icons/speaker-muted.svg"));
            else if(val <= 3)
                btnAudioOnVector[x]->setIcon(QIcon(":/icons/speaker-low.svg"));
            else if(val <= 7)
                btnAudioOnVector[x]->setIcon(QIcon(":/icons/speaker-medium.svg"));
            else
                btnAudioOnVector[x]->setIcon(QIcon(":/icons/speaker-high.svg"));

            // Reguluj głośność playera
            if(x < playerVector.count() && playerVector[x]) {
                if(val == 0) {
                    playerVector[x]->setAudioEnabled(false);
                } else {
                    if(!playerVector[x]->isAudioEnabled())
                        playerVector[x]->setAudioEnabled(true);
                    playerVector[x]->setVolume(val);
                }
            }

            // Wycisz pozostałe kamery
            for(int i = 0; i < sliderVector.count(); i++) {
                if(i != x) {
                    sliderVector[i]->blockSignals(true);
                    sliderVector[i]->setValue(0);
                    sliderVector[i]->blockSignals(false);
                    btnAudioOnVector[i]->setIcon(QIcon(":/icons/speaker-muted.svg"));
                    if(i < playerVector.count() && playerVector[i])
                        playerVector[i]->setAudioEnabled(false);
                }
            }
        });
        layoutResize->addWidget(btnResize,1);
        layoutResize->addWidget(btnAudioOnVector[x],1);
        layoutResize->addWidget(sliderVector[x],4);
        connect(this, &MainWindow::sygnalResize,gboxPasekWlasciwosci,
                [gboxPasekWlasciwosci,this,x](){
            qDebug()<< kameraWidgetVector[x]->width() << kameraWidgetVector[x]->height();
            gboxPasekWlasciwosci->setGeometry(0,kameraWidgetVector[x]->height()-30,
                                    kameraWidgetVector[x]->width(),30);
        });

        col++;
        if (col >= cols) {
            col = 0; row++;
        }
        kameraWidgetVector[x]->installEventFilter(this);
    //    labelVideo->setProperty("camContainer", QVariant::fromValue((QWidget*)livePodgladWidget));
        labelVideo->setCursor(Qt::PointingHandCursor);
        gboxPasekWlasciwosci->hide();

        // Ustawiamy geometrię od razu przy tworzeniu (nie tylko przy resize)
        // żeby gbox miał właściwą pozycję zanim nastąpi pierwszy sygnalResize
        gboxPasekWlasciwosci->setGeometry(0, 0, kameraWidgetVector[x]->width(), 30);

    }

    // Równy stretch dla wszystkich wierszy i kolumn od początku
    for (int i = 0; i < grid->rowCount(); i++)
        grid->setRowStretch(i, 1);
    for (int i = 0; i < grid->columnCount(); i++)
        grid->setColumnStretch(i, 1);

    rootLayout->addWidget(livePodgladWidget);
    QTimer::singleShot(0, this, [this](){
        emit sygnalResize();
    });
}

void MainWindow::zapiszSerweryDat()
{
    QString path = appHomePath+"/";
    path = path.simplified();
    path.remove(" ");
    //path = path.trimmed();
    QDir dir;
    if (!dir.exists(path))
        dir.mkpath(path);
    QFile file(path+"serwery.dat");
    if (file.open(QIODevice::WriteOnly))
    {
        QDataStream stream(&file);
        stream.setVersion(QDataStream::Qt_6_0);
        qint32 n = ItemModelSerweryDat->rowCount();
        qint32 m = ItemModelSerweryDat->columnCount();
        stream << n << m;
        for (int i=0; i<n; ++i)
        {
            for (int j=0; j<m; j++)
            {
                QString tekst = ItemModelSerweryDat->item(i,j)
                                    ? ItemModelSerweryDat->item(i,j)->text()
                                    : QString();
                stream << tekst;
            }
        }
        file.close();
        QMessageBox::information(this, "INFO", "Lista serwerów zapisana");
    }else{
        QMessageBox::information(this, "INFO", "Lista serwerów nie zapisana");
    }
}

void MainWindow::czytajSerweryDat()
{
    if (!ItemModelSerweryDat)
        ItemModelSerweryDat = new QStandardItemModel(this);
    else
        ItemModelSerweryDat->clear();

    QString path = appHomePath+"/";
    path = path.simplified();
    path.remove(" ");

    QFile file(path+"serwery.dat");
    if (file.open(QIODevice::ReadOnly))
    {
        QDataStream stream(&file);
        stream.setVersion(QDataStream::Qt_6_0);
        qint32 n, m;
        stream >> n >> m;
        ItemModelSerweryDat->setRowCount(n);
        ItemModelSerweryDat->setColumnCount(m);

        for (int i = 0; i < n ; ++i) {
            for (int j = 0; j < m; j++) {
                QString tekst;
                stream >> tekst;
                QStandardItem *item = new QStandardItem(tekst);
                item->setTextAlignment(Qt::AlignCenter);
                ItemModelSerweryDat->setItem(i, j, item);
            }
        }
        file.close();

    }else{
        qDebug()<< "NIE MOŻNA OTWORZYĆ PLIKU" << file.fileName();
    }
}

void MainWindow::createWidgetListaLivekamery()
{
   // czytajSerweryDat();

    // menuListPodglad->clear();   //clerowanie pod butonem btnSerweryLiveStream
    // widgetVectr.clear();
    // widgetLayutVector.clear();
    // itemVector.clear();

    QFont font2;
    font2.setBold(true);
    font2.setPointSize(16);
    //int liczba = 0;

    // for(int x = 0; x < ItemModelSerweryDat->rowCount(); x++){ tutaj
    //     QString nazwa = ItemModelSerweryDat->item(x,1)
    //     ?ItemModelSerweryDat->item(x,1)->text()
    //     :QString();
    //     QString adres = ItemModelSerweryDat->item(x,2)
    //                         ?ItemModelSerweryDat->item(x,2)->text()
    //                         :QString();
    qDebug() << "table =" << table;
        Q_ASSERT(table);
        int rowSelected = table->currentRow();
        QString nazwa = table->item(rowSelected,1)
                            ?table->item(rowSelected,1)->text()
                            :QString();
        QString adres = table->item(rowSelected,2)
                            ?table->item(rowSelected,2)->text()
                            :QString();
        qDebug()<< "teraz dobrze"<< nazwa << adres;

        QListWidgetItem *item = new QListWidgetItem();
        itemVector.append(item);
        QWidget *widget = new QWidget();
        widgetVectr.append(widget);
        widget->setStyleSheet(
            "QWidget{"
            " background-color: #3399FF;"
            " color: white;"
            //" border: none;"
            "border: 2px solid white;"
            "}"
            );
        QVBoxLayout *widgetLayut = new QVBoxLayout(widget);
        widgetLayutVector.append(widgetLayut);
        QHBoxLayout *hlayout = new QHBoxLayout();
        hlayout->setContentsMargins(8,4,8,4);
        hlayout->setSpacing(10);
        QLabel *label = new QLabel(nazwa,widget);
        label->setMaximumWidth(150);
        label->setFont(font2);
        label->setStyleSheet("border: none;");
        QLabel *label2 = new QLabel(adres,widget);
        label2->setMaximumWidth(140);
        label2->setFont(font2);
        label2->setStyleSheet("border: none;");
        hlayout->addWidget(label);
        hlayout->addStretch();
        hlayout->addWidget(label2);
        widgetLayut->addLayout(hlayout);
        item->setFont(font2);
        item->setSizeHint(widget->sizeHint());

    if(czytajKameryDat("http://"+adres+":8080/kamery.dat")){
        qDebug()<<"http://"+adres+":8080/kamery.dat";
        for(int row = 0; row < ItemModel->rowCount(); row++){
            QString kameraName = ItemModel->item(row,1)
                ?ItemModel->item(row,1)->text()
                :QString();
            QString adresKamery = ItemModel->item(row,2)
                ?ItemModel->item(row,2)->text()
                :QString();
            QGroupBox *groupBox = new QGroupBox(widget);
            QHBoxLayout *hlayout = new QHBoxLayout(groupBox);
            hlayout->setContentsMargins(8,4,8,4);
            QLabel *label = new QLabel(kameraName,widget);
            label->setMaximumWidth(150);
            label->setFixedWidth(150);
        //    label->setStyleSheet("border: none;");
            QPushButton *btnOn = new QPushButton("On",widget);
            btnOn->setFixedWidth(60);
            btnOn->setStyleSheet(stylesheetPushButton);
            int nrKamery = liczba;
            connect(btnOn, &QPushButton::clicked, widget,[this,nrKamery,kameraName,adresKamery,adres](){
                if(labelVideoVector.isEmpty()){
                    QMessageBox::information(nullptr,"INFO","WYBIERZ PODZIAŁ SIATKI KAMER");
                    return;
                }
                if(nrKamery >= labelVideoVector.size()){
                    QMessageBox::information(nullptr,"INFO",
                        QString("BRAK WOLNEGO OKNA\n (OKNA: %1,LICZBA KAMER: %2)\nWYBIERZ SIATKĘ DLA %2 KAMER")
                        .arg(labelVideoVector.size()).arg(nrKamery+1));
                    return;
                }

                // Rozbudowujemy playerVector do odpowiedniego rozmiaru
                while(playerVector.size() <= nrKamery){
                    playerVector.append(nullptr);
                }
                audioEnabledVector.resize(playerVector.size(), false);

            //   if(btnOn->text() == "On"){
                    // Tworzymy player i startujemy odtwarzanie
                if(!playerVector[nrKamery]){
                    playerVector[nrKamery] = new FfmpegPlayer(this);
                    connect(playerVector[nrKamery], &FfmpegPlayer::error,
                            this, [nrKamery](const QString &msg){
                        qWarning() << "Player" << nrKamery << "błąd:" << msg;
                    });
                }
                kameraWidgetVector[nrKamery]->setProperty("powieksz", true);
                QString rtspUrl = "rtsp://"+adres+":8554/"+kameraName;
                playerVector[nrKamery]->setLabel(labelVideoVector[nrKamery]);
                playerVector[nrKamery]->setUrl(rtspUrl);
                playerVector[nrKamery]->play();
            //        btnOn->setText("Off");
                qDebug() << "Start odtwarzania:" << rtspUrl << "→ slot" << nrKamery;

                // } else {
                //     // Zatrzymujemy odtwarzanie
                //     if(playerVector[nrKamery] && playerVector[nrKamery]->isPlaying()){
                //         playerVector[nrKamery]->stop();
                //         labelVideoVector[nrKamery]->clear();
                //         labelVideoVector[nrKamery]->setText(QString("KAMERA Nr: %1\nBRAK OBRAZU").arg(nrKamery+1));
                //     }
                //     btnOn->setText("On");
                // }
            });
            QPushButton *btnOff = new QPushButton("Off", widget);
            btnOff->setFixedWidth(60);
            btnOff->setStyleSheet(stylesheetPushButton);
            connect(btnOff, &QPushButton::clicked,widget,[this,nrKamery](){
                if(nrKamery >= labelVideoVector.size()){
                    QMessageBox::information(nullptr,"INFO",
                        QString("Ta kamera nie odtwarza (okna: %1, kamera: %2)")
                            .arg(labelVideoVector.size()).arg(nrKamery+1));
                    return;
                }
                if(nrKamery >= playerVector.size()){
                    return;
                }
                // Zatrzymujemy odtwarzanie
                if(playerVector[nrKamery] && playerVector[nrKamery]->isPlaying()){
                    playerVector[nrKamery]->stop();
                    labelVideoVector[nrKamery]->clear();
                    labelVideoVector[nrKamery]->setText(QString("KAMERA Nr: %1\nBRAK OBRAZU").arg(nrKamery+1));
                }
            });

            hlayout->addWidget(label);
            hlayout->addStretch(1);
            hlayout->addWidget(btnOn);
            hlayout->addWidget(btnOff);
            widgetLayutVector.last()->addWidget(groupBox);

        //    widgetLayutVector[x]->addLayout(hlayout);
            liczba++;
        }
        menuListPodglad->addItem(item);
        menuListPodglad->setItemWidget(item,widget);

//tutaj    }
        for (int i = 0; i < widgetVectr.size(); ++i)
            {
                itemVector[i]->setSizeHint(widgetVectr[i]->sizeHint());
            }
    }else{
        QMessageBox::information(this,"INFO","ERROR CAM");
    }
    tworzeWidgetNagrania(liczba);
}

void MainWindow::createWidgetUstawienia()
{
    QStackedWidget *stack = new QStackedWidget(this);
    stack->setAttribute(Qt::WA_DeleteOnClose);
    stack->setGeometry(0,0,this->width(),this->height());
    QWidget *widget = new QWidget();
    widget->setStyleSheet("background-color: white;");
    widget->setAttribute(Qt::WA_DeleteOnClose);
    widget->setWindowTitle("USTAWIENIA");
    widget->setGeometry(0,0,this->width(),this->height());
    QVBoxLayout *widgetLayout = new QVBoxLayout(widget);
//    QLabel *labelTitle = new QLabel("NA SERWERZE "+adreshttp+"\nKAMERY:\nDODAJ USUŃ MODYFIKUJ",widget);
    QString tekst = QString(
                        "<div style='font-size: 24px;'>SERWER %1</div>"
                        "<div style='font-size: 18px;'>KAMERY:<br>DODAJ USUŃ MODYFIKUJ</div>"
                        ).arg(adreshttp);
    QLabel *labelTitle = new QLabel(tekst, widget);
    labelTitle->setAlignment(Qt::AlignCenter);
    labelTitle->setStyleSheet(stylesheetLabelSelectedBlue);
    QTableWidget *table = new QTableWidget(0,9,widget);
    table->verticalHeader()->setVisible(false);
    table->setHorizontalHeaderLabels(
        {"Id.", "Kamera", "Adres","Rozdzielczość","fps/s","Zapis nagrań do","Ile dni","Czułość","Detekcja"}
        );
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setShowGrid(true);
    QHeaderView *header = table->horizontalHeader();
    header->setFixedHeight(40);
    header->setDefaultAlignment(Qt::AlignCenter);
    header->setStyleSheet(
        "QHeaderView::section{"
        "background:#BDE8FF;"
        "color:#003366;"
        "font-weight:bold;"
        "font-size:20px;"
        "border:1px solid #8EC7E8;"
        "padding:6px;"
        "}"
        );
    header->setSectionResizeMode(0,QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1,QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2,QHeaderView::Stretch);
    header->setSectionResizeMode(3,QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4,QHeaderView::ResizeToContents);
    header->setSectionResizeMode(5,QHeaderView::Stretch);
    header->setSectionResizeMode(6,QHeaderView::ResizeToContents);
    header->setSectionResizeMode(7,QHeaderView::ResizeToContents);
    header->setSectionResizeMode(8,QHeaderView::ResizeToContents);
//    table->hideColumn(3);
//    table->hideColumn(4);

    for(int row = 0; row < ItemModel->rowCount(); row++){
        table->insertRow(row);
        for(int col = 0; col < ItemModel->columnCount(); col++){
            QString text = ItemModel->item(row,col)
            ?ItemModel->item(row,col)->text()
            :QString();
            QTableWidgetItem *item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            table->setItem(row,col, item);
        }
    }
    table->setStyleSheet(stylesheetTable);

    QHBoxLayout *layouth1 = new QHBoxLayout();
    QPushButton *btnZapisz = new QPushButton("ZAPISZ",widget);
    btnZapisz->setIcon(QIcon(":/icons/zapisz.svg"));
    btnZapisz->setIconSize(QSize(32,32));
    btnZapisz->setStyleSheet(stylesheetPushButton);
    QPushButton *btnDodaj = new QPushButton("DODAJ",widget);
    btnDodaj->setIcon(QIcon(":/icons/dodaj.svg"));
    btnDodaj->setIconSize(QSize(32,32));
    btnDodaj->setStyleSheet(stylesheetPushButton);
    QPushButton *btnModyfikuj = new QPushButton("POKAŻ - MODYFIKUJ",widget);
    btnModyfikuj->setIcon(QIcon(":/icons/pokazmodyfikuj.svg"));
    btnModyfikuj->setIconSize(QSize(32,32));
    btnModyfikuj->setStyleSheet(stylesheetPushButton);
    QPushButton *strefyRuchu = new QPushButton("STREFY RUCHU",widget);
    strefyRuchu->setIcon(QIcon(":/icons/strefyruchu.svg"));
    strefyRuchu->setIconSize(QSize(32,32));
    strefyRuchu->setStyleSheet(stylesheetPushButton);
    QPushButton *btnUsun = new QPushButton("USUŃ",widget);
    btnUsun->setIcon(QIcon(":/icons/usun.svg"));
    btnUsun->setIconSize(QSize(32,32));
    btnUsun->setStyleSheet(stylesheetPushButton);
    QPushButton *btnAnuluj = new QPushButton("ANULUJ",widget);
    btnAnuluj->setIcon(QIcon(":/icons/anuluj.svg"));
    btnAnuluj->setIconSize(QSize(32,32));
    btnAnuluj->setStyleSheet(stylesheetPushButtonRed);
    layouth1->addWidget(btnZapisz,1);
    layouth1->addWidget(btnDodaj,1);
    layouth1->addWidget(btnModyfikuj,1);
    layouth1->addWidget(strefyRuchu,1);
    layouth1->addWidget(btnUsun,1);
    layouth1->addStretch(1);
    layouth1->addWidget(btnAnuluj,1);
    widgetLayout->addWidget(labelTitle);
    widgetLayout->addWidget(table);
    widgetLayout->addLayout(layouth1);

    QString labelStyleSheet = R"(QLabel{
        border: 2px solid #0078D7;
        background-color: #BDE8FF;
        color: black;
        font-size: 30px;
        }
    )";
    QString labelBorderStyleSheet = R"(QLabel{
        border: 2px solid #0078D7;
        color: black;
        font-size: 30px;
    })";
    QString lineEditStyleSheet  = R"(QLineEdit{
        border: 2px solid #0078D7;
        color: black;
        font-size: 30px;
    })";
    QString spinBoxStyleSheet = R"(
        QSpinBox{
        border: 2px solid #0078D7;
        color: black;
        font-size: 30px;
    }
        QSpinBox:focus {
        color: black;
        background-color: white;
        selection-background-color: white;
        selection-color: black;
    }
    )";
    QString comboBoxStyleSheet = R"(
        QComboBox{
        border: 2px solid #0078D7;
        color: black;
        font-size: 30px;
        selection-background-color: #BDE8FF;
    }
    )";
//widgetDodajKam
    QWidget *widgetDodajKam = new QWidget();
    widgetDodajKam->setStyleSheet("background-color: white;");
    widgetDodajKam->setAttribute(Qt::WA_DeleteOnClose);
    widgetDodajKam->setWindowTitle("USTAWIENIA KAMERY");
    widgetDodajKam->setGeometry(0,0,this->width(),this->height());
    QVBoxLayout *dodajKamLayout = new QVBoxLayout(widgetDodajKam);
    QLabel *labelDodajKam = new QLabel("KAMERY:\nDODAJ",widgetDodajKam);
    labelDodajKam->setAlignment(Qt::AlignCenter);
    labelDodajKam->setStyleSheet(stylesheetLabelSelectedBlue);
    dodajKamLayout->addWidget(labelDodajKam);

    QHBoxLayout *layoutDK0 = new QHBoxLayout();
    QLabel *labelDK01 = new QLabel("Id",widgetDodajKam);
    labelDK01->setAlignment(Qt::AlignCenter);
    labelDK01->setStyleSheet(labelStyleSheet);
    QLabel *labelDK02 = new QLabel(QString::number(table->rowCount()),widgetDodajKam);
    labelDK02->setAlignment(Qt::AlignCenter);
    labelDK02->setStyleSheet(labelBorderStyleSheet);
    layoutDK0->addWidget(labelDK01);
    layoutDK0->addWidget(labelDK02);
    layoutDK0->addStretch(0);

    QHBoxLayout *layoutDK1 = new QHBoxLayout();
    QLabel *labelDkNazwa = new QLabel("KAMERA:",widgetDodajKam);
    labelDkNazwa->setAlignment(Qt::AlignCenter);
    labelDkNazwa->setStyleSheet(labelStyleSheet);
    QLineEdit *lineEditNazwa = new QLineEdit("",widgetDodajKam);
    lineEditNazwa->setAlignment(Qt::AlignCenter);
    lineEditNazwa->setStyleSheet(lineEditStyleSheet);
    layoutDK1->addWidget(labelDkNazwa);
    layoutDK1->addWidget(lineEditNazwa);

    QHBoxLayout *layoutDK2 = new QHBoxLayout();
    QLabel *labelDkAdres = new QLabel("ADRES STRUMIENIA:",widgetDodajKam);
    labelDkAdres->setAlignment(Qt::AlignCenter);
    labelDkAdres->setStyleSheet(labelStyleSheet);
    QLineEdit *lineEditAdres = new QLineEdit("",widgetDodajKam);
    lineEditAdres->setAlignment(Qt::AlignCenter);
    lineEditAdres->setStyleSheet(lineEditStyleSheet);
    layoutDK2->addWidget(labelDkAdres);
    layoutDK2->addWidget(lineEditAdres);

    QHBoxLayout *layoutDK3 = new QHBoxLayout();
    QLabel *labelDkRozdz1 = new QLabel("ROZDZIELCZOŚĆ KAMERY:",widgetDodajKam);
    labelDkRozdz1->setAlignment(Qt::AlignCenter);
    labelDkRozdz1->setStyleSheet(labelStyleSheet);
    QLabel *labelDkRozdz2 =new QLabel(widgetDodajKam);
    labelDkRozdz2->setAlignment(Qt::AlignCenter);
    labelDkRozdz2->setStyleSheet(labelBorderStyleSheet);
    layoutDK3->addWidget(labelDkRozdz1);
    layoutDK3->addWidget(labelDkRozdz2);
    layoutDK3->addStretch(0);

    QHBoxLayout *layoutDK4 = new QHBoxLayout();
    QLabel *labelFps1 = new QLabel("fps/s", widgetDodajKam);
    labelFps1->setAlignment(Qt::AlignCenter);
    labelFps1->setStyleSheet(labelStyleSheet);
    QLabel *labelFps2 = new QLabel(widgetDodajKam);
    labelFps2->setAlignment(Qt::AlignCenter);
    labelFps2->setStyleSheet(labelBorderStyleSheet);
    layoutDK4->addWidget(labelFps1);
    layoutDK4->addWidget(labelFps2);
    layoutDK4->addStretch(0);

    QHBoxLayout *layoutDK5 = new QHBoxLayout();
    QLabel *labelDkPath = new QLabel("ZAPIS NAGRAŃ DO:",widgetDodajKam);
    labelDkPath->setAlignment(Qt::AlignCenter);
    labelDkPath->setStyleSheet(labelStyleSheet);
    QString path = QDir::homePath()+"/AppMultiCam/nagrania/";
    QLineEdit *lineEditPath = new QLineEdit(path,widgetDodajKam);
    lineEditPath->setReadOnly(true);
    lineEditPath->setAlignment(Qt::AlignCenter);
    connect(lineEditNazwa, &QLineEdit::textChanged,widgetDodajKam,
                                [path,lineEditPath,lineEditNazwa](){
        lineEditPath->setText(path+lineEditNazwa->text().toLower().trimmed().remove(' ')+"/");
    });

    QPushButton *btnPath = new QPushButton("▼",widgetDodajKam);
    btnPath->setFixedSize(40,40);
    btnPath->setStyleSheet(stylesheetPushButton);
    connect(btnPath, &QPushButton::clicked, this, [lineEditPath,lineEditNazwa]() {
        if(lineEditNazwa->text().trimmed().isEmpty()){
            QMessageBox::information(nullptr,"UWAGA","WYPEŁNIJ NAJPIERW NAZWĘ KAMERY");
            lineEditNazwa->setFocus();
            return;
        }
        QString sciezka = QFileDialog::getExistingDirectory(
            nullptr,
            "Wybierz katalog",
            QDir::homePath()
            );
        if (!sciezka.isEmpty())
            lineEditPath->setText(sciezka+"/"+lineEditNazwa->text().toLower().trimmed().remove(' ')+"/");
    });
    lineEditPath->setStyleSheet(lineEditStyleSheet);
    layoutDK5->addWidget(labelDkPath);
    layoutDK5->addWidget(lineEditPath);
    layoutDK5->addWidget(btnPath);

    QHBoxLayout *layoutDK6 = new QHBoxLayout();
    QLabel *labelDkIleDni = new QLabel("ILE DNI PRZECHOWYWAĆ:",widgetDodajKam);
    labelDkIleDni->setAlignment(Qt::AlignCenter);
    labelDkIleDni->setStyleSheet(labelStyleSheet);
    QSpinBox *spinBoxIleDni = new QSpinBox(widgetDodajKam);
    spinBoxIleDni->setValue(10);
    spinBoxIleDni->setAlignment(Qt::AlignCenter);
    spinBoxIleDni->setStyleSheet(spinBoxStyleSheet);
    layoutDK6->addWidget(labelDkIleDni);
    layoutDK6->addWidget(spinBoxIleDni);
    layoutDK6->addStretch(0);

    QHBoxLayout *layoutDK7 = new QHBoxLayout();
    QLabel *labelDkCzulosc = new QLabel("CZUŁOŚĆ DETEKCJI:",widgetDodajKam);
    labelDkCzulosc->setAlignment(Qt::AlignCenter);
    labelDkCzulosc->setStyleSheet(labelStyleSheet);
    QSpinBox *spinBoxCzulosc = new QSpinBox(widgetDodajKam);
    spinBoxCzulosc->setMinimum(1000);
    spinBoxCzulosc->setMaximum(100000);
    spinBoxCzulosc->setValue(10000);
    spinBoxCzulosc->setAlignment(Qt::AlignCenter);
    spinBoxCzulosc->setStyleSheet(spinBoxStyleSheet);
    spinBoxCzulosc->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Maximum);
    layoutDK7->addWidget(labelDkCzulosc);
    layoutDK7->addWidget(spinBoxCzulosc);
    layoutDK7->addStretch(0);

    QHBoxLayout *layoutDK8 = new QHBoxLayout();
    QLabel *labelDkDetekcja = new QLabel("DETEKCJA RUCHU:",widgetDodajKam);
    labelDkDetekcja->setAlignment(Qt::AlignCenter);
    labelDkDetekcja->setStyleSheet(labelStyleSheet);
    QComboBox *comboBoxDetekcja = new QComboBox(widgetDodajKam);
    comboBoxDetekcja->setStyleSheet(comboBoxStyleSheet);
    comboBoxDetekcja->setFixedWidth(150);
    comboBoxDetekcja->addItem("TAK",true);
    comboBoxDetekcja->addItem("NIE",false);
    for (int i = 0; i < comboBoxDetekcja->count(); ++i)
        comboBoxDetekcja->setItemData(
            i,
            Qt::AlignCenter,
            Qt::TextAlignmentRole
            );
    layoutDK8->addWidget(labelDkDetekcja);
    layoutDK8->addWidget(comboBoxDetekcja);
    layoutDK8->addStretch(0);

    int szerokosc = 150;
    QHBoxLayout *layoutBtn = new QHBoxLayout();
    // QPushButton *btnZapisz = new QPushButton("ZAPISZ",widgetDodajKam);
    // btnZapisz->setStyleSheet(stylesheetPushButton);
    // btnZapisz->setFixedWidth(szerokosc);
    QPushButton *btnTest = new QPushButton("TEST",widgetDodajKam);
    btnTest->setStyleSheet(stylesheetPushButton);
    btnTest->setFixedWidth(szerokosc);
    // QPushButton *btnStrefyRuchu = new QPushButton("STREFY RUCHU",widgetDodajKam);
    // btnStrefyRuchu->setStyleSheet(stylesheetPushButton);
    // btnStrefyRuchu->setFixedWidth(szerokosc);
    QPushButton *btnCancel = new QPushButton("ANULUJ",widgetDodajKam);
    btnCancel->setStyleSheet(stylesheetPushButtonRed);
    btnCancel->setFixedWidth(szerokosc);
//    layoutBtn->addWidget(btnZapisz);
    layoutBtn->addWidget(btnTest);
//    layoutBtn->addWidget(btnStrefyRuchu);
    layoutBtn->addStretch(0);
    layoutBtn->addWidget(btnCancel);


    dodajKamLayout->addLayout(layoutDK0);
    dodajKamLayout->addLayout(layoutDK1);
    dodajKamLayout->addLayout(layoutDK2);
    dodajKamLayout->addLayout(layoutDK3);
    dodajKamLayout->addLayout(layoutDK4);
    dodajKamLayout->addLayout(layoutDK5);
    dodajKamLayout->addLayout(layoutDK6);
    dodajKamLayout->addLayout(layoutDK7);
    dodajKamLayout->addLayout(layoutDK8);
    dodajKamLayout->addStretch(0);
    dodajKamLayout->addLayout(layoutBtn);

    QList<QLabel*> labelList;
    int w = 400;
    labelList = {labelDK01,labelDkNazwa,labelDkAdres,labelDkRozdz1,labelFps1,labelDkPath,labelDkIleDni,
                 labelDkCzulosc,labelDkDetekcja};
    for(auto lab: std::as_const(labelList)){
        lab->setFixedWidth(w);
    }
    w = 240;
    labelDK02->setFixedWidth(w);
    labelDkRozdz2->setFixedWidth(w);
    labelFps2->setFixedWidth(w);
    spinBoxIleDni->setFixedWidth(w);
    spinBoxCzulosc->setFixedWidth(w);
    comboBoxDetekcja->setFixedWidth(w);
    lineEditNazwa->setFocus();
//widgetDodajKam koniec

    stack->addWidget(widget);
    stack->addWidget(widgetDodajKam);
    stack->show();

    table->selectRow(0);
    table->setFocus();

    connect(btnZapisz, &QPushButton::clicked, widget,[this,table](){
        menuListPodglad->clear();
    qDebug()<< "test 1";
        widgetVectr.clear();
        widgetLayutVector.clear();
        itemVector.clear();
    qDebug()<< "test 2";
        audioEnabledVector.fill(false);
    qDebug()<< "test 3";
        liczba = 0;
        for(int x =0; x < playerVector.size(); x++){
            // KRYTYCZNA POPRAWKA (null-deref): patrz komentarz przy
            // analogicznej pętli w konstruktorze MainWindow.
            if (playerVector[x])
                playerVector[x]->stop();
        }

        ItemModel->clear();
        ItemModel->setRowCount(table->rowCount());
        ItemModel->setColumnCount(table->columnCount());
        for(int row = 0; row < table->rowCount(); row++){
            for(int col = 0; col < table->columnCount(); col++){
                QString text = table->item(row,col) ?table->item(row,col)->text():QString();
                qDebug()<< text;
                QStandardItem *item = new QStandardItem(text);
                item->setTextAlignment(Qt::AlignCenter);
                ItemModel->setItem(row, col, item);
            }
        }
        qDebug()<< adreshttp;
        bool ok = zapiszKameryDat("http://" + adreshttp + ":8080/kamery.dat");
    //    mtx->stopMtx();
    //    QTimer::singleShot(2000,[this](){mtx->startMtx();});
        if(ok){
            QMessageBox::information(nullptr,"INFO","DANE ZAPISANE");

        }else{
            QMessageBox::information(nullptr,"UWAGA","DANE NIE ZAPISANE");
        }
    });
    connect(btnDodaj, &QPushButton::clicked, widget,[table,labelDK02,stack,widgetDodajKam,
                        lineEditNazwa,labelDkRozdz2,labelFps2,lineEditAdres,
                        spinBoxIleDni,spinBoxCzulosc,lineEditPath,path](){
        stack->setCurrentWidget(widgetDodajKam);
        QString numer = QString::number(table->rowCount());
        labelDK02->setText(numer);
        lineEditNazwa->setFocus();
        labelDkRozdz2->setText("");
        labelFps2->setText("");
        lineEditNazwa->setText("");
        lineEditAdres->setText("");
        lineEditPath->setText("");
        spinBoxIleDni->setValue(7);
        spinBoxCzulosc->setValue(10000);
    });
    connect(btnModyfikuj, &QPushButton::clicked, widget, [table,stack,widgetDodajKam,
        labelDK02,lineEditNazwa,lineEditAdres,labelDkRozdz2,labelFps2,lineEditPath,spinBoxIleDni,
        spinBoxCzulosc,comboBoxDetekcja](){
        stack->setCurrentWidget(widgetDodajKam);
        int row = table->currentRow();
        for(int x = 0; x < table->columnCount(); x++){
            QString text = table->item(row, x)
                    ?table->item(row, x)->text()
                    :QString();
            if(x == 0)
                labelDK02->setText(text);
            if(x == 1)
                lineEditNazwa->setText(text);
            if(x == 2)
                lineEditAdres->setText(text);
            if(x == 3)
                labelDkRozdz2->setText(text);
            if(x == 4)
                labelFps2->setText(text);
            if(x == 5)
                lineEditPath->setText(text);
            if(x == 6)
                spinBoxIleDni->setValue(text.toInt());
            if(x == 7)
                spinBoxCzulosc->setValue(text.toInt());
            if(x == 8){
                int index = comboBoxDetekcja->findData(text, Qt::UserRole);
                if (index >= 0){
                comboBoxDetekcja->setCurrentIndex(index);
                }
            }
        }
    });
    connect(btnUsun, &QPushButton::clicked, widget, [table](){
        int row = table->currentRow();
        table->removeRow(row);
        for(int x = 0; x < table->rowCount(); x++){
            QTableWidgetItem *item = new QTableWidgetItem(QString::number(x));
            item->setTextAlignment(Qt::AlignCenter);
            table->setItem(x,0, item);
        }
    });
    connect(btnAnuluj, &QPushButton::clicked, widget, [stack](){
        stack->close();
        stack->deleteLater();
    });
    connect(this, &MainWindow::sygnalResize,widget,[this,stack](){
        stack->setGeometry(0,0,this->width(),this->height());
    });
    // connect(btnZapisz, &QPushButton::clicked,widgetDodajKam,[stack,widget,labelDK02,
    //         lineEditNazwa,lineEditAdres,labelDkRozdz2,labelFps2,lineEditPath,spinBoxIleDni,
    //         spinBoxCzulosc,comboBoxDetekcja,table](){

    //     if(lineEditNazwa->text().trimmed().isEmpty()
    //         || lineEditAdres->text().trimmed().isEmpty()
    //         || lineEditPath->text().trimmed().isEmpty()){
    //         lineEditNazwa->setFocus();
    //         QMessageBox::information(nullptr,"UEAGA","POLA:\n-KAMERA\n-ADRES KAMERY\nNIE MOGĄ BYĆ PUSTE");
    //         return;
    //     }
    //     int row = labelDK02->text().toInt();
    //     for(int x = 0; x < table->rowCount(); x++)
    //     {
    //         if(x != row){
    //             QString nazwa = table->item(x,1)->text();
    //             if(nazwa == lineEditNazwa->text().trimmed())
    //             {
    //                 QMessageBox::information(nullptr,"UWAGA",R"(
    //                     KAMERA O TEJ NAZWIE
    //                     JUŻ ISTNIEJE
    //                     ZMIEŃ NAZWĘ KAMERY)");
    //                 return;
    //             }
    //         }
    //     }
    //     if(labelDK02->text().toInt() < table->rowCount()){
    //         // int row = labelDK02->text().toInt();
    //         // for(int x = 0; x < table->rowCount(); x++)
    //         // {
    //         //     if(x != row){
    //         //         QString nazwa = table->item(x,1)->text();
    //         //         if(nazwa == lineEditNazwa->text().trimmed())
    //         //         {
    //         //             QMessageBox::information(nullptr,"UWAGA",R"(
    //         //             KAMERA O TEJ NAZWIE
    //         //             JUŻ ISTNIEJE
    //         //             ZMIEŃ NAZWĘ KAMERY)");
    //         //             return;
    //         //         }
    //         //     }
    //         // }
    //         QStringList lista;

    //         lista = {labelDK02->text().trimmed(),lineEditNazwa->text().trimmed(),
    //             lineEditAdres->text().trimmed(),labelDkRozdz2->text().trimmed(),
    //             labelFps2->text().trimmed(), lineEditPath->text().trimmed(),
    //             spinBoxIleDni->text().trimmed(),spinBoxCzulosc->text().trimmed(),
    //             comboBoxDetekcja->currentData(Qt::UserRole).toString()
    //         };
    //         for(int x = 0; x < table->columnCount(); x++){
    //             QTableWidgetItem *item = new QTableWidgetItem(lista[x]);
    //             item->setTextAlignment(Qt::AlignCenter);
    //             table->setItem(row, x, item);
    //             stack->setCurrentWidget(widget);
    //         }
    //     }else{
    //         // for(int x = 0; x < table->rowCount(); x++){
    //         //     QString nazwa = table->item(x,1)->text();
    //         //     if(nazwa == lineEditNazwa->text().trimmed())
    //         //     {
    //         //         QMessageBox::information(nullptr,"UWAGA",R"(
    //         //         KAMERA O TEJ NAZWIE
    //         //         JUŻ ISTNIEJE
    //         //         ZMIEŃ NAZWĘ KAMERY)");
    //         //         return;
    //         //     }
    //         // }
    //         int row = labelDK02->text().toInt();
    //         QStringList lista;
    //         lista = {labelDK02->text().trimmed(),lineEditNazwa->text().trimmed(),
    //                  lineEditAdres->text().trimmed(), labelDkRozdz2->text().trimmed(),
    //                  labelFps2->text().trimmed(), lineEditPath->text().trimmed(),
    //                  spinBoxIleDni->text().trimmed(),spinBoxCzulosc->text().trimmed(),
    //                   comboBoxDetekcja->currentData(Qt::UserRole).toString()
    //         };
    //         table->insertRow(row);
    //         for(int x = 0; x < table->columnCount(); x++){
    //             QTableWidgetItem *item = new QTableWidgetItem(lista[x]);
    //             item->setTextAlignment(Qt::AlignCenter);
    //             table->setItem(row, x, item);
    //             stack->setCurrentWidget(widget);
    //         }
    //     }
    // });
    connect(btnTest, &QPushButton::clicked, widgetDodajKam,[this,lineEditAdres,table,
            labelDK02,lineEditNazwa,lineEditPath,labelFps2, labelDkRozdz2,
            spinBoxIleDni,spinBoxCzulosc,comboBoxDetekcja,stack,widget](){
        if(lineEditNazwa->text().trimmed().isEmpty()
            || lineEditAdres->text().trimmed().isEmpty()
            || lineEditPath->text().trimmed().isEmpty()){
            lineEditNazwa->setFocus();
            QMessageBox::information(nullptr,"UEAGA","POLA:\n-KAMERA\n-ADRES KAMERY\nNIE MOGĄ BYĆ PUSTE");
            return;
        }
        int row = labelDK02->text().toInt();
        for(int x = 0; x < table->rowCount(); x++)
        {
            if(x != row){
                QString nazwa = table->item(x,1)->text();
                if(nazwa == lineEditNazwa->text().trimmed())
                {
                    QMessageBox::information(nullptr,"UWAGA",R"(
                        KAMERA O TEJ NAZWIE
                        JUŻ ISTNIEJE
                        ZMIEŃ NAZWĘ KAMERY)");
                    return;
                }
            }
        }
        // POPRAWKA (blokada duplikatu adresu): większość tanich kamer
        // IP/MJPEG po HTTP (np. "videostream.cgi") obsługuje TYLKO JEDNO
        // jednoczesne połączenie, a nawet dla RTSP dodanie tej samej kamery
        // dwa razy nie ma sensu (dubluje obciążenie łącza/kamery i zapis
        // nagrań). Dodanie tej samej kamery pod dwiema różnymi nazwami
        // powodowało wcześniej trudny do zdiagnozowania błąd "404 Not
        // Found" dopiero przy próbie odtwarzania (drugi wewnętrzny
        // publisher MediaMTX nigdy nie mógł się połączyć - kamera zajęta
        // przez pierwszego). Blokujemy to na starcie, tak jak duplikat
        // nazwy powyżej - bez możliwości zapisania.
        {
            QString noweAdres = lineEditAdres->text().trimmed();
            for (int x = 0; x < table->rowCount(); x++) {
                if (x != row && table->item(x, 2)
                    && table->item(x, 2)->text().trimmed() == noweAdres) {
                    QString istniejacaNazwa = table->item(x, 1)
                                                   ? table->item(x, 1)->text()
                                                   : QString();
                    QMessageBox::information(
                        nullptr, "UWAGA",
                        QString("KAMERA O TAKIM ADRESIE JUŻ ISTNIEJE\n"
                                "(\"%1\")\n"
                                "ZMIEŃ ADRES KAMERY").arg(istniejacaNazwa));
                    return;
                }
            }
        }
        qDebug()<<"1";
        auto [ok, resolution, fps] = ffprobeTest(lineEditAdres->text().trimmed());
        if(!ok){
            QMessageBox::information(nullptr,"INFO","SPRAWDŹ:\n- POPRAWNOŚĆ STRUMIENIA\n- ŁĄCZNOŚĆ Z KAMERĄ");
            return;
        }
        qDebug()<<"2";
        QList list = fps.split("/");
        labelDkRozdz2->setText("Size:"+resolution);
        labelFps2->setText("FPS:"+QString::number(list[0].toDouble()/list[1].toDouble(),'f',0)+"/s");
qDebug()<<"3";
        QWidget *testWidget = new QWidget(this);
        testWidget->setStyleSheet(R"(QWidget{
            background-color: white;
            border: 4px solid blue;
        })");
        testWidget->setAttribute(Qt::WA_DeleteOnClose);
        int x = this->width();
        int y = this->height();
        testWidget->setGeometry(0, 0, x, y);
        QVBoxLayout *layoutTestWidget = new QVBoxLayout(testWidget);
        layoutTestWidget->setContentsMargins(30,30,30,30);
        QLabel *label = new QLabel("POCZEKAJ\n"
                "JEŚLI OBRAZ NIE POKAŻE SIĘ W CIĄGU PARU SEKUND\n"
                "SPRAWDŹ ADRES STRUMIENIA",testWidget);
        QFont font = label->font();
        font.setPixelSize(24);
        label->setFont(font);
        label->setAlignment(Qt::AlignCenter);
        QHBoxLayout *layouth1 = new QHBoxLayout();
        QPushButton *btnStop = new QPushButton("STOP", testWidget);
        btnStop->setFixedWidth(150);
        btnStop->setStyleSheet(stylesheetPushButton);
        layouth1->addStretch(0);
        layouth1->addWidget(btnStop);
        layouth1->addStretch(0);

        layoutTestWidget->addWidget(label);
        layoutTestWidget->addLayout(layouth1);
qDebug()<<"4";

        if(!playerek){
            qDebug()<< "brak playerek" << playerek;
            playerek = new FfmpegPlayer(this);
            qDebug()<<"nowy playerek";
        }
qDebug()<<"5"<<lineEditAdres->text().trimmed();
        playerek->setLabel(label);
        playerek->setUrl(lineEditAdres->text().trimmed());
        playerek->setAspectRatioMode(Qt::KeepAspectRatio);
        playerek->play();
        testWidget->show();
qDebug()<<"6";
        connect(this, &MainWindow::sygnalResize,testWidget,[this,testWidget](){
            int x = this->width();
            int y = this->height();
            testWidget->setGeometry(0, 0, x, y);
        });

    //    QPointer<FfmpegPlayer> player = playerek;
        connect(btnStop, &QPushButton::clicked, testWidget,[this,testWidget,labelDK02,
                lineEditNazwa,lineEditAdres,labelDkRozdz2,labelFps2,lineEditPath,
                spinBoxIleDni,spinBoxCzulosc,comboBoxDetekcja,table,row,stack,widget](){
            if(labelDK02->text().toInt() < table->rowCount()){
                QStringList lista;
                lista = {labelDK02->text().trimmed(),lineEditNazwa->text().trimmed(),
                    lineEditAdres->text().trimmed(),labelDkRozdz2->text().trimmed(),
                    labelFps2->text().trimmed(), lineEditPath->text().trimmed(),
                    spinBoxIleDni->text().trimmed(),spinBoxCzulosc->text().trimmed(),
                    comboBoxDetekcja->currentData(Qt::UserRole).toString()
                };
                for(int x = 0; x < table->columnCount(); x++){
                    QTableWidgetItem *item = new QTableWidgetItem(lista[x]);
                    item->setTextAlignment(Qt::AlignCenter);
                    table->setItem(row, x, item);
                    stack->setCurrentWidget(widget);
                }
            }else{
                int row = labelDK02->text().toInt();
                QStringList lista;
                lista = {labelDK02->text().trimmed(),lineEditNazwa->text().trimmed(),
                    lineEditAdres->text().trimmed(), labelDkRozdz2->text().trimmed(),
                    labelFps2->text().trimmed(), lineEditPath->text().trimmed(),
                    spinBoxIleDni->text().trimmed(),spinBoxCzulosc->text().trimmed(),
                    comboBoxDetekcja->currentData(Qt::UserRole).toString()
                };
                table->insertRow(row);
                for(int x = 0; x < table->columnCount(); x++){
                    QTableWidgetItem *item = new QTableWidgetItem(lista[x]);
                    item->setTextAlignment(Qt::AlignCenter);
                    table->setItem(row, x, item);
                    stack->setCurrentWidget(widget);
                }
            }
            if (playerek && playerek->isPlaying()) {
                playerek->stop();
                playerek->deleteLater();
                playerek = nullptr;
            }
            testWidget->close();
            testWidget->deleteLater();
        });
    });
    // connect(btnStrefyRuchu, &QPushButton::clicked, widgetDodajKam,[this,lineEditAdres](){

    //     auto [ok, resolution, fps] = ffprobeTest(lineEditAdres->text().trimmed());

    //     if (ok) {
    //         qDebug() << "Resolution:" << resolution;
    //         QStringList list = fps.split("/");
    //         double licznik = list[0].toDouble();
    //         double mianownik = list[1].toDouble();
    //         double fpsDouble = licznik/mianownik;
    //         qDebug()<< fps << fpsDouble;
    //     } else {
    //         qDebug() << "Błąd RTSP";
    //     }
    // });
    connect(btnCancel, &QPushButton::clicked,widgetDodajKam,[stack,widget](){
        stack->setCurrentWidget(widget);
    });
}

std::tuple<bool, QString, QString> MainWindow::ffprobeTest(const QString &rtspUrl)
{
    // DROBNA POPRAWKA (spójność): "-rtsp_transport" jest opcją tylko
    // demuxera RTSP - dla adresów HTTP/MJPEG niektóre wersje ffprobe mogą
    // zgłosić błąd "Option not found" (tak jak zawsze robi to ffmpeg -
    // patrz analogiczna poprawka w FfmpegPlayer::probeFrameSize()).
    QProcess process;
    QStringList args = {"-v", "error"};
    if (rtspUrl.startsWith("rtsp://", Qt::CaseInsensitive))
        args << "-rtsp_transport" << "tcp";
    args += QStringList{
        "-timeout", "5000000",
        "-select_streams", "v:0",
        "-show_entries", "stream=width,height,r_frame_rate",
        "-of", "default=noprint_wrappers=1:nokey=1",
        rtspUrl
    };

    process.start("ffprobe", args);

    if (!process.waitForStarted(3000))
        return {false, "", ""};

    if (!process.waitForFinished(8000)) {
        process.kill();
        process.waitForFinished();
        return {false, "", ""};
    }

    if (process.exitStatus() != QProcess::NormalExit)
        return {false, "", ""};

    if (process.exitCode() != 0)
        return {false, "", ""};

    QString wynik = QString::fromUtf8(
                        process.readAllStandardOutput()
                        ).trimmed();

    QStringList dane = wynik.split('\n', Qt::SkipEmptyParts);

    if (dane.size() < 3)
        return {false, "", ""};

    QString resolution = dane[0] + "x" + dane[1];
    QString fps = dane[2];

    return {true, resolution, fps};
}

// KRYTYCZNA POPRAWKA BEZPIECZEŃSTWA: HttpSerwer wymaga teraz nagłówka
// "X-Auth-Token" do odczytu/zapisu plików .dat (patrz httpserwer.h/.cpp).
// Ta metoda dostarcza właściwy token w zależności od tego, czy łączymy się
// z własnym, lokalnym serwerem (token znamy automatycznie), czy z serwerem
// innej instancji aplikacji w sieci (token trzeba podać ręcznie raz -
// zapisujemy go potem w QSettings, żeby nie pytać przy każdym połączeniu).
QString MainWindow::resolveAuthTokenForHost(const QUrl &url)
{
    const QString host = url.host();
    if (host == "localhost" || host == "127.0.0.1") {
        return httpSerwer ? httpSerwer->authToken() : QString();
    }

    QSettings settings("MojaFirma", "CameraSerwer");
    const QString key = "remoteAuthToken_" + host;
    QString saved = settings.value(key).toString();
    if (!saved.isEmpty())
        return saved;

    bool ok = false;
    QString entered = QInputDialog::getText(
        this,
        "Token uwierzytelniający",
        QString("Serwer %1 wymaga tokenu dostępu (nagłówek X-Auth-Token).\n"
                "Skopiuj go z pliku .http_auth_token w katalogu aplikacji\n"
                "na urządzeniu, z którym się łączysz:").arg(host),
        QLineEdit::Normal, QString(), &ok);
    if (ok && !entered.trimmed().isEmpty()) {
        entered = entered.trimmed();
        settings.setValue(key, entered);
        return entered;
    }
    return QString();
}

bool MainWindow::czytajKameryDat(const QString &adres)
{
    if (!ItemModel)
    { ItemModel = new QStandardItemModel();
    }
    else {
        ItemModel->clear();
    }
    QNetworkAccessManager manager;
    QNetworkRequest request((QUrl(adres)));
    QString authToken = resolveAuthTokenForHost(QUrl(adres));
    if (!authToken.isEmpty())
        request.setRawHeader("X-Auth-Token", authToken.toUtf8());
    else
        qWarning() << "MainWindow::czytajKameryDat: brak tokenu dla" << adres
                   << "- serwer prawdopodobnie odpowie 401 Unauthorized";
    QNetworkReply *reply = manager.get(request);
    // POWAŻNA POPRAWKA (zawieszenie UI): wcześniej ta zagnieżdżona
    // QEventLoop nie miała żadnego limitu czasu - jeśli serwer pod `adres`
    // nie odpowiadał (np. zapora sieciowa cicho odrzucała pakiety), cała
    // aplikacja wisiała w nieskończoność. Dodajemy awaryjny timer 8s, który
    // przerywa oczekiwanie i traktuje to jak błąd sieci.
    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timeoutTimer.start(8000);
    loop.exec();
    if (!reply->isFinished()) {
        // Timer wygasł zanim reply się zakończył - realny timeout.
        reply->abort();
    }
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << reply->errorString();
        QMessageBox::information( this, "INFO", reply->errorString() +
                    "\nPRZYCZYNY:\n"
                    "- zły adres serwera\n"
                    "- brak internetu\n"
                    "- serwer nie uruchomiony" );
        reply->deleteLater();
        ItemModel->clear();
        return false;
    }
    QByteArray data = reply->readAll();
    QDataStream stream(data);
    stream.setVersion(QDataStream::Qt_6_0);
    qint32 n, m;
    stream >> n >> m;
    if (stream.status() != QDataStream::Ok) {
        qDebug() << "Błąd odczytu kamery.dat";
        reply->deleteLater();
        ItemModel->clear();
        return false;
    }
    ItemModel->setRowCount(n);
    ItemModel->setColumnCount(m);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            QString tekst; stream >> tekst;
            QStandardItem *item = new QStandardItem(tekst);
            item->setTextAlignment(Qt::AlignCenter);
            ItemModel->setItem(i, j, item);
        }
    }
    reply->deleteLater();
    return true;
}

bool MainWindow::zapiszKameryDat(const QString &adres)
{
    if (!ItemModel) {
        qDebug() << "Brak ItemModel";
        return false;
    }
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);

    qint32 n = ItemModel->rowCount();
    qint32 m = ItemModel->columnCount();

    stream << n << m;

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {

            QString tekst;

            QStandardItem *item = ItemModel->item(i, j);

            if (item)
                tekst = item->text();

            stream << tekst;
        }
    }

    if (stream.status() != QDataStream::Ok) {
        qDebug() << "Błąd tworzenia danych kamery.dat";
        return false;
    }

    QNetworkAccessManager manager;

    QNetworkRequest request((QUrl(adres)));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/octet-stream");
    QString authToken = resolveAuthTokenForHost(QUrl(adres));
    if (!authToken.isEmpty())
        request.setRawHeader("X-Auth-Token", authToken.toUtf8());
    else
        qWarning() << "MainWindow::zapiszKameryDat: brak tokenu dla" << adres
                   << "- serwer prawdopodobnie odpowie 401 Unauthorized";

    QNetworkReply *reply = manager.put(request, data);

    // POWAŻNA POPRAWKA (zawieszenie UI): jak w czytajKameryDat() - dodajemy
    // awaryjny timeout, żeby brak odpowiedzi zdalnego serwera nie zawieszał
    // aplikacji w nieskończoność.
    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

    connect(reply, &QNetworkReply::finished,
            &loop, &QEventLoop::quit);

    timeoutTimer.start(8000);
    loop.exec();

    if (!reply->isFinished()) {
        reply->abort();
    }

    if (reply->error() != QNetworkReply::NoError) {

        qDebug() << "Błąd zapisu:" << reply->errorString();

        QMessageBox::information(
            this,
            "INFO",
            reply->errorString() +
                "\nPRZYCZYNY:\n"
                "- zły adres serwera\n"
                "- brak internetu\n"
                "- serwer nie uruchomiony"
            );

        reply->deleteLater();
        return false;
    }

    qDebug() << "kamery.dat zapisany:"
             << n << "wierszy,"
             << m << "kolumn";

    reply->deleteLater();

    return true;
}

QIcon MainWindow::createGridIcon(int rows, int cols)
{
    const int size = 32;

    QPixmap pix(size, size);
    pix.fill(Qt::transparent);

    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QPen(Qt::black, 1));
    p.setBrush(Qt::NoBrush);

    int margin = 2;
    int w = (size - 2 * margin - (cols - 1) * 2) / cols;
    int h = (size - 2 * margin - (rows - 1) * 2) / rows;

    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            p.drawRect(
                margin + c * (w + 2),
                margin + r * (h + 2),
                w,
                h);

    return QIcon(pix);
}

void MainWindow::onMenuItemSerwerClicked(QListWidgetItem *item)
{
    //QString text = item->text();
    //if(text == "Start serwer rtsp i http" || text == "Zatrzymaj serwer rtsp i http"){
    if(item->data(Qt::UserRole).toString() == "StartStop"){
        if (!httpSerwer) {
            httpSerwer = new HttpSerwer(this);
        }
        if (!httpSerwer->isRunning()) {
            QDir homeDir(appHomePath);
            if (!homeDir.exists()) {
                qDebug() << "appHomePath nie istnieje, tworzę:" << appHomePath;
                homeDir.mkpath(appHomePath);
            }

            if (httpSerwer->start(appHomePath, 8080)) {
                item->setText("ZATRZYMAJ SERWER RTSP i HTTP");
                qDebug() << "Serwer HTTP wystartował na porcie" << httpSerwer->serverPort();
                mtx->ensureInstalled();
            } else {
                QMessageBox::warning(this, "Błąd serwera HTTP",
                    "Nie udało się uruchomić serwera HTTP (port może być zajęty).");
            }
        } else {
            httpSerwer->stop();
            mtx->stopMtx();
            item->setText("START SERWER RTSP I HTTP");
            qDebug() << "Serwer HTTP zatrzymany";
        }
    }else if(item->data(Qt::UserRole).toString() == "szukaj kamer"){     //(text == "Szukaj kamer"){
        ukryjPokazPanelSerwer();
        QDir homeDir;//(appHomePath);
        homeDir = appHomePath;
        if(!homeDir.exists()){
            qDebug()<<"homeDir nie istnieje, tworzę:" <<homeDir.absolutePath();
            homeDir.mkpath(homeDir.absolutePath());
        }else{
            qDebug()<< homeDir.absolutePath() << "istnieje";
        }
        int width = centralWidget->width();
         FindNewCamera *newCamera = new FindNewCamera(this);
        newCamera->resize(width, height());
        newCamera->move(0,0);
        newCamera->setStyleSheet("background:white;");
        newCamera->setParent(centralWidget);
        newCamera->show();
//      statusBar()->hide();
        toolbar->hide();
        connect(this, &MainWindow::sygnalResize, newCamera, [=](){
            newCamera->resize(centralWidget->width(), centralWidget->height());
            //newCamera->move(centralWidget->rect().center()-newCamera->rect().center());
        });
        connect(newCamera, &FindNewCamera::requestCloseFrame, this, [this]() {
//          statusBar()->show();
            toolbar->show();
            });
    }else if(item->data(Qt::UserRole).toString() == "USTAWIENIA"){
        qDebug()<< "tutaj będę pisał";
        QDialog *dialog = new QDialog();
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowTitle("ADRES SERWERA");
        QVBoxLayout *layoutDialog = new QVBoxLayout(dialog);
        QHBoxLayout *layouth1 = new QHBoxLayout();
        QLabel *labelAdres = new QLabel("ADRES IP SERWERA:",dialog);
        labelAdres->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        labelAdres->setStyleSheet(stylesheetLabelSelectedBlue);
        czytajSerweryDat();
        QComboBox *comboBox = new QComboBox(dialog);
        comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        for(int row = 0; row < ItemModelSerweryDat->rowCount();row++){
            QString nazwa = ItemModelSerweryDat->item(row,1)->text();
            QString adres = ItemModelSerweryDat->item(row,2)->text();

            comboBox->addItem(nazwa + " - " + adres, adres);
        }
        comboBox->setStyleSheet(stylesheetComboBox);
        int w = qMax(labelAdres->sizeHint().width(),
                     comboBox->sizeHint().width());
        labelAdres->setFixedWidth(w);
        comboBox->setFixedWidth(w);
        layouth1->addWidget(labelAdres,1);
        layouth1->addWidget(comboBox,1);
        QHBoxLayout *layouth2 = new QHBoxLayout();
        QPushButton *btnOk = new QPushButton("OK",dialog);
        btnOk->setIcon(QIcon(":/icons/ok.svg"));
        btnOk->setIconSize(QSize(32,32));
        btnOk->setStyleSheet(stylesheetPushButton);
        QPushButton *btnAnuluj = new QPushButton("ANULUJ",dialog);
        btnAnuluj->setIcon(QIcon(":/icons/anuluj.svg"));
        btnAnuluj->setIconSize(QSize(32,32));
        btnAnuluj->setStyleSheet(stylesheetPushButtonRed);
        layouth2->addWidget(btnOk,1);
        layouth2->addWidget(btnAnuluj,1);
        layoutDialog->addLayout(layouth1);
        layoutDialog->addLayout(layouth2);
        dialog->show();
        connect(btnOk, &QPushButton::clicked, dialog, [this,comboBox, dialog]() {
            QString adres = comboBox->currentData().toString();
            adreshttp = adres;
            if (czytajKameryDat("http://" + adres + ":8080/kamery.dat")) {
                 if (ItemModel && ItemModel->rowCount() > 0) {
                     createWidgetUstawienia(); dialog->close();
                }
                else{
                     QMessageBox::information( dialog, "INFO", "Serwer działa, ale nie znaleziono żadnych kamer." );
                }
            }
        });
        connect(btnAnuluj, &QPushButton::clicked, dialog, [dialog](){
            dialog->close();
        });
    }else if(item->data(Qt::UserRole).toString() == "TOKEN"){
        QFile file(QDir(appHomePath).filePath(".http_auth_token"));
        QString tokentext;
        if(file.open(QIODevice::ReadOnly)){
            QTextStream stream(&file);
            tokentext = stream.readAll();

            qDebug()<<"PRZECZYTAŁEM";
        }else{
            QMessageBox::information(nullptr,"UWAGA",
                "NIE MOŻNA OTWORZYĆ PLIKU\nERROR:"+file.errorString());
            return;
        }
        file.close();

        QDialog *dialog = new QDialog(this);
        dialog->setFixedWidth(400);
        dialog->setWindowTitle("TWÓJ TOKEN:");
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        QVBoxLayout *dialoglayout = new QVBoxLayout(dialog);
        QLabel *label = new QLabel(dialog);
        label->setText("token jest potrzebny do logowania\nze zdalnego komputera\n"
                       "do tego serwera");
        QLineEdit *lineEdit = new QLineEdit();
        lineEdit->setAlignment(Qt::AlignCenter);
        lineEdit->setReadOnly(true);
        lineEdit->setText(tokentext);
        QFont font = lineEdit->font();
        font.setPointSize(12);
        font.setBold(true);
        lineEdit->setFont(font);

        QHBoxLayout *layouth1 = new QHBoxLayout();
        QPushButton *btnSkopiuj = new QPushButton("SKOPIUJ", dialog);
        btnSkopiuj->setIcon(QIcon(":/icons/kopiuj.svg"));
        btnSkopiuj->setIconSize(QSize(24,24));
        btnSkopiuj->setStyleSheet(stylesheetPushButton);
        QPushButton *btnZamknij = new QPushButton("ZAMKNIJ", dialog);
        btnZamknij->setIcon(QIcon(":/icons/zamknij.svg"));
        btnZamknij->setIconSize(QSize(24,24));
        btnZamknij->setStyleSheet(stylesheetPushButtonRed);
        layouth1->addStretch(0);
        layouth1->addWidget(btnSkopiuj);
        layouth1->addWidget(btnZamknij);
        layouth1->addStretch(0);

        dialoglayout->addWidget(label);
        dialoglayout->addWidget(lineEdit);
        dialoglayout->addSpacing(50);
        dialoglayout->addLayout(layouth1);
        dialog->show();
        connect(btnZamknij, &QPushButton::clicked, dialog, [dialog](){
            dialog->close();
        });
        connect(btnSkopiuj, &QPushButton::clicked, dialog, [dialog,lineEdit,btnSkopiuj](){
            if (!lineEdit->text().isEmpty()) {
                QGuiApplication::clipboard()->setText(lineEdit->text());
                QPoint globalPos = btnSkopiuj->mapToGlobal(QPoint(btnSkopiuj->width() / 2, -30));
            //    QToolTip::showText(globalPos, "Skopiowano!",nullptr);
                QLabel *dymek = new QLabel(" Skopiowano! ", dialog, Qt::ToolTip | Qt::BypassWindowManagerHint);
                QPointer<QLabel> safeDymek = dymek;
                dymek->move(globalPos);
                dymek->show();
                QTimer::singleShot(3000, [safeDymek]() {
                    //dymek->deleteLater();
                    if (safeDymek) {
                        safeDymek->deleteLater();
                    }
                });
            }else{
                QPoint globalPos = btnSkopiuj->mapToGlobal(QPoint(btnSkopiuj->width() / 2, -30));
                QLabel *dymek = new QLabel(" Pole jest puste\nnie skopiowano ", nullptr, Qt::ToolTip | Qt::BypassWindowManagerHint);
                dymek->move(globalPos);
                dymek->show();
                QTimer::singleShot(3000, [dymek]() {
                    dymek->deleteLater();
                });
            }
        });
    }else if(item->data(Qt::UserRole).toString() == "IKONAPULPITU"){
        QString iconPath = appHomePath + "/MultiCamIp.png";
        if (!QFile::exists(iconPath)) {
            if (!QFile::copy(":/icons/camera.png", iconPath))
                qWarning() << "Nie udało się skopiować ikony do" << iconPath;
        }
        QString aplicationPath = QCoreApplication::applicationFilePath();
        QFileInfo fileInfo(aplicationPath);
        QString nazwaPliku = fileInfo.baseName();
        nazwaPliku.append(".desktop");
        nazwaPliku.prepend("/");
        qDebug() << nazwaPliku;
        // DROBNA POPRAWKA: QStandardPaths::writableLocation() może w
        // rzadkich przypadkach (np. system bez skonfigurowanych
        // katalogów xdg-user-dirs) zwrócić pusty string - bez tego
        // sprawdzenia plik trafiłby do "/MultiCamIp.desktop" (katalog
        // główny systemu plików) zamiast normalnie się nie udać z
        // czytelnym komunikatem.
        QString desktopDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        if (desktopDir.isEmpty()) {
            QMessageBox::warning(this, "UWAGA",
                "Nie udało się ustalić katalogu Pulpitu w tym systemie.\n"
                "Skrót nie został utworzony.");
            return;
        }
        QString desktopPath = desktopDir + nazwaPliku;  //"/MultiCamIp.desktop";
        if (QFile::exists(desktopPath)) {
            QFile::remove(desktopPath);
        }
        QFile file(desktopPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)){
            QMessageBox::warning(this, "UWAGA",
                "Nie udało się zapisać skrótu:\n" + desktopPath);
            return;
        }
        QTextStream DesktopIkon(&file);
        DesktopIkon << "[Desktop Entry]\n";
        DesktopIkon << "Type=Application\n";
        DesktopIkon << "Name=MultiCamIp\n";
        DesktopIkon << "Comment=Uruchamia aplikację CameraSerwer\n";
        DesktopIkon << "Exec=" <<aplicationPath <<"\n";
        //DesktopIkon << "Icon=/home/sobolewski/projekt/MultiCamIp.png\n";
        DesktopIkon << "Icon=" << iconPath << "\n";
        DesktopIkon << "Terminal=false\n";
        DesktopIkon << "Categories=Utility;Video;Camera;\n";
        DesktopIkon << "StartupWMClass=MultiCamIp\n";
        file.close();
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                            QFileDevice::ReadGroup | QFileDevice::ExeGroup |
                            QFileDevice::ReadOther | QFileDevice::ExeOther);

        QString menuDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/applications";
        QString menuPath = menuDir + nazwaPliku;  //"/multicamip.desktop";
        QDir().mkpath(menuDir);
        if (QFile::exists(menuPath)) {
            QFile::remove(menuPath);
        }
        if (QFile::copy(desktopPath, menuPath)) {
            QFile menuFile(menuPath);
            menuFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                                    QFileDevice::ReadGroup | QFileDevice::ExeGroup |
                                    QFileDevice::ReadOther | QFileDevice::ExeOther);
        }

        #if defined(Q_OS_LINUX)
        QProcess::startDetached("gio", QStringList() << "set" << "-t" << "string" << desktopPath << "metadata::trusted" << "true");
        #endif

        QMessageBox::information(this, "OK",
            "Skrót do pulpitu utworzony:\n" + desktopPath);
    }
}

void MainWindow::onMenuItemPodgladClicked(QListWidgetItem *item)
{
    qDebug()<< "działa";
    QString text = item->text();
    if(text == "Live stream"){
        qDebug()<< "działa";
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (centralWidget) {
        emit sygnalResize();
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Enter) {
        QWidget *w = qobject_cast<QWidget*>(obj);
        bool pokaz = w->property("powieksz").toBool();
        if (w) {
            QWidget *kameraWidget = nullptr;
            if (kameraWidgetVector.contains(w)) {
                kameraWidget = w;
            } else {
                for (QWidget *kw : std::as_const(kameraWidgetVector)) {
                    if (kw && kw->isAncestorOf(w)) {
                        kameraWidget = kw; break;
                    }
                }
            }
            if (kameraWidget) {
                QGroupBox *gbox = kameraWidget->findChild<QGroupBox*>();
                if (gbox && pokaz == true) {
                    // Aktualizujemy geometrię - pasek na dole kameraWidget
                    gbox->setGeometry(0, kameraWidget->height() - 30,
                                      kameraWidget->width(), 30);
                    gbox->raise();
                    gbox->show();
                }
            }
        }
    }

    if (event->type() == QEvent::Leave) {
        // Sprawdzamy czy obiekt to kameraWidget, labelVideo lub gboxPasekWlasciwosci.
        // Leave odpala się gdy mysz wychodzi z danego widgetu - ale może wchodzić
        // na dziecko tego widgetu (np. z labelVideo na gboxPasekWlasciwosci).
        // Sprawdzamy globalną pozycję kursora - jeśli nadal jest w obszarze
        // kameraWidget (włącznie z dziećmi), pasek pozostaje widoczny.

        // Znajdź kameraWidget powiązany z obiektem który dostał Leave
        QWidget *kameraWidget = nullptr;
        if (kameraWidgetVector.contains(qobject_cast<QWidget*>(obj))) {
            kameraWidget = qobject_cast<QWidget*>(obj);
        } else {
            // Sprawdź czy obj jest dzieckiem któregoś kameraWidget (labelVideo, gbox, itp.)
            QWidget *w = qobject_cast<QWidget*>(obj);
            if (w) {
                for (QWidget *kw : std::as_const(kameraWidgetVector)) {
                    if (kw && (w == kw || kw->isAncestorOf(w))) {
                        kameraWidget = kw;
                        break;
                    }
                }
            }
        }

        if (kameraWidget) {
            QGroupBox *gbox = kameraWidget->findChild<QGroupBox*>();
            if (gbox && gbox->isVisible()) {
                // Sprawdzamy globalną pozycję - czy mysz jest poza kameraWidget
                QPoint globalCursor = QCursor::pos();
                QRect globalRect(kameraWidget->mapToGlobal(QPoint(0,0)),
                                 kameraWidget->size());
                if (!globalRect.contains(globalCursor)) {
                    gbox->hide();
                }
            }
        }
    }

    if (event->type() == QEvent::MouseButtonPress) {
    //    QLabel *labelVideo = qobject_cast<QLabel*>(obj);
        QWidget *widgetVideo = qobject_cast<QWidget*>(obj);
    bool pokaz = widgetVideo->property("powieksz").toBool();
        if (widgetVideo) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                if(pokaz == true){
                    qDebug()<< true;

                if (powiekszonyWidget == nullptr) {
                    // --- TRYB POWIĘKSZENIA ---
                    // Chowamy wszystkie kameraWidgety poza klikniętym
                    for (int i = 0; i < grid->count(); i++) {
                        QWidget *w = grid->itemAt(i)->widget();
                        if (w && w != widgetVideo)
                            w->hide();
                    }
                    // Rozciągamy kameraWidget (który zawiera labelVideo i gbox)
                    // na całą siatkę - gbox pozostaje wewnątrz i działa normalnie
                    int maxRow = 0, maxCol = 0;
                    for (int i = 0; i < grid->count(); i++) {
                        int r, c, rs, cs;
                        grid->getItemPosition(i, &r, &c, &rs, &cs);
                        maxRow = qMax(maxRow, r + rs);
                        maxCol = qMax(maxCol, c + cs);
                    }
                    grid->removeWidget(widgetVideo);
                    grid->addWidget(widgetVideo, 0, 0, maxRow, maxCol);
                    widgetVideo->show();

                    // Aktualizujemy geometrię gboxa po zmianie rozmiaru kameraWidget
                    // i pokazujemy go - w trybie powiększonym pasek jest zawsze widoczny
                    QGroupBox *gbox = widgetVideo->findChild<QGroupBox*>();
                    if (gbox) {
                        QTimer::singleShot(50, widgetVideo, [widgetVideo, gbox](){
                            gbox->setGeometry(0, widgetVideo->height() - 30,
                                              widgetVideo->width(), 30);
                            gbox->raise();
                            gbox->show();
                        });
                    }

                //    powiekszonyLabel = labelVideo;
                    powiekszonyWidget = widgetVideo;
                //    qDebug() << "Powiększono:" << labelVideo->text();

                }else
                {
                    // --- POWRÓT DO SIATKI ---

                    // Usuń powiększony widget z siatki
                    grid->removeWidget(powiekszonyWidget);

                    // Dodaj ponownie wszystkie widgety w ich oryginalne miejsca
                    for (QWidget *w : std::as_const(kameraWidgetVector))
                    {
                        if (!w)
                            continue;

                        int r = w->property("rowKamery").toInt();
                        int c = w->property("colKamery").toInt();

                        grid->addWidget(w, r, c, 1, 1);
                        w->show();
                        w->setMinimumSize(1, 1);
                        w->setSizePolicy(QSizePolicy::Expanding,
                                         QSizePolicy::Expanding);

                        if (QGroupBox *gb = w->findChild<QGroupBox*>())
                            gb->hide();
                    }

                    // Przywróć rozciąganie siatki
                    for (int i = 0; i < grid->rowCount(); ++i)
                        grid->setRowStretch(i, 1);

                    for (int i = 0; i < grid->columnCount(); ++i)
                        grid->setColumnStretch(i, 1);

                    grid->invalidate();
                    livePodgladWidget->updateGeometry();

                    QTimer::singleShot(50, this, [this]()
                        {
                            for (QWidget *w : std::as_const(kameraWidgetVector))
                                {
                                    if (!w)
                                         continue;
                                        if (QGroupBox *gbox = w->findChild<QGroupBox*>())
                                            {
                                                gbox->setGeometry(0,
                                                w->height() - 30,
                                                w->width(),
                                                30);
                                            }
                                }
                        });

                    powiekszonyWidget = nullptr;

                    qDebug() << "Powrót do siatki";
                }
                return true;
                }
                }
            }
        }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->key() ==  Qt::Key_F12){
        if (isFullScreen()){
            showNormal();
            toolbar->show();
        }
        else{
            showFullScreen();
            toolbar->hide();
        }
        return;
    }
    if(event->key() ==  Qt::Key_F11){
        if (isMaximized())
            showNormal();
        else{
            showMaximized();
        }
        return;
    }
    QMainWindow::keyPressEvent(event);
}
