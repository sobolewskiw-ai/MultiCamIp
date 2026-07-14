#include "mainwindow.h"
#include "findnewcamera.h"
#include "httpserwer.h"
#include "mediamtxmanager.h"
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
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoSink>
#include <QVideoFrame>
#include <QActionGroup>

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
            [this]( const QVector<QString> &wynik)
            {
                qDebug() << "Wersja :" << wynik[0];
                qDebug() << "Plik    :" << wynik[1];
                qDebug() << "URL     :" << wynik[2];
                mtx->pobieramMtxmanager(wynik[2],appHomePath+"/mediamtx/"+wynik[1]);
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
    setupUi();
}
//MainWindow::~MainWindow() = default;
MainWindow::~MainWindow() {
    // Rozłączamy sygnały mtx PRZED zatrzymaniem - lambdy z connect mogą
    // wskazywać na już-zniszczone widgety (np. QListWidgetItem *item)
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
    connect(toggleActionSerwer, &QAction::triggered, this,[this](){
        ukryjPokazPanelSerwer();
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
    QListWidgetItem *item1 = new QListWidgetItem("Start serwer rtsp i http");
    item1->setFont(itemfont);
    item1->setIcon(QIcon(":/icons/httpstart.png"));
    menuListSerwer->addItem(item1);
    //Add item2
    QListWidgetItem *item2 = new QListWidgetItem("Szukaj kamer");
    item2->setFont(itemfont);
    item2->setIcon(QIcon(":/icons/szukaj.png"));
    menuListSerwer->addItem(item2);

    layoutSerwer->addWidget(menuListSerwer);
    menuListSerwer->setFocus();
    menuListSerwer->setCurrentItem(item1);
    connect(menuListSerwer, &QListWidget::itemClicked, this, &MainWindow::onMenuItemSerwerClicked);
    connect(menuListSerwer, &QListWidget::itemActivated, this, &MainWindow::onMenuItemSerwerClicked);

    QPushButton *closeBtnSerwer = new QPushButton("Ukryj", drawerWidgetSerwer);
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
    connect(toggleActionPodglad, &QAction::triggered, this,[this](){
        ukryjPokazPanelPodglad();
    });
    QVBoxLayout *layoutPodglad = new QVBoxLayout(drawerWidgetPodglad);
    layoutPodglad->setContentsMargins(8,8,8,8);
    layoutPodglad->setSpacing(8);

    QLabel *titlePodglad = new QLabel("<b>LIVE PODGLĄÐ</b>", drawerWidgetPodglad);
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
    btnSerweryLiveStream->setStyleSheet(stylesheetPushButton);
    layoutPodglad->addWidget(btnSerweryLiveStream);
    connect(btnSerweryLiveStream, &QPushButton::clicked, this, [this,font](){
        QDialog *dialog = new QDialog(this);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowTitle("SERWERY LIVE STREAM");
        dialog->resize(900, 500);
        QVBoxLayout *layoutDialog = new QVBoxLayout(dialog);

        QStackedWidget *stackedWidget = new QStackedWidget(dialog);
        QWidget *widget1page = new QWidget();
        QVBoxLayout *layoutPage1 = new QVBoxLayout(widget1page);
        QTableWidget *table = new QTableWidget();
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

        header->setSectionResizeMode(0,QHeaderView::Fixed);
        table->setColumnWidth(0,60);
        header->setSectionResizeMode(1,QHeaderView::Stretch);
        header->setSectionResizeMode(2,QHeaderView::Stretch);
        header->setSectionResizeMode(3,QHeaderView::Fixed);
        table->setColumnWidth(3,100);

        czytajSerweryDat();
        table->setRowCount(ItemModelSerweryDat->rowCount());
        table->setColumnCount(ItemModelSerweryDat->columnCount());

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
                if(col == 3)
                    item->setTextAlignment(Qt::AlignLeft);
                table->setItem(row,col, item);
            }
        }

        QTimer *timerOnline = new QTimer(dialog);
        connect(timerOnline, &QTimer::timeout,dialog,[table](){
            for(int row = 0; row < table->rowCount(); row++){
                qDebug()<< table->item(row, 2)->text();
                QTcpSocket socket;
                socket.connectToHost(table->item(row, 2)->text(), 8554);
                if (socket.waitForConnected(1000))
                {
                    qDebug() << "Serwer działa";
                    socket.disconnectFromHost();
                    table->item(row,3)->setText("🟢 Online");
                }
                else
                {
                    qDebug() << "Serwer nie działa";
                    table->item(row,3)->setText("🔴 Offline");
                }
            }

        });
        timerOnline->start(1000);


        QHBoxLayout *h1layout = new QHBoxLayout();
        QPushButton *btnDodaj      = new QPushButton("➕ Dodaj");
        QPushButton *btnUsun       = new QPushButton("🗑 Usuń");
        QPushButton *btnModyfikuj  = new QPushButton("✏ Modyfikuj");
        QPushButton *btnPolacz     = new QPushButton("Połącz");
        QPushButton *btnZapisz     = new QPushButton("💾 Zapisz");
        QPushButton *btnAnuluj     = new QPushButton("✖ Zamknij");

        btnDodaj->setStyleSheet(stylesheetPushButton);
        btnUsun->setStyleSheet(stylesheetPushButtonRed);
        btnModyfikuj->setStyleSheet(stylesheetPushButton);
        btnPolacz->setStyleSheet(stylesheetPushButton);
        btnZapisz->setStyleSheet(stylesheetPushButton);
        btnAnuluj->setStyleSheet(stylesheetPushButtonRed);
        QList<QPushButton*> buttons =
        {
            btnDodaj,
            btnUsun,
            btnModyfikuj,
            btnPolacz,
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
                widget1page,table,numer2,lineEditNazwa,lineEditAdres](){
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
                      << new QTableWidgetItem("🟢 Online");  //"🔴 Offline"
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
                      << new QTableWidgetItem("🟢 Online");  //"🔴 Offline"
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
                [this,numer2,table,font,lineEditNazwa,lineEditAdres,stackedWidget,widget1page](const QHostInfo &info){
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
                      << new QTableWidgetItem("🟢 Online");  //"🔴 Offline"
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
        connect(btnDodaj, &QPushButton::clicked, dialog,[stackedWidget,widget2page,
                numer2,table,lineEditNazwa, lineEditAdres](){
            stackedWidget->setCurrentWidget(widget2page);
            numer2->setText(QString::number(table->rowCount()+1));
            lineEditNazwa->setText("");
            lineEditNazwa->setFocus();
            lineEditAdres->setText("");
        });
        connect(btnModyfikuj, &QPushButton::clicked,dialog,[table,stackedWidget,
                                    widget2page,numer2,lineEditNazwa,lineEditAdres](){
            int row = table->currentRow();
            if(row == -1){
                QMessageBox::information(nullptr,"INFO","WYBIERZ WIERSZ");
                return;
            }else{
            numer2->setText(table->item(row,0)->text());
            lineEditNazwa->setText(table->item(row,1)->text());
            lineEditAdres->setText(table->item(row,2)->text());
            stackedWidget->setCurrentWidget(widget2page);
            }
        });
        connect(btnUsun, &QPushButton::clicked, dialog,[table,font,timerOnline](){
            timerOnline->stop();
            int row = table->currentRow();
            table->removeRow(row);
            for(int x = 0; x < table->rowCount(); x++){
                table->item(x,0)->setText(QString::number(x+1));
            }
            timerOnline->start();
        });
        connect(btnZapisz, &QPushButton::clicked, dialog,[this,table](){

            if(!ItemModelSerweryDat){
                ItemModelSerweryDat = new QStandardItemModel();
            }else{
                ItemModelSerweryDat->clear();
            }
            ItemModelSerweryDat->setRowCount(table->rowCount());
            ItemModelSerweryDat->setColumnCount(table->columnCount());
            for(int row = 0; row < table->rowCount(); row++){
                for(int col = 0; col < table->columnCount(); col++){
                    QString text = table->item(row, col)->text();
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
        connect(btnPolacz, &QPushButton::clicked, dialog, [this,dialog](){
            createWidgetListaLivekamery();
            dialog->close();
        });
        connect(btnAnuluj,&QPushButton::clicked, dialog,&QDialog::close);

        dialog->show();
    });

    QGroupBox *groupBox = new QGroupBox("widok okna liveStream",drawerWidgetPodglad);
    groupBox->setAlignment(Qt::AlignCenter);
    //groupBox->setFixedHeight(80);
    QHBoxLayout *layoutWidokOkien = new QHBoxLayout(groupBox);
    layoutWidokOkien->setContentsMargins(5,20,5,5);
    layoutWidokOkien->setSpacing(5);
    QPushButton *btn4okna = new QPushButton(groupBox);
    btn4okna->setIcon(createGridIcon(2, 2));   // 4 okna
    btn4okna->setIconSize(QSize(32, 32));
    connect(btn4okna, &QPushButton::clicked,this ,[this](){
        tworzeWidgetNagrania(4);
    });
    QPushButton *btn6okna = new QPushButton(groupBox);
    btn6okna->setIcon(createGridIcon(2, 3));   // 6 okna
    btn6okna->setIconSize(QSize(32, 32));
    connect(btn6okna, &QPushButton::clicked,this ,[this](){
        tworzeWidgetNagrania(6);
    });
    QPushButton *btn9okna = new QPushButton(groupBox);
    btn9okna->setIcon(createGridIcon(3, 3));   // 9 okna
    btn9okna->setIconSize(QSize(32, 32));
    connect(btn9okna, &QPushButton::clicked,this ,[this](){
        tworzeWidgetNagrania(9);
    });
    QPushButton *btn12okna = new QPushButton(groupBox);
    btn12okna->setIcon(createGridIcon(3, 4));   // 12 okna
    btn12okna->setIconSize(QSize(32, 32));
    connect(btn12okna, &QPushButton::clicked,this ,[this](){
        tworzeWidgetNagrania(12);
    });
    QPushButton *btn16okna = new QPushButton(groupBox);
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
    closeBtnPodglad->setStyleSheet(stylesheetPushButton);
    connect(closeBtnPodglad, &QPushButton::clicked, this, &MainWindow::ukryjPokazPanelPodglad);
//    layoutPodglad->addStretch(1);
    layoutPodglad->addWidget(groupBox);
    layoutPodglad->addWidget(closeBtnPodglad);

//PANEL BOCZNY NAGRANIA
    QAction *toggleActionNagrania = toolbar->addAction("☰ NAGRANIA");
    toggleActionNagrania->setCheckable(true);
    QToolButton *toolButtonNagrania = qobject_cast<QToolButton*>(toolbar->widgetForAction(toggleActionNagrania));
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
    connect(toggleActionNagrania, &QAction::triggered, this,[this,toggleActionNagrania](){
        ukryjPokazPanelNagrania();
    });
    QVBoxLayout *layoutNagrania = new QVBoxLayout(drawerWidgetNagrania);
    layoutNagrania->setContentsMargins(8,8,8,8);
    layoutNagrania->setSpacing(8);

    QLabel *titleNagrania = new QLabel("<b>NAGRANIA</b>", drawerWidgetNagrania);
    titleNagrania->setStyleSheet(stylesheetLabelSelectedBlue);
    titleNagrania->setAlignment(Qt::AlignCenter);
    layoutNagrania->addWidget(titleNagrania);

    QPushButton *closeBtnNagrania = new QPushButton("Ukryj", drawerWidgetNagrania);
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
    int end = 300;
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
    int end = 300;
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
    int end = 300;
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
    // while (playerVector.size() > ileKamer) {
    //     if(!playerVector.isEmpty()){
    //         playerVector.last()->stop();
    //         playerVector.last()->deleteLater();
    //     }
    //     playerVector.removeLast();
    // }

    labelVideoVector.clear();
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

    for(int x = 0; x < ileKamer; x++){
        QLabel *labelVideo = new QLabel("KAMERA Nr: "+QString::number(x+1)+"\nBRAK OBRAZU", livePodgladWidget);
        labelVideoVector.append(labelVideo);
        labelVideo->setAlignment(Qt::AlignCenter);
        labelVideo->setStyleSheet("background: black;color: white;font-size:26px");
        labelVideo->setProperty("indexKamery", x); // zapamiętujemy indeks do przywracania pozycji w siatce
        labelVideo->setProperty("rowKamery", row);
        labelVideo->setProperty("colKamery", col);
        grid->addWidget(labelVideo, row, col);
        col++;
        if (col >= cols) {
            col = 0; row++;
        }
        labelVideo->installEventFilter(this);
        labelVideo->setProperty("camContainer", QVariant::fromValue((QWidget*)livePodgladWidget));
        labelVideo->setCursor(Qt::PointingHandCursor);
    }

    rootLayout->addWidget(livePodgladWidget);
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
    czytajSerweryDat();
    qDebug()<<widgetVectr.size() << widgetLayutVector.size();
    menuListPodglad->clear();
    widgetVectr.clear();
    widgetLayutVector.clear();
    itemVector.clear();
    QFont font2;
    font2.setBold(true);
    font2.setPointSize(16);
    int liczba = 0;
    for(int x = 0; x < ItemModelSerweryDat->rowCount(); x++){
        QString nazwa = ItemModelSerweryDat->item(x,1)
        ?ItemModelSerweryDat->item(x,1)->text()
        :QString();
        QString adres = ItemModelSerweryDat->item(x,2)
                            ?ItemModelSerweryDat->item(x,2)->text()
                            :QString();
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
        QLabel *label = new QLabel(nazwa,widget);
        label->setFont(font2);
        label->setStyleSheet("border: none;");
        QLabel *label2 = new QLabel(adres,widget);
        label2->setFont(font2);
        label2->setStyleSheet("border: none;");
        hlayout->addWidget(label);
        hlayout->addStretch();
        hlayout->addWidget(label2);
        widgetLayut->addLayout(hlayout);
        item->setFont(font2);
        item->setSizeHint(widget->sizeHint());

        czytajKameryDat("http://"+adres+":8080/kamery.dat");
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
            label->setStyleSheet("border: none;");
            QPushButton *btnOn = new QPushButton("On",widget);
            btnOn->setFixedWidth(60);
            btnOn->setStyleSheet(stylesheetPushButton);
            connect(btnOn, &QPushButton::clicked, widget,[this,liczba,kameraName,adresKamery,adres](){
                if(labelVideoVector.size() == 0){
                    QMessageBox::information(nullptr,"INFO","WYBIERZ PODZIAŁ SIATKI KAMER");
                    return;
                }
                qDebug() << kameraName << adres;
                qDebug()<<liczba << "rtsp://"+adres+":8554/"+kameraName;
                // if(btnOn->text() == "On"){
                // if(liczba < labelVideoVector.size()){
                //     btnOn->setText("Off");
                //     labelVideoVector[liczba]->setText(adres+"\n"+kameraName);
                //     playerVector.resize(labelVideoVector.size());
                // if (!playerVector[liczba]) {
                //     playerVector[liczba] = new QMediaPlayer(widget);
                //     QAudioOutput *audio  = new QAudioOutput(playerVector[liczba]);
                //     QVideoSink   *sink   = new QVideoSink(playerVector[liczba]);
                //     audio->setVolume(0);
                //     playerVector[liczba]->setAudioOutput(audio);
                //     playerVector[liczba]->setVideoSink(sink);
                //     playerVector[liczba]->setSource("rtsp://"+adres+":8554/"+kameraName);
                //     playerVector[liczba]->play();
                //     connect(sink, &QVideoSink::videoFrameChanged, this,
                //             [=](const QVideoFrame &frame) {
                //                 if (!frame.isValid()) return;
                //                 QVideoFrame copy(frame);
                //                 copy.map(QVideoFrame::ReadOnly);
                //                 labelVideoVector[liczba]->setPixmap(
                //                     QPixmap::fromImage(copy.toImage()).scaled(
                //                         labelVideoVector[liczba]->size(), Qt::IgnoreAspectRatio,
                //                         Qt::SmoothTransformation));
                //                 copy.unmap();
                //             //    lblBrak->hide();
                //             });
                // }
                // }
                // }else if(btnOn->text() == "Off"){
                //     if (playerVector.size() > liczba && playerVector[liczba]) {
                //         playerVector[liczba]->stop();
                //         playerVector[liczba]->deleteLater();
                //         playerVector[liczba] = nullptr;
                //     }
                //     btnOn->setText("On");
                // }
            });
            QPushButton *btnOff = new QPushButton("Off", widget);
            btnOff->setFixedWidth(60);
            btnOff->setStyleSheet(stylesheetPushButton);
            connect(btnOff, &QPushButton::clicked,widget,[liczba](){
                qDebug() << "reconect " << liczba;
            });

            hlayout->addWidget(label);
            hlayout->addStretch(1);
            hlayout->addWidget(btnOn);
            hlayout->addWidget(btnOff);
            widgetLayutVector[x]->addWidget(groupBox);
        //    widgetLayutVector[x]->addLayout(hlayout);
            liczba++;
        }
        menuListPodglad->addItem(item);
        menuListPodglad->setItemWidget(item,widget);

    }
    for (int i = 0; i < widgetVectr.size(); ++i)
    {
        itemVector[i]->setSizeHint(widgetVectr[i]->sizeHint());
    }
}

void MainWindow::czytajKameryDat(QString adres)
{
    if(!ItemModel){
    ItemModel = new QStandardItemModel();
    }else{
        ItemModel->clear();
    }
//    QString adres = "http://localhost:8080/kamery.dat";
    QNetworkAccessManager manager;
    QNetworkRequest request(
    //    (QUrl("http://localhost:8080/kamery.dat"))
        (QUrl(adres))
        );

    QNetworkReply *reply = manager.get(request);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished,
            &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError)
    {
        qDebug() << reply->errorString();
//        QMessageBox::information(this, "INFO",reply->errorString());
//        ileWierszy = 0;
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();

    QDataStream stream(data);
    stream.setVersion(QDataStream::Qt_6_0);

    qint32 n, m;
    stream >> n >> m;

    ItemModel->setRowCount(n);
    ItemModel->setColumnCount(m);

    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < m; ++j)
        {
            QString tekst;
            stream >> tekst;
            QStandardItem *item = new QStandardItem(tekst);
            item->setTextAlignment(Qt::AlignCenter);
            ItemModel->setItem(i, j, item);
        }
    }

//    ileWierszy = ItemModel->rowCount();

    reply->deleteLater();
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
    QString text = item->text();
    if(text == "Start serwer rtsp i http" || text == "Zatrzymaj serwer rtsp i http"){
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
                item->setText("Zatrzymaj serwer rtsp i http");
                qDebug() << "Serwer HTTP wystartował na porcie" << httpSerwer->serverPort();

                mtx->ensureInstalled();

                // czytajKameryDat("http://localhost:8080/kamery.dat");
                // for(int x = 0; x < ItemModel->rowCount(); x++){
                //     qDebug()<<ItemModel->rowCount()<< ItemModel->index(x,1).data().toString()
                //     << ItemModel->index(x,2).data().toString()
                //     << ItemModel->index(x,3).data().toString()
                //     << ItemModel->index(x,4).data().toString()
                //     << ItemModel->index(x,5).data().toString();
                // }

            } else {
                QMessageBox::warning(this, "Błąd serwera HTTP",
                    "Nie udało się uruchomić serwera HTTP (port może być zajęty).");
            }
        } else {
            httpSerwer->stop();
            mtx->stopMtx();
            item->setText("Start serwer rtsp i http");
            qDebug() << "Serwer HTTP zatrzymany";
        }
    }else if(text == "Szukaj kamer"){
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
        int height = centralWidget->height()+toolbar->height();  //+statusBar()->height();
        FindNewCamera *newCamera = new FindNewCamera(this);
        newCamera->resize(width, height);
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
    if (event->type() == QEvent::MouseButtonPress) {
        QLabel *labelVideo = qobject_cast<QLabel*>(obj);
        if (labelVideo && labelVideo->property("camContainer").isValid()) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {

                if (powiekszonyLabel == nullptr) {
                    // --- TRYB POWIĘKSZENIA ---
                    // Liczymy ile wierszy i kolumn ma siatka
                    int maxRow = 0, maxCol = 0;
                    for (int i = 0; i < grid->count(); i++) {
                        int r, c, rs, cs;
                        grid->getItemPosition(i, &r, &c, &rs, &cs);
                        maxRow = qMax(maxRow, r + rs);
                        maxCol = qMax(maxCol, c + cs);
                    }
                    // Ukrywamy wszystkie inne labele
                    for (int i = 0; i < grid->count(); i++) {
                        QWidget *w = grid->itemAt(i)->widget();
                        if (w && w != labelVideo)
                            w->hide();
                    }
                    // Wyjmujemy label z jego komórki i wstawiamy
                    // z powrotem rozciągnięty na całą siatkę (rowSpan x colSpan)
                    grid->removeWidget(labelVideo);
                    grid->addWidget(labelVideo, 0, 0, maxRow, maxCol);
                    powiekszonyLabel = labelVideo;
                    qDebug() << "Powiększono:" << labelVideo->text();

                } else {
                    // --- POWRÓT DO SIATKI ---
                    // Wyjmujemy powiększony label ze spana
                    grid->removeWidget(powiekszonyLabel);
                    // Wstawiamy z powrotem na oryginalne miejsce (1x1)
                    int r = powiekszonyLabel->property("rowKamery").toInt();
                    int c = powiekszonyLabel->property("colKamery").toInt();
                    grid->addWidget(powiekszonyLabel, r, c, 1, 1);
                    // Pokazujemy wszystkie ukryte labele
                    for (int i = 0; i < grid->count(); i++) {
                        QWidget *w = grid->itemAt(i)->widget();
                        if (w) w->show();
                    }
                    powiekszonyLabel = nullptr;
                    qDebug() << "Powrót do siatki";
                }
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

