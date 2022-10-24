#ifndef AUDIOLEVELWIDGET_H
#define AUDIOLEVELWIDGET_H

#include <QWidget>

#include <QPainter>

class AudioLevelWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AudioLevelWidget(Qt::Orientation orientationFlag = Qt::Vertical, QWidget *parent = nullptr);

    void setOrientation(Qt::Orientation flag);
    Qt::Orientation getOrientation();

public slots:
    void setLevel(double value);

protected:
    void paintEvent(QPaintEvent *);

private:
    void drawVerticalIndicator();

    Qt::Orientation orientation{Qt::Vertical};

    double alignmentLevel    {-23};
    double overloadAlarmLevel{-18};
    double lowerBound        {-90};
    double upperBound        {0};

    double range;
    double emptyRange;
    double zeroLine;
    double overloadLine;

    double level;

    QColor emptyColor         {QColorConstants::Svg::black};
    QColor normalSignalColor  {QColorConstants::Svg::limegreen};
    QColor alarmSignalColor   {QColorConstants::Svg::yellow};
    QColor overloadSignalColor{QColorConstants::Svg::red};

    QPainter painter;
};

#endif // AUDIOLEVELWIDGET_H
