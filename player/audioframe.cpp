#include "audioframe.h"

//---------------------------------------------------------------------------------------
AudioFrame::AudioFrame(int64_t pts) : Frame(pts), outputFormat(AV_SAMPLE_FMT_NONE)
{

}

//---------------------------------------------------------------------------------------
AudioFrame::AudioFrame(int64_t pts, AVSampleFormat outFormat) : Frame(pts), outputFormat(outFormat)
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
//    AVSampleFormat outSampleFormat = av_get_packed_sample_fmt(static_cast<AVSampleFormat>(avFrame->format));
//    AVSampleFormat inSampleFormat = static_cast<AVSampleFormat>(avFrame->format);
    AVSampleFormat outSampleFormat = av_get_packed_sample_fmt(static_cast<AVSampleFormat>(avFrame->format));

    int res = swr_alloc_set_opts2(&swrCtx,
                                  &channelLayout,
                                  outSampleFormat,
                                  avFrame->sample_rate,
                                  &channelLayout,
                                  static_cast<AVSampleFormat>(avFrame->format),
                                  avFrame->sample_rate,
                                  0, nullptr);
    if (res < 0)
        return 0;

    if (swr_init(swrCtx) == 0)
    {
        size = av_samples_get_buffer_size(nullptr,
                                          avFrame->ch_layout.nb_channels,
                                          avFrame->nb_samples,

                                          outSampleFormat, 0);
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
int AudioFrame::fromAvFrame(const AVFrame *avFrame, SwrContext *swrContext)
{
    if (!avFrame || !swr_is_initialized(swrContext))
        return 0;


    size = av_samples_get_buffer_size(nullptr,
                                      avFrame->ch_layout.nb_channels,
                                      avFrame->nb_samples,
                                      outputFormat, 0);
    data = new uint8_t[size];

    swr_convert(swrContext,
                &data,
                avFrame->nb_samples,
                const_cast<const uint8_t**>(avFrame->data),
                avFrame->nb_samples);

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
void AudioFrame::fillProperities(const AVFrame *avFrame)
{
    channelCount = avFrame->ch_layout.nb_channels;
    channelOrder = avFrame->ch_layout.order;
    samplesPerChannelCount = avFrame->nb_samples;
    sampleFormat = static_cast<AVSampleFormat>(avFrame->format);
}

//---------------------------------------------------------------------------------------
