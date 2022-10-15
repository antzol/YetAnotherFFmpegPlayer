#ifndef DETAILSDOCKWIDGET_H
#define DETAILSDOCKWIDGET_H

#include <QDockWidget>
#include <QTextEdit>

class DetailsDockWidget : public QDockWidget
{
    Q_OBJECT
public:
    explicit DetailsDockWidget(QWidget *parent = nullptr);

private:
    QTextEdit *textEdit{nullptr};
};

#endif // DETAILSDOCKWIDGET_H
