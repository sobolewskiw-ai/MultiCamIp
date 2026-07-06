#include "mainwindow.h"
#include "findnewcamera.h"
#include "httpserwer.h"
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    centralWidget(nullptr),
    drawerWidgetSerwer(nullptr),
    drawerWidgetPodglad(nullptr),
    drawerWidgetNagrania(nullptr)
    //toolbar(nullptr)
{
    appHomePath = QDir::homePath()+"/AppMultiCam";
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
    //Add item1Podglad
    QListWidgetItem *item1Podglad = new QListWidgetItem("Live stream");
    item1Podglad->setFont(itemfont);
    item1Podglad->setIcon(QIcon(":/icons/start.png"));
    menuListPodglad->addItem(item1Podglad);
    layoutPodglad->addWidget(menuListPodglad);
    menuListPodglad->setFocus();
    menuListPodglad->setCurrentItem(item1Podglad);
    connect(menuListPodglad, &QListWidget::itemClicked,this, &MainWindow::onMenuItemPodgladClicked);

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
    connect(toggleActionNagrania, &QAction::triggered, this,[this](){
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
        QLabel *labelVideo = new QLabel("KAMERA Nr: "+QString::number(x+1), livePodgladWidget);
        labelVideo->setAlignment(Qt::AlignCenter);
        labelVideo->setStyleSheet("background: black;color: white");
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
            } else {
                QMessageBox::warning(this, "Błąd serwera HTTP",
                    "Nie udało się uruchomić serwera HTTP (port może być zajęty).");
            }
        } else {
            httpSerwer->stop();
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

