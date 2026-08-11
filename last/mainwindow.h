#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHBoxLayout>
#include <QLabel>
#include <QAction>
#include <QToolBar>
#include <QListWidget>
#include <QTimer>
#include <QResizeEvent>
#include <QStandardItemModel>
#include <QGroupBox>
#include <QPushButton>
#include <QTableWidget>
#include <QFileSystemWatcher>

class MediaMTXManager;
class HttpSerwer;
class FfmpegPlayer;

class MainWindow : public QMainWindow
{
    Q_OBJECT
protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    bool czytajKameryDat(const QString &adres);
    bool zapiszKameryDat(const QString &adres);
    QStandardItemModel *ItemModel = nullptr;
    QStandardItemModel *ItemModelSerweryDat = nullptr;
    QString appHomePath;

private slots:
    void onMenuItemSerwerClicked(QListWidgetItem *item);
    void onMenuItemPodgladClicked(QListWidgetItem *item);
signals:
    void sygnalResize();

private:
    // Zwraca token "X-Auth-Token" potrzebny do odczytu/zapisu plików .dat
    // na serwerze HTTP wskazanym przez url - dla własnego (lokalnego)
    // serwera token jest znany automatycznie, dla zdalnego trzeba go podać
    // ręcznie (zapamiętywany w QSettings). Patrz httpserwer.h.
    QString resolveAuthTokenForHost(const class QUrl &url);
//    QString resolveAuthTokenForHostPopraw(const class QUrl &url);
    void setupUi();
    void ukryjPokazPanelSerwer();
    void ukryjPokazPanelPodglad();
    void ukryjPokazPanelNagrania();
    void tworzeWidgetNagrania(int ileKamer);
    void zapiszSerweryDat();
    void czytajSerweryDat();
    void createWidgetListaLivekamery();
    void createWidgetUstawienia();
    bool statusUkrytySerwer = true;
    bool statusUkrytyPodglad = true;
    bool statusUkrytyNagrania = true;
    QFileSystemWatcher fileWatcher;
    std::tuple<bool, QString, QString> ffprobeTest(const QString &rtspUrl);
    MediaMTXManager *mtx;
    QIcon createGridIcon(int rows, int cols);
    QVector<QWidget *> widgetVectr;
    QVector<QVBoxLayout *> widgetLayutVector;
    QVector<QListWidgetItem *> itemVector;
    QVector<QLabel *> labelVideoVector;
    QVector<FfmpegPlayer *> playerVector;
    FfmpegPlayer *playerek = nullptr;
    QVector<bool> ignoreAspectRatio; // stan IgnoreAspectRatio per kamera (dla btnResize)
    QVector<QWidget *> kameraWidgetVector;
    QVector<QSlider *> sliderVector;
    QVector<QPushButton *> btnAudioOnVector;
    QVector<bool> audioEnabledVector;
    QString adreshttp;
    QString stylesheetPushButton;
    QString stylesheetPushButtonRed;
    QString stylesheetLabelSelectedBlue;
    QString stylesheetListWidgetBlue;
    QString stylesheetSliderBlue;
    QString stylesheetComboBox;
    QString stylesheetTable;
    QWidget *centralWidget;
    QWidget *drawerWidgetSerwer;
    QWidget *drawerWidgetPodglad;
    QWidget *drawerWidgetNagrania;
    QWidget *livePodgladWidget = nullptr;
    QTableWidget *table;
    int liczba = 0;
    QLabel *powiekszonyLabel = nullptr;   // nullptr = widok siatki, != nullptr = tryb powiększenia
    QWidget *powiekszonyWidget = nullptr; // kameraWidget powiększonego labela (zawiera gbox)
    //    QString appHomePath;
    QListWidget *menuListSerwer;
    QListWidget *menuListPodglad;
    QHBoxLayout *rootLayout;
    QGridLayout *grid;
    QLabel *centralLabel;
    QToolBar *toolbar;
//    QStandardItemModel *ItemModel = nullptr;
    // Timery animacji wysuwania paneli - przechowywane jako pola,
    // żeby móc zatrzymać poprzednią animację przy szybkim przeklikaniu.
    QTimer *animTimerSerwer = nullptr;
    QTimer *animTimerPodglad = nullptr;
    QTimer *animTimerNagrania = nullptr;
    HttpSerwer *httpSerwer = nullptr;
};
#endif // MAINWINDOW_H
