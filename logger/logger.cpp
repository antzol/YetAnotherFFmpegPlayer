#include "logger.h"

#include <iostream>

#include <QDateTime>
#include <QDir>

#include <QDebug>

extern "C" {
#include "libavutil/error.h"
#include "libavutil/log.h"
}

Logger *Logger::instance = nullptr;
std::once_flag Logger::initInstanceFlag;

//---------------------------------------------------------------------------------------
constexpr const char* getMessageTypeString(QtMsgType type)
{
    switch (type)
    {
    case QtDebugMsg:    return "[Debug]    ";
    case QtInfoMsg:     return "[Info]     ";
    case QtWarningMsg:  return "[Warning]  ";
    case QtCriticalMsg: return "[Critical] ";
    case QtFatalMsg:    return "[Fatal]    ";
    }
    return "[Unknown]  ";
}

//---------------------------------------------------------------------------------------
Logger* Logger::getInstance()
{
    std::call_once(initInstanceFlag, &Logger::initInstance);

    return instance;
}

//---------------------------------------------------------------------------------------
Logger::Logger(QObject *parent)
    : QObject{parent}
{
    qRegisterMetaType <QtMsgType> ("QtMsgType");

    QString dirName = "logs";
    QString path = QString("%1/%2.log")
            .arg(dirName, QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss"));

    file.setFileName(path);

    if(!file.isOpen())
    {
        QDir appDir = QDir::current();
        if(!appDir.exists(dirName))
            appDir.mkdir(dirName);
        file.open(QIODevice::WriteOnly | QIODevice::Append);
    }

    fileStream.setDevice(&file);

    std::thread writerThread = std::thread(&Logger::writeToFile, this);
    writerThread.detach();
}

//---------------------------------------------------------------------------------------
Logger::~Logger()
{
    needQuit.store(true);
    std::unique_lock<std::mutex> locker(mutex);
    while (!messages.empty())
    {
        dequeSizeChanged.wait(locker, [this](){return messages.empty();});
    }
    file.close();
}

//---------------------------------------------------------------------------------------
void Logger::writeToFile()
{
    while (!needQuit)
    {
        {
            std::unique_lock<std::mutex> locker(mutex);
            while (messages.empty())
            {
                dequeSizeChanged.wait(locker, [this](){return needQuit.load() || !messages.empty();});
            }
            if (needQuit.load() && messages.empty())
                break;
        }

        decltype(messages) copiedMessages;
        {
            std::lock_guard<std::mutex> guard(mutex);
            std::swap(copiedMessages, messages);
        }

        while (!copiedMessages.empty())
        {
            QString txt = copiedMessages.dequeue();

            std::cout << txt.toStdString();
            fileStream << txt;

            if (txt.back() != '\n')
            {
                std::cout << std::endl;
                fileStream << Qt::endl;
            }
        }
    }
    std::cout << "Log finished." << std::endl;
    fileStream << "Log finished." << Qt::endl;
    dequeSizeChanged.notify_all();
}

//---------------------------------------------------------------------------------------
void Logger::initInstance()
{
    instance = new Logger();


}

//---------------------------------------------------------------------------------------
void Logger::writeMessage(const QString &src, QtMsgType type, const QString &msg)
{
    QString dt = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss.zzz");
    QString txt = QString("[%1]%2[%3] %4").arg(dt, getMessageTypeString(type), src, msg);

    std::lock_guard<std::mutex> guard(mutex);
    messages.enqueue(txt);
    dequeSizeChanged.notify_all();
}

//---------------------------------------------------------------------------------------
void Logger::writeAvError(const QString &src, QtMsgType type, const QString &msg, int avError)
{
    char buf[AV_ERROR_MAX_STRING_SIZE];
    int res = av_strerror(avError, buf, AV_ERROR_MAX_STRING_SIZE);
    QString message = QString("%1 %2").arg(msg, res ? buf : "Unknown error code.");
    writeMessage(src, type, message);
}

//---------------------------------------------------------------------------------------
