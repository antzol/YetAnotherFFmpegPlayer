#ifndef LOGGABLE_H
#define LOGGABLE_H

#include <QObject>

#include "logger.h"

class Loggable : public QObject
{
    Q_OBJECT
public:
    explicit Loggable(QObject *parent = nullptr);

    void logMessage(const QString &src, QtMsgType type, const QString &msg);
    void logAvError(const QString &src, QtMsgType type, const QString &msg, int avError);

signals:
    void sendMessage(const QString &src, QtMsgType type, const QString &msg);
    void sendAvError(const QString &src, QtMsgType type, const QString &msg, int avError);

private:
    enum {
        MaxAvErrorRepeat = 10
    };

    int lastError{0};
    int lastErrorCount{0};
    int maxAvErrorRepeat{MaxAvErrorRepeat};
};

#endif // LOGGABLE_H
