#include "loggable.h"

//---------------------------------------------------------------------------------------
Loggable::Loggable(QObject *parent)
    : QObject{parent}
{
    Logger *logger = Logger::getInstance();

    connect(this, &Loggable::sendMessage, logger, &Logger::writeMessage, Qt::DirectConnection);
    connect(this, &Loggable::sendAvError, logger, &Logger::writeAvError, Qt::DirectConnection);
}

//---------------------------------------------------------------------------------------
void Loggable::logMessage(const QString &src, QtMsgType type, const QString &msg)
{
    emit sendMessage(src, type, msg);
}

//---------------------------------------------------------------------------------------
void Loggable::logAvError(const QString &src, QtMsgType type, const QString &msg, int avError)
{
    if (avError != lastError)
    {
        emit sendAvError(src, type, msg, avError);
        lastError = avError;
        lastErrorCount = 1;
    }
    else
    {
        if (lastErrorCount < maxAvErrorRepeat)
        {
            emit sendAvError(src, type, msg, avError);
            lastErrorCount++;
        }
        if (lastErrorCount == maxAvErrorRepeat)
        {
            lastErrorCount++;
            emit sendMessage(src, QtWarningMsg, "The maximum number of repeated errors for logging has been reached.");
        }
    }
}

//---------------------------------------------------------------------------------------
