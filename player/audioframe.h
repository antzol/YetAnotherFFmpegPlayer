#ifndef AUDIOFRAME_H
#define AUDIOFRAME_H

#include "frame.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libavdevice/avdevice.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

class AudioFrame : public Frame
{
public:
    explicit AudioFrame(int64_t pts);
    explicit AudioFrame(int64_t pts, AVSampleFormat outFormat);
    virtual ~AudioFrame();

    int fromAvFrame(const AVFrame *avFrame) override;
    int fromAvFrame(const AVFrame *avFrame, SwrContext *swrContext);

    int getSize() const;
    const char* getData() const;

private:
    void fillProperities(const AVFrame *avFrame);

    uint8_t *data{nullptr};
    int size{0};

    int channelCount;
    AVChannelOrder channelOrder;
    int samplesPerChannelCount;
    AVSampleFormat sampleFormat;

    AVRational timeBase;
    int64_t duration;

    AVSampleFormat outputFormat;
};

#endif // AUDIOFRAME_H
