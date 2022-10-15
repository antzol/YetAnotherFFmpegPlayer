#ifndef OPENSTREAMDIALOG_H
#define OPENSTREAMDIALOG_H

#include <QDialog>

namespace Ui {
class OpenStreamDialog;
}

class OpenStreamDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OpenStreamDialog(QWidget *parent = nullptr);
    ~OpenStreamDialog();

    QString getStreamFullUri();
    int getRwTimeout();

public slots:
    void prepareOutputAndAccept();

private:
    Ui::OpenStreamDialog *ui;

    QString uri;
};

#endif // OPENSTREAMDIALOG_H
