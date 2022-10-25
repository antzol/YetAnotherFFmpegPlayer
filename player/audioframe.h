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
    virtual ~AudioFrame();

    int fromAvFrame(const AVFrame *avFrame) override;

    int getSize() const;
    const char* getData() const;

private:
    uint8_t *data{nullptr};
    int size{0};
};

#endif // AUDIOFRAME_H
