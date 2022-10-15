#include "openstreamdialog.h"
#include "ui_openstreamdialog.h"

static const int UdpBufferPageSize = 4096;

//---------------------------------------------------------------------------------------
OpenStreamDialog::OpenStreamDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OpenStreamDialog)
{
    ui->setupUi(this);

    /// TODO: use recently used settings
    // debug
    ui->udpAddressField->setText("235.0.1.10");
    ui->udpPortField->setValue(2022);
    ui->udpLocalInterfaceAddressField->setText("192.168.1.232");
    ui->udpBufferSizeField->setValue(70);
}

//---------------------------------------------------------------------------------------
OpenStreamDialog::~OpenStreamDialog()
{
    delete ui;
}

//---------------------------------------------------------------------------------------
QString OpenStreamDialog::getStreamFullUri()
{
    return uri;
}

//---------------------------------------------------------------------------------------
int OpenStreamDialog::getRwTimeout()
{
    return ui->udpTimeoutField->value();
}

//---------------------------------------------------------------------------------------
void OpenStreamDialog::prepareOutputAndAccept()
{
    ///TODO: validate input

    uri = QString("udp://%1:%2?fifo_size=%3")
            .arg(ui->udpAddressField->text().trimmed())
            .arg(ui->udpPortField->value())
            .arg(ui->udpBufferSizeField->value() * UdpBufferPageSize);

    QString localAddress = ui->udpLocalInterfaceAddressField->text().trimmed();

    if (!localAddress.isEmpty())
    {
        uri += QString("&localaddr=%1").arg(localAddress);
    }
    else
    {
        uri.insert(uri.indexOf("//")+2, '@');
    }

    accept();
}

//---------------------------------------------------------------------------------------
