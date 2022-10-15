#ifndef VIDEOFRAME_H
#define VIDEOFRAME_H

#include "frame.h"

#include <QVideoFrame>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

class VideoFrame : public Frame
{
public:
    explicit VideoFrame(int64_t pts);
    virtual ~VideoFrame();

    int fromAvFrame(const AVFrame *avFrame) override;
    const QVideoFrame* getVideoFrame() const;

private:
    QVideoFrame *videoFrame{nullptr};
    QVideoFrameFormat::PixelFormat pixelFormat;
};

#endif // VIDEOFRAME_H
