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
    void czytajKameryDat(QString adres);
    QStandardItemModel *ItemModel = nullptr;
    QStandardItemModel *ItemModelSerweryDat = nullptr;
    QString appHomePath;

private slots:
    void onMenuItemSerwerClicked(QListWidgetItem *item);
    void onMenuItemPodgladClicked(QListWidgetItem *item);
signals:
    void sygnalResize();

private:
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
    MediaMTXManager *mtx;
    QIcon createGridIcon(int rows, int cols);
    QVector<QWidget *> widgetVectr;
    QVector<QVBoxLayout *> widgetLayutVector;
    QVector<QListWidgetItem *> itemVector;
    QVector<QLabel *> labelVideoVector;
    QVector<FfmpegPlayer *> playerVector;
    QVector<bool> ignoreAspectRatio; // stan IgnoreAspectRatio per kamera (dla btnResize)
    QVector<QWidget *> kameraWidgetVector;
    QVector<QSlider *> sliderVector;
    QVector<QPushButton *> btnAudioOnVector;
    QVector<bool> audioEnabledVector;
    QString stylesheetPushButton;
    QString stylesheetPushButtonRed;
    QString stylesheetLabelSelectedBlue;
    QString stylesheetListWidgetBlue;
    QString stylesheetSliderBlue;
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
