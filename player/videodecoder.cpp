#include "videodecoder.h"

#include "logger.h"
#include "utils.h"

#include <QDebug>

#define FFMPEG_ALIGNMENT (32)

//---------------------------------------------------------------------------------------
VideoDecoder::VideoDecoder(const QString &name, QObject *parent)
    : Decoder{name, parent}
{

}

//---------------------------------------------------------------------------------------
VideoDecoder::~VideoDecoder()
{
    if (deinterlacer)
        delete deinterlacer;

    if (cropper)
        delete cropper;
}

//---------------------------------------------------------------------------------------
int VideoDecoder::outputFrame(AVFrame *avFrame)
{
    int size = 0;
    if (!avFrame->interlaced_frame)
    {   // not interlaced
        size = outputVideoFrame(avFrame);
    }
    else
    {
        if (!deinterlacingQueueHead)
            createDeinterlacingFiltersQueue();
        size = convertFrame(avFrame, deinterlacingQueueHead);
    }
    return size;
}

//---------------------------------------------------------------------------------------
QSize VideoDecoder::getPictureSize() const
{
    return QSize(codecContext->width, codecContext->height);
}

//---------------------------------------------------------------------------------------
int VideoDecoder::convertFrame(AVFrame *avFrame, FFmpegFilter *filter)
{
    int size = 0;
    int result;

    if (!filter->isReady())
    {
        if (filter == cropper)
            initCropper(avFrame);

        if (filter == deinterlacer)
            initDeinterlacer(avFrame);
    }

    result = filter->feedGraph(avFrame);
    if (result < 0)
    {
        loggable.logAvError(objectName(), QtWarningMsg,
                        QString("Error while feeding the filter graph for %1.").arg(filter->objectName()),
                        result);
        return 0;
    }

    while (true)
    {
        AVFrame *totalFrame{nullptr};

        result = filter->getOutputFrame(&totalFrame);

        if (result == AVERROR(EAGAIN) || result == AVERROR(AVERROR_EOF))
            break;
        if (result < 0)
            return 0;

        FFmpegFilter *next = filter->getNextFilter();
        if (next)
            size = convertFrame(totalFrame, next);
        else
            size = outputVideoFrame(totalFrame);

        av_frame_unref(totalFrame);
        av_frame_unref(avFrame);
    }
    return size;
}

//---------------------------------------------------------------------------------------
int VideoDecoder::outputVideoFrame(AVFrame *avFrame)
{
    std::shared_ptr<VideoFrame> videoFrame(new VideoFrame(avFrame->pts));

    int size = videoFrame->fromAvFrame(avFrame);

    if (size)
        emit videoFrameReady(videoFrame);

    return size;
}

//---------------------------------------------------------------------------------------
void VideoDecoder::createDeinterlacingFiltersQueue()
{
    loggable.logMessage(objectName(), QtDebugMsg, "Create deinterlacing filters queue...");
//    deinterlacer = new FFmpegFilter("Deinterlacer");
    cropper = new FFmpegFilter("Predeinterlace Cropper");
//    cropper->setNextFilter(deinterlacer);
    deinterlacingQueueHead = cropper;
//    deinterlacingQueueHead = deinterlacer;
}

//---------------------------------------------------------------------------------------
void VideoDecoder::initDeinterlacer(AVFrame *frame)
{
    if (!deinterlacer)
        deinterlacer = new FFmpegFilter("Deinterlacer");

    if (deinterlacer->isReady())
        return;

    QString filter{"yadif"};
    QString filterParams{"0:-1:1"};

    char args[512];
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             frame->width,
             frame->height,
             frame->format,
             codecContext->time_base.num,
             codecContext->time_base.den,
             codecContext->sample_aspect_ratio.num,
             codecContext->sample_aspect_ratio.den);

    QString inputParams{args};
    deinterlacer->init(filter, filterParams, inputParams);
}

//---------------------------------------------------------------------------------------
void VideoDecoder::initCropper(AVFrame *frame)
{
    if (!cropper)
        cropper = new FFmpegFilter("Cropper");

    if (cropper->isReady())
        return;

    int frameFix = 0; // fix bad width on some streams
    if (frame->height == 576)
    {
        if (frame->width < 704)
            frameFix = 2;
        else if (frame->width > 704)
            frameFix = -16;
    }

    QString filter{"crop"};
//    QString filterParams = QString("%1:%2:0:0:0:1").arg(frame->width + frameFix).arg(frame->height);
    QString filterParams = QString("w=%1:h=%2:x=0:y=0").arg(frame->width + frameFix).arg(frame->height);

    char args[512];
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             frame->width,
             frame->height,
             frame->format,
             codecContext->time_base.num,
             codecContext->time_base.den,
             codecContext->sample_aspect_ratio.num,
             codecContext->sample_aspect_ratio.den);

    QString inputParams{args};
    cropper->init(filter, filterParams, inputParams);
}

//---------------------------------------------------------------------------------------


