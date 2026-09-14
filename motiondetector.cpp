#include "motiondetector.h"

#include <opencv2/imgproc.hpp>

MotionDetector::MotionDetector(double minObjectAreaPercent)
    : minObjectAreaPercent(minObjectAreaPercent)
{
    // history=500 klatek pamięci tła, varThreshold=16 czułość na piksel,
    // detectShadows=true - żeby móc je potem odfiltrować przez threshold
    // (MOG2 oznacza wykryte cienie wartością ok. 127, pewny ruch to 255).
    pMOG2 = cv::createBackgroundSubtractorMOG2(500, 16, true);
}

bool MotionDetector::processFrame(const QImage &frame, const QVector<QRect> &strefyWolne)
{
    if (frame.isNull())
        return false;

    // QImage (RGB888, ciągły bufor z FfmpegPlayer::frameAvailable) -> cv::Mat
    // bez kopiowania danych pikseli (tylko nagłówek Mat wskazujący na
    // istniejący bufor QImage).
    QImage rgb = (frame.format() == QImage::Format_RGB888)
                     ? frame
                     : frame.convertToFormat(QImage::Format_RGB888);
    if (rgb.width() <= 0 || rgb.height() <= 0)
        return false;

    cv::Mat matRgb(rgb.height(), rgb.width(), CV_8UC3,
                    const_cast<uchar*>(rgb.constBits()),
                    static_cast<size_t>(rgb.bytesPerLine()));

    // Pomniejszenie - analiza nie musi być w pełnej rozdzielczości.
    cv::Mat small;
    cv::resize(matRgb, small, cv::Size(), SKALA_ANALIZY, SKALA_ANALIZY, cv::INTER_LINEAR);
    if (small.empty())
        return false;

    cv::Mat fgMask;
    pMOG2->apply(small, fgMask); // ujemny (domyślny) learningRate = automatyczny

    // Odrzucamy cienie (wartość ok. 127) - zostaje tylko pewny ruch (255).
    cv::threshold(fgMask, fgMask, 200, 255, cv::THRESH_BINARY);

    // Maska wykluczająca strefy wolne od detekcji - współrzędne stref są w
    // pikselach oryginalnego obrazu, trzeba je przeskalować tak samo jak
    // sam obraz analizy.
    if (!strefyWolne.isEmpty()) {
        cv::Mat exclMask(small.size(), CV_8UC1, cv::Scalar(255));
        const cv::Rect granice(0, 0, small.cols, small.rows);
        for (const QRect &r : strefyWolne) {
            cv::Rect cr(static_cast<int>(r.x() * SKALA_ANALIZY),
                        static_cast<int>(r.y() * SKALA_ANALIZY),
                        static_cast<int>(r.width() * SKALA_ANALIZY),
                        static_cast<int>(r.height() * SKALA_ANALIZY));
            cr &= granice; // przytnij do granic obrazu (chroni przed UB przy rect poza obrazem)
            if (cr.width > 0 && cr.height > 0)
                cv::rectangle(exclMask, cr, cv::Scalar(0), cv::FILLED);
        }
        cv::bitwise_and(fgMask, exclMask, fgMask);
    }

    // Filtr globalnej zmiany światła: jeśli "ruch" to nagle większość
    // kadru, to nie jest realny obiekt, tylko zmiana oświetlenia sceny -
    // ignorujemy całą klatkę.
    const double procentRuchu = cv::countNonZero(fgMask) / double(fgMask.total());
    if (procentRuchu > PROG_GLOBALNEJ_ZMIANY)
        return false;

    // Czyszczenie drobnego szumu (pojedyncze piksele/kropki) morfologicznie.
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(fgMask, fgMask, cv::MORPH_OPEN, kernel);

    // Szukamy konturów i filtrujemy po realnej wielkości jako PROCENCIE
    // CAŁEGO KADRU - w pikselach ORYGINALNEGO obrazu, nie pomniejszonego,
    // stąd dzielenie przez SKALA_ANALIZY^2 (pole skaluje się z kwadratem
    // skali liniowej). Liczymy próg w pikselach DYNAMICZNIE z rzeczywistej
    // rozdzielczości bieżącej klatki (rgb.width()*rgb.height()) - dzięki
    // temu ten sam ustawiony procent oznacza porównywalną detekcję
    // niezależnie od rozdzielczości konkretnej kamery.
    std::vector<std::vector<cv::Point>> kontury;
    cv::findContours(fgMask, kontury, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    const double skalaPola = SKALA_ANALIZY * SKALA_ANALIZY;
    const double progPikseli = (minObjectAreaPercent / 100.0)
                                * (static_cast<double>(rgb.width()) * rgb.height());
    for (const auto &k : kontury) {
        const double pole = cv::contourArea(k) / skalaPola;
        if (pole >= progPikseli)
            return true; // wystarczy jeden wystarczająco duży obiekt poza strefami wolnymi
    }
    return false;
}
