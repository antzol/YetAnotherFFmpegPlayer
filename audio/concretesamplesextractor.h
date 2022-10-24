#ifndef CONCRETESAMPLESEXTRACTOR_H
#define CONCRETESAMPLESEXTRACTOR_H

#include "samplesextractor.h"

#include <optional>

//---------------------------------------------------------------------------------------
template<typename T>
class ConcreteSamplesExtractor : public SamplesExtractor
{
public:
    std::vector<int16_t> getNextSamplesForAllChannels() override;

private:
    std::optional<T> getSample(int channelIndex, int sampleIndex);

};

//---------------------------------------------------------------------------------------
template<typename T>
std::vector<int16_t> ConcreteSamplesExtractor<T>::getNextSamplesForAllChannels()
{
    if (currentSampleIndex >= samplesCount)
    {
        samples.clear();
    }
    else
    {
        for (int i = 0; i < channelsCount && i < samples.size(); ++i)
        {
            std::optional<T> res = getSample(i, currentSampleIndex);
            T sampleValue = res ? *res : 0;
            int16_t convertedValue;
            if (!std::is_floating_point<T>::value)
            {   // for int16_t and uint_8_t
                convertedValue = sampleValue;
            }
            else
            {   // for float
                if (sampleValue > 1.0)
                    sampleValue = 1.0;
                if (sampleValue < -1.0)
                    sampleValue = -1.0;
                convertedValue = sampleValue * 0x7fff;
            }

            samples[i] = convertedValue;
        }
        currentSampleIndex++;
    }

    return samples;
}

//---------------------------------------------------------------------------------------
template<typename T>
std::optional<T> ConcreteSamplesExtractor<T>::getSample(int channelIndex, int sampleIndex)
{
    T sample;

    if (sampleFormatIsPlanar)
    {
        if ((sampleIndex + 1) * bytesPerSample >= frame->linesize[0])
            return {};
        T *channelSamples = reinterpret_cast<T*>(frame->data[channelIndex]);
        sample = channelSamples[sampleIndex];
    }
    else
    {
        int samplePos = sampleIndex * channelsCount + channelsCount;
        int lineIndex = 0;
        int lineSizeInSamples = frame->linesize[lineIndex] / bytesPerSample;
        while (samplePos >= lineSizeInSamples)
        {
            samplePos -= lineSizeInSamples;
            lineIndex++;
            if (lineIndex >= AV_NUM_DATA_POINTERS)
                return {};
            lineSizeInSamples = frame->linesize[lineIndex] / bytesPerSample;
        }
        uint8_t *ptr = frame->data[lineIndex] + samplePos * bytesPerSample;
        sample = *(reinterpret_cast<T*>(ptr));
    }

    return sample;
}

#endif // CONCRETESAMPLESEXTRACTOR_H
