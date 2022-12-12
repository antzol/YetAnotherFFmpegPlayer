#include "videoframe.h"
#include "utils.h"

#include <QDebug>

#define FFMPEG_ALIGNMENT (32)

//---------------------------------------------------------------------------------------
VideoFrame::VideoFrame(int64_t pts) : Frame(pts)
{

}

//---------------------------------------------------------------------------------------
VideoFrame::~VideoFrame()
{
    if (videoFrame)
        delete videoFrame;
}

//---------------------------------------------------------------------------------------
int VideoFrame::fromAvFrame(const AVFrame *avFrame)
{
    SwsContext *swsCtx = nullptr;
    int size = 0;
    uint8_t *dstData[AV_NUM_DATA_POINTERS] = {};
    int dstLinesize[AV_NUM_DATA_POINTERS] = {};

    if (!avFrame)
        return 0;

    AVPixelFormat srcAVFormat = static_cast<AVPixelFormat>(avFrame->format);
    AVPixelFormat dstAVFormat = srcAVFormat;

    pixelFormat = mapPixelFormat(srcAVFormat);

    if (pixelFormat == QVideoFrameFormat::Format_Invalid)
    {
        pixelFormat = QVideoFrameFormat::Format_YUV420P;
        dstAVFormat = mapPixelFormat(pixelFormat);
    }
    int dstHeight = avFrame->height;
    int dstWidth = avFrame->width;

    if (avFrame->sample_aspect_ratio.num != avFrame->sample_aspect_ratio.den)
        dstWidth = (avFrame->width * avFrame->sample_aspect_ratio.num) / avFrame->sample_aspect_ratio.den;

    swsCtx = sws_getContext(avFrame->width, avFrame->height, srcAVFormat,
                            dstWidth, dstHeight, dstAVFormat,
                            SWS_BICUBIC, nullptr, nullptr, nullptr);

    size = av_image_fill_arrays(dstData, dstLinesize, nullptr, dstAVFormat,
                                dstWidth, dstHeight, FFMPEG_ALIGNMENT);

    QVideoFrameFormat frameFormat(QSize(dstWidth, dstHeight), pixelFormat);
    videoFrame = new QVideoFrame(frameFormat);
    if (videoFrame->map(QVideoFrame::WriteOnly))
    {
        if (swsCtx)
        {
            int i = 0;
            while (videoFrame->bits(i))
            {
                dstData[i] = videoFrame->bits(i);
                ++i;
            }

            sws_scale(swsCtx, avFrame->data, avFrame->linesize, 0, avFrame->height, dstData, dstLinesize);
            sws_freeContext(swsCtx);
        }
        videoFrame->unmap();
    }
    return size;
}

//---------------------------------------------------------------------------------------
const QVideoFrame *VideoFrame::getVideoFrame() const
{
    return videoFrame;
}

//---------------------------------------------------------------------------------------
