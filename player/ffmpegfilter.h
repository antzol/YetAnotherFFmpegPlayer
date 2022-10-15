#ifndef FFMPEGFILTER_H
#define FFMPEGFILTER_H

#include <QObject>

#include "loggable.h"

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}

class FFmpegFilter : public QObject
{
    Q_OBJECT
public:
    explicit FFmpegFilter(const QString &name = "", QObject *parent = nullptr);
    virtual ~FFmpegFilter();

    bool init(const QString &filter, const QString& params, const QString &format);
    bool init();

    bool isReady() const;

    int feedGraph(AVFrame *frame);
    int getOutputFrame(AVFrame **frame);

    void setNextFilter(FFmpegFilter *filter);
    FFmpegFilter* getNextFilter() const;

private:
    QString filterName;
    QString filterParams;
    QString inputBufferParams;

    bool ready{false};

    AVFilterGraph *graph{nullptr};

    AVFilterContext *bufferSourceContext{nullptr};
    AVFilterContext *bufferSinkContext{nullptr};

    AVFrame *outFrame{nullptr};

    FFmpegFilter *nextFilter{nullptr};

    Loggable loggable;
};

#endif // FFMPEGFILTER_H
