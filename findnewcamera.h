#ifndef FINDNEWCAMERA_H
#define FINDNEWCAMERA_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QTimer>
//#include <opencv4/opencv2/opencv.hpp>
#include <opencv2/opencv.hpp>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QProgressDialog>
#include <QStandardItemModel>


class MainWindow; // forward declaration - pełna definicja niepotrzebna w nagłówku

/**
 * Lekki helper do odczytu listy kamer z pliku kamery.dat.
 * Nie tworzy żadnego UI – bezpieczny do użycia w wątkach roboczych.
 */
class CameraDataReader
{
public:
    CameraDataReader();
    ~CameraDataReader();

    void czytajListaKamer();

    QStandardItemModel *ItemModel;

    CameraDataReader(const CameraDataReader &) = delete;
    CameraDataReader &operator=(const CameraDataReader &) = delete;
};

/**
 * Widżet do wyszukiwania i konfiguracji nowych kamer IP.
 * Zawiera pola do wprowadzania adresu IP, loginu, hasła,
 * przycisk do skanowania oraz listę znalezionych strumieni.
 */
class FindNewCamera : public QWidget
{
    Q_OBJECT

protected:

public:
    explicit FindNewCamera(QWidget *parent = nullptr);
    ~FindNewCamera() override;

    void zapiszListaKamer();
    void czytajListaKamer();
    QStandardItemModel *ItemModel = nullptr;

private slots:
    void buttonZamknij_cliced();
    void buttonSkanuj_clicked();
    void buttonPlay_clicked();
    void buttonZapisz_cliced();
    void updateFrame();

signals:
    void requestCloseFrame();

private:
    MainWindow *mainwindow;
    // --- Etykiety ---
    QLabel *labelAdresIpKamery;
    QLabel *labelPortyRTSP;
    QLabel *labelPortyHTTP;
    QLabel *labelCameraLogin;
    QLabel *labelCameraPassword;
    QLabel *labelPlay;

    // --- Pola edycyjne ---
    QLineEdit *LineEditAdresIpKamery;
    QLineEdit *LineEditPortyRTSP;
    QLineEdit *LineEditPortyHTTP;
    QLineEdit *LineEditCameraLogin;
    QLineEdit *LineEditCameraPassword;

    // --- Przyciski ---
    QPushButton *buttonSkanuj;
    QPushButton *buttonPlay;
    QPushButton *buttonZapisz;
    QPushButton *buttonZamknij;

    // --- Lista strumieni ---
    QListWidget *widgetListListaStrumieni;

    cv::VideoCapture cap;
    //std::unique_ptr<cv::VideoCapture> cap;
    QTimer timer;
    QFutureWatcher<QString> watcher;
    QProgressDialog *progressDialog = nullptr;
    // Licznik kolejnych żądań otwarcia strumienia (Play). Pozwala odróżnić
    // "aktualne" zakończenie otwierania w tle od przestarzałego, gdy
    // użytkownik w międzyczasie wybrał inny strumień albo kliknął Play
    // ponownie zanim poprzednie otwarcie się zakończyło.
    quint64 playRequestId = 0;

    // --- Button zapisz ---
    //void zapiszListaKamer();
    //void czytajListaKamer();
    int ileWierszy = 0;
    //QStandardItemModel *ItemModel;
    QStringList kamera;
    //QList<QStringList> listaKamer;
};

#endif // FINDNEWCAMERA_H