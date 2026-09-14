#pragma once

#include <QImage>
#include <QRect>
#include <QVector>
#include <opencv2/video/background_segm.hpp>

/**
 * Prosty detektor ruchu oparty o odejmowanie tła (MOG2), uwzględniający:
 *  - zmiany światła: model tła MOG2 sam się adaptuje do stopniowych zmian
 *    (zachmurzenie, zmierzch); cienie są wykrywane osobno przez MOG2 i
 *    odrzucane progowaniem, a nagłe globalne zmiany oświetlenia (np.
 *    włączenie światła) są wykrywane jako "ruch" obejmujący nienaturalnie
 *    dużą część kadru naraz i ignorowane,
 *  - wielkość przedmiotu: filtrowanie konturów po polu powierzchni jako
 *    PROCENCIE CAŁEGO KADRU (nie bezwzględnej liczbie pikseli) - dzięki
 *    temu ten sam ustawiony poziom czułości oznacza to samo niezależnie
 *    od rozdzielczości konkretnej kamery (obiekt zajmujący np. 1% kadru
 *    wyzwoli detekcję zarówno przy 640x360, jak i 1920x1080 - przy progu
 *    w bezwzględnych pikselach kamera o wyższej rozdzielczości byłaby
 *    wielokrotnie czulsza dla tego samego fizycznego obiektu),
 *  - strefy wolne od detekcji: maskowanie fragmentów obrazu przed
 *    szukaniem konturów, na podstawie prostokątów w oryginalnych
 *    współrzędnych obrazu kamery (takich samych jak w strefa.dat).
 *
 * Klasa NIE jest wątkowo-bezpieczna - jedna instancja powinna być używana
 * zawsze z tego samego wątku (ma własny, narastający w czasie stan modelu
 * tła, więc nie nadaje się też do współdzielenia między kamerami).
 */
class MotionDetector
{
public:
    // minObjectAreaPercent: minimalne pole obiektu jako PROCENT powierzchni
    // całego kadru (0-100), powyżej którego uznajemy to za ruch. Domyślnie
    // 1.0 (1% kadru) - odpowiednik wcześniejszego progu 10000px przy
    // rozdzielczości 640x360 (10000 / (640*360) ≈ 4.3% - patrz uwaga w
    // mainwindow.cpp przy przeliczaniu z pola CZUŁOŚĆ DETEKCJI).
    explicit MotionDetector(double minObjectAreaPercent = 1.0);

    // Zwraca true, jeśli w klatce wykryto ruch obiektu o polu
    // >= minObjectAreaPercent% powierzchni kadru, POZA strefami
    // wymienionymi w strefyWolne (współrzędne x,y,szerokość,wysokość w
    // pikselach oryginalnego obrazu kamery - te same co w strefa.dat).
    bool processFrame(const QImage &frame, const QVector<QRect> &strefyWolne);

    void setMinObjectAreaPercent(double percent) { minObjectAreaPercent = percent; }
    double getMinObjectAreaPercent() const { return minObjectAreaPercent; }

private:
    cv::Ptr<cv::BackgroundSubtractor> pMOG2;
    double minObjectAreaPercent; // % powierzchni kadru (0-100)

    // Analiza w pomniejszonej rozdzielczości - znacząco szybsze, a do
    // wykrycia "czy w ogóle coś się rusza" pełna rozdzielczość nie jest
    // potrzebna.
    static constexpr double SKALA_ANALIZY = 0.5;
    // Jeśli "ruch" nagle obejmuje więcej niż tyle % kadru naraz, uznajemy
    // to za globalną zmianę oświetlenia (np. włączenie światła), a nie
    // realny obiekt - ignorujemy taką klatkę.
    static constexpr double PROG_GLOBALNEJ_ZMIANY = 0.6;
};
