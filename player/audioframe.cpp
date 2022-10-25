#include "audioframe.h"

//---------------------------------------------------------------------------------------
AudioFrame::AudioFrame(int64_t pts) : Frame(pts)//, outputFormat(AV_SAMPLE_FMT_NONE)
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

    AVChannelLayout channelLayout = avFrame->ch_layout;
    AVSampleFormat inSampleFormat = static_cast<AVSampleFormat>(avFrame->format);
    AVSampleFormat outSampleFormat = av_get_packed_sample_fmt(static_cast<AVSampleFormat>(avFrame->format));

    int res = swr_alloc_set_opts2(&swrCtx,
                                  &channelLayout, outSampleFormat, avFrame->sample_rate,
                                  &channelLayout, inSampleFormat, avFrame->sample_rate,
                                  0, nullptr);
    if (res < 0)
        return 0;

    if (swr_init(swrCtx) == 0)
    {
        size = av_samples_get_buffer_size(nullptr, avFrame->ch_layout.nb_channels,
                                          avFrame->nb_samples, outSampleFormat, 0);
        data = new uint8_t[size];

        swr_convert(swrCtx, &data, avFrame->nb_samples,
                    const_cast<const uint8_t**>(avFrame->data), avFrame->nb_samples);
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
