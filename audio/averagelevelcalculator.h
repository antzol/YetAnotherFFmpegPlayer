#ifndef AVERAGELEVELCALCULATOR_H
#define AVERAGELEVELCALCULATOR_H

#include "audiolevelcalculator.h"

#include <deque>
#include <numeric>

class AverageLevelCalculator : public AudioLevelCalculator
{
public:
    explicit AverageLevelCalculator(int channelsCount);

    void setChannelsCount(int cnt) override;
    std::vector<double> pushSamples(const std::vector<int16_t> &samples) override;

private:
    // 48 kHz
    // 5 ms => 240 samples
    // 300 ms => 14 400 samples
    // 400 ms => 19 200 samples
    const int measurementWindowSize{14400};
    std::vector<double> sums;
    std::vector<std::deque<int16_t>> samplesDeque;

};

#endif // AVERAGELEVELCALCULATOR_H
