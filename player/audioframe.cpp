#include "audioframe.h"

//---------------------------------------------------------------------------------------
AudioFrame::AudioFrame(int64_t pts) : Frame(pts)
{

}

//---------------------------------------------------------------------------------------
AudioFrame::~AudioFrame()
{
    delete[] data;
}

//---------------------------------------------------------------------------------------
int AudioFrame::fromAvFrame(const AVFrame *avFrame)
{
    SwrContext *swrCtx = nullptr;

    if (!avFrame )
        return size;

    int64_t channelLayout = avFrame->channel_layout != 0
            ? avFrame->channel_layout
            : av_get_default_channel_layout(avFrame->channels);
    AVSampleFormat outSampleFormat = av_get_packed_sample_fmt(static_cast<AVSampleFormat>(avFrame->format));

    swrCtx = swr_alloc_set_opts(nullptr,
                                channelLayout,
                                outSampleFormat,
                                avFrame->sample_rate,
                                channelLayout,
                                static_cast<AVSampleFormat>(avFrame->format),
                                avFrame->sample_rate,
                                0, nullptr);

    if (swr_init(swrCtx) == 0)
    {
        size = av_samples_get_buffer_size(nullptr, avFrame->channels, avFrame->nb_samples, outSampleFormat, 0);
        data = new uint8_t[size];

        swr_convert(swrCtx,
                    &data,
                    avFrame->nb_samples,
                    const_cast<const uint8_t**>(avFrame->data),
                    avFrame->nb_samples);
    }

    if (swrCtx)
        swr_free(&swrCtx);

    return size;
}

//---------------------------------------------------------------------------------------
int AudioFrame::getSize() const
{
    return size;
}

//---------------------------------------------------------------------------------------
const char *AudioFrame::getData() const
{
    return reinterpret_cast<char*>(data);
}

//---------------------------------------------------------------------------------------
