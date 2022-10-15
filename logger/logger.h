#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>

#include <deque>
#include <mutex>
#include <condition_variable>

#include <QFile>
#include <QHash>
#include <QMutex>
#include <QQueue>
#include <QTextStream>

class Logger : public QObject
{
    Q_OBJECT
public:
    static Logger *getInstance();
    virtual ~Logger();

public slots:
    void writeMessage(const QString &src, QtMsgType type, const QString &msg);
    void writeAvError(const QString &src, QtMsgType type, const QString &msg, int avError);

private:
    explicit Logger(QObject *parent = nullptr);
    void writeToFile();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static void initInstance();
    static Logger *instance;
    static std::once_flag initInstanceFlag;

    std::mutex mutex;
    std::condition_variable dequeSizeChanged;
    std::atomic_bool needQuit{false};

    QQueue<QString> messages;

    bool ready{false};

    QFile file;
    QTextStream fileStream;
};

#endif // LOGGER_H
