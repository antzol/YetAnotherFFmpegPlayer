#ifndef SAMPLESEXTRACTOR_H
#define SAMPLESEXTRACTOR_H

#include <vector>

extern "C" {
#include <libavutil/frame.h>
}

class SamplesExtractor
{
public:
    SamplesExtractor() {}
    virtual ~SamplesExtractor() {}

    void pushAvFrame(AVFrame *avFrame)
    {
        frame = avFrame;
        sampleFormat = static_cast<AVSampleFormat>(frame->format);
        channelsCount = frame->ch_layout.nb_channels;
        samplesCount = frame->nb_samples;

        sampleFormatIsPlanar = av_sample_fmt_is_planar(sampleFormat);
        bytesPerSample = av_get_bytes_per_sample(sampleFormat);

        samples.resize(channelsCount);
        currentSampleIndex = 0;
    }

    AVSampleFormat getSampleFormat() const
    {
        return sampleFormat;
    }

    virtual std::vector<int16_t> getNextSamplesForAllChannels() = 0;

protected:
    AVFrame *frame{nullptr};
    AVSampleFormat sampleFormat{AV_SAMPLE_FMT_NONE};
    int channelsCount{0};
    int samplesCount{0};

    bool sampleFormatIsPlanar{false};
    int bytesPerSample{1};

    int currentSampleIndex{0};
    std::vector<int16_t> samples;
};

#endif // SAMPLESEXTRACTOR_H
