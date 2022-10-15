#include "detailsdockwidget.h"



//---------------------------------------------------------------------------------------
DetailsDockWidget::DetailsDockWidget(QWidget *parent) : QDockWidget(parent)
{
    setWindowTitle(tr("Details"));

    textEdit = new QTextEdit();


    setWidget(textEdit);


    setMinimumWidth(300);
    setMinimumHeight(500);


}

//---------------------------------------------------------------------------------------
