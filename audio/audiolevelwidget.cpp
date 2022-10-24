#include "audiolevelwidget.h"

//---------------------------------------------------------------------------------------
AudioLevelWidget::AudioLevelWidget(Qt::Orientation orientationFlag, QWidget *parent)
    : QWidget{parent}
    , orientation(orientationFlag)
    , range(upperBound - lowerBound)
    , emptyRange(range)
    , zeroLine((upperBound - alignmentLevel) / range)
    , overloadLine((upperBound - overloadAlarmLevel) / range)
    , level(lowerBound)
{
    /// TODO: load parameters of the audio level widget from configuration (ini file)
    setMinimumSize(10, 10);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

//---------------------------------------------------------------------------------------
void AudioLevelWidget::setOrientation(Qt::Orientation flag)
{
    orientation = flag;
}

//---------------------------------------------------------------------------------------
Qt::Orientation AudioLevelWidget::getOrientation()
{
    return orientation;
}

//---------------------------------------------------------------------------------------
void AudioLevelWidget::setLevel(double value)
{
    if (value > upperBound)
        level = upperBound;
    else if (value < lowerBound)
        level = lowerBound;
    else
        level = value;

    update();
}

//---------------------------------------------------------------------------------------
void AudioLevelWidget::paintEvent(QPaintEvent *)
{
    emptyRange = fabs(upperBound - level) / range;

    painter.begin(this);
    painter.setWindow(rect());
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    /// TODO: draw audio level widget with horizontal orientation.
    drawVerticalIndicator();

    painter.end();
}

//---------------------------------------------------------------------------------------
void AudioLevelWidget::drawVerticalIndicator()
{
    if(level > overloadAlarmLevel)
    {
        painter.setBrush(overloadSignalColor);
        QRect overloadBar(0, emptyRange * height(),
                          width(), abs(emptyRange - overloadLine) * height());
        painter.drawRect(overloadBar);

        painter.setBrush(alarmSignalColor);
        QRect alarmBar(0, overloadLine * height(),
                       width(), abs(overloadLine - zeroLine) * height());
        painter.drawRect(alarmBar);

        painter.setBrush(normalSignalColor);
        QRect normalBar(0, zeroLine * height(), width(), height());
        painter.drawRect(normalBar);
    }
    else if(level > alignmentLevel)
    {
        painter.setBrush(alarmSignalColor);
        QRect alarmBar(0, emptyRange * height(),
                       width(), abs(emptyRange - zeroLine) * height());
        painter.drawRect(alarmBar);

        painter.setBrush(normalSignalColor);
        QRect normalBar(0, zeroLine * height(), width(), height());
        painter.drawRect(normalBar);
    }
    else
    {
        painter.setBrush(normalSignalColor);
        QRect normalBar(0, emptyRange * height(), width(), height());
        painter.drawRect(normalBar);
    }

    painter.setBrush(emptyColor);
    QRectF leftEmptyBar(0, 0, width(), emptyRange * height());
    painter.drawRect(leftEmptyBar);

    // alignment line
    painter.setPen(QPen(Qt::white, 1));
    painter.drawLine(QLineF(0, zeroLine * height(), width(), zeroLine * height()));

    // overload line
    painter.setPen(QPen(Qt::yellow, 1));
    painter.drawLine(QLineF(0, overloadLine * height(), width(), overloadLine * height()));
}

//---------------------------------------------------------------------------------------

