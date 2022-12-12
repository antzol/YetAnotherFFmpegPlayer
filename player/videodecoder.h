#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include "decoder.h"
#include "ffmpegfilter.h"
#include "videoframe.h"

#include <QVideoFrame>

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/frame.h>
}

class VideoDecoder : public Decoder
{
    Q_OBJECT
public:
    explicit VideoDecoder(const QString& name, QObject *parent = nullptr);
    virtual ~VideoDecoder();

    int outputFrame(AVFrame *avFrame) override;

    QSize getPictureSize() const;

signals:
    void videoFrameReady(const std::shared_ptr<VideoFrame> videoFrame);

private:
    int convertFrame(AVFrame *avFrame, FFmpegFilter *filter);
    int outputVideoFrame(AVFrame *avFrame);

    void createDeinterlacingFiltersQueue();
    void initDeinterlacer(AVFrame *frame);
    void initCropper(AVFrame *frame);

    QVideoFrame m_videoFrame;
    QVideoFrameFormat::PixelFormat m_pixelFormat;

    FFmpegFilter *deinterlacingQueueHead{nullptr};
    FFmpegFilter *deinterlacer{nullptr};
    FFmpegFilter *cropper{nullptr};

};

#endif // VIDEODECODER_H
