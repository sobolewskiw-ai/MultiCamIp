#pragma once

#include <QLabel>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QRect>

class ClickableLabel : public QLabel {
    Q_OBJECT

public:
    explicit ClickableLabel(QWidget *parent = nullptr);
    struct strefaLinia
    {
        QRect rect;
        int id;
    //    QSize videoSize;
    };
    QSize videoSize;
    // Metoda pozwalająca pobrać zaznaczony prostokąt z poziomu innych klas
    QRect getSelectionRect() const { return selectionRect; }
    void ustawStrefy(const QVector<strefaLinia> &noweSrefyVector){
        strefaLiniaVector = noweSrefyVector;
        update();
    };
    // Sterowanie widocznością napisu "REC" (np. z MotionDetector) - do
    // wizualnego potwierdzenia, że wykrywanie ruchu z uwzględnieniem stref
    // wolnych faktycznie działa.
    void setMotionActive(bool active){
        if (motionActive != active) { motionActive = active; update(); }
    };
    bool isMotionActive() const { return motionActive; }

signals:
    // Sygnał wysyłany, gdy użytkownik skończy zaznaczać obszar
    void selectionFinished(const QRect &rect);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QPoint startPoint;   // Punkt początkowy (kliknięcie)
    QRect selectionRect; // Aktualnie zaznaczony prostokąt
    bool isSelecting = false; // Czy myszka jest wciśnięta i przeciągana
    QVector<strefaLinia> strefaLiniaVector;
    bool motionActive = false; // Czy aktualnie wykryto ruch (rysuje "REC")
};
