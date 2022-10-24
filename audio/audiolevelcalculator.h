#ifndef AUDIOLEVELCALCULATOR_H
#define AUDIOLEVELCALCULATOR_H

#include <vector>

extern "C" {
#include <libavutil/frame.h>
}

class AudioLevelCalculator
{
public:
    explicit AudioLevelCalculator(int numberOfChannels);
    virtual ~AudioLevelCalculator() {}

    virtual std::vector<double> pushSamples(const std::vector<int16_t> &samples) = 0;
    virtual void setChannelsCount(int cnt);

    std::vector<double> getLastCalculatedLevels() const;

    size_t getChannelsCount() const;

protected:
    int channelsCount{0};
    std::vector<double> calculatedLevels;
};

#endif // AUDIOLEVELCALCULATOR_H
