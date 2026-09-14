#include "clickableslabel.h"
#include <QPainter>
#include <QPen>

ClickableLabel::ClickableLabel(QWidget *parent) : QLabel(parent) {
    // Opcjonalnie: ustaw kursor krzyżyka, który ułatwia precyzyjne zaznaczanie
    setCursor(Qt::CrossCursor);
}

void ClickableLabel::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        startPoint = event->position().toPoint(); // Zapamiętaj punkt startu
        selectionRect = QRect(); // Zresetuj poprzedni prostokąt
        isSelecting = true;
    }
    QLabel::mousePressEvent(event); // Przepuść zdarzenie dalej, jeśli potrzeba
}

void ClickableLabel::mouseMoveEvent(QMouseEvent *event) {
    if (isSelecting) {
        // Dynamicznie twórz prostokąt między punktem startu a obecną pozycją myszy
        // QRect::normalized() dba o to, by szerokość i wysokość były dodatnie, 
        // nawet jeśli przeciągasz mysz w lewo lub w górę
        selectionRect = QRect(startPoint, event->position().toPoint()).normalized();
        update(); // Wymuś przerysowanie widgetu (wywoła paintEvent)
    }
    QLabel::mouseMoveEvent(event);
}

void ClickableLabel::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && isSelecting) {
        isSelecting = false;
        
        // Wyślij sygnał z gotowymi współrzędnymi prostokąta
        emit selectionFinished(selectionRect);
    }
    QLabel::mouseReleaseEvent(event);
}

void ClickableLabel::paintEvent(QPaintEvent *event) {
    QLabel::paintEvent(event); // Najpierw rysujemy wideo pod spodem

    QPainter painter(this);

    // 1. OBLIČZENIA SKALOWANIA (odwrócony wzór)
    double wideoW = videoSize.width();//strefaLiniaVector[0].videoSize.width();
    double wideoH = videoSize.height();//strefaLiniaVector[0].videoSize.height();
    double labelW = this->width();
    double labelH = this->height();

    // POPRAWKA (dzielenie przez zero / NaN -> UB): dopóki nie przyszła
    // jeszcze pierwsza klatka wideo (albo strumień nigdy się nie
    // połączył), videoSize jest domyślnie (0,0). "labelW / 0.0" dla typu
    // double daje +inf (nie crash), ale zaraz potem "0 * inf" przy
    // liczeniu offsetX/offsetY daje NaN, a rzutowanie NaN na int
    // (static_cast<int>) jest zachowaniem niezdefiniowanym w C++ (UB).
    // Podobnie gdy label jeszcze nie ma realnego rozmiaru (labelW/labelH
    // == 0, np. tuż po utworzeniu widgetu, przed layoutem). W obu
    // przypadkach nie da się poprawnie przeskalować ZAPISANYCH stref
    // (są w pikselach obrazu kamery, wymagają znajomości jej
    // rozdzielczości) - pomijamy TYLKO tę sekcję. Aktualnie rysowany
    // prostokąt zaznaczenia (sekcja 3 poniżej) jest już we współrzędnych
    // widgetu i nie wymaga skalowania, więc rysuje się niezależnie.
    bool mozemySkalowac = (wideoW > 0.0 && wideoH > 0.0 && labelW > 0.0 && labelH > 0.0);

    if (mozemySkalowac) {
        double skala = std::min(labelW / wideoW, labelH / wideoH);
        int offsetX = static_cast<int>((labelW - (wideoW * skala)) / 2.0);
        int offsetY = static_cast<int>((labelH - (wideoH * skala)) / 2.0);

    // 2. RYSOWANIE STREF ZAPISANYCH W TABELI
    // Ustawiamy styl dla zapisanych stref (np. zielona linia, żółty półprzezroczysty środek)
    QPen penZapisany(Qt::green, 2, Qt::SolidLine);
    painter.setPen(penZapisany);
    painter.setBrush(QColor(255, 255, 0, 40)); // Żółte wypełnienie, alfa 40

    for (const auto &strefa : std::as_const(strefaLiniaVector)) {

        // Przeliczenie pikseli kamery na piksele okna QLabel
        int labelX = static_cast<int>(strefa.rect.x() * skala) + offsetX;
        int labelY = static_cast<int>(strefa.rect.y() * skala) + offsetY;
        int labelWiersz = static_cast<int>(strefa.rect.width() * skala);
        int labelWysokosc = static_cast<int>(strefa.rect.height() * skala);

        QRect prostokatNaEkranie(labelX, labelY, labelWiersz, labelWysokosc);
        painter.drawRect(prostokatNaEkranie);

        // Opcjonalnie: Rysowanie numeru ID strefy w lewym górnym rogu prostokąta
        QString id = QString::number(strefa.id);
        QFont font = painter.font();
        font.setPixelSize(24);
        font.setBold(true);
        painter.setFont(font);
        painter.drawText(labelX + 1, labelY + 25, id);
    }
    } // if (mozemySkalowac)

    // 3. RYSOWANIE AKTUALNIE RYSOWANEGO PROSTOKĄTA (MYSZKĄ)
    if (isSelecting && !selectionRect.isNull()) {
        QPen penRysowanie(Qt::red, 2, Qt::DashLine);
        painter.setPen(penRysowanie);
        painter.setBrush(QColor(0, 120, 255, 50));
        painter.drawRect(selectionRect);
    }

    // 4. NAPIS "REC" GDY WYKRYTO RUCH (np. z MotionDetector) - służy do
    // wizualnego potwierdzenia, że strefy wolne od detekcji z strefa.dat
    // faktycznie działają (ruch w strefie wolnej NIE powinien pokazać REC).
    if (motionActive) {
        QFont fontRec = painter.font();
        fontRec.setPixelSize(28);
        fontRec.setBold(true);
        painter.setFont(fontRec);
        painter.setPen(Qt::red);
        painter.drawText(rect().adjusted(0, 10, -15, 0),
                         Qt::AlignTop | Qt::AlignRight, "● REC");
    }
}
