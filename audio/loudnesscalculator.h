#ifndef LOUDNESSCALCULATOR_H
#define LOUDNESSCALCULATOR_H

#include "audiolevelcalculator.h"

class LoudnessCalculator : public AudioLevelCalculator
{
public:
    explicit LoudnessCalculator(int channelsCount);

    void setChannelsCount(int cnt) override;
    std::vector<double> pushSamples(const std::vector<int16_t> &samples) override;

private:
    std::vector<double> preFilter1Z0;
    std::vector<double> preFilter1Z1;
    std::vector<double> preFilter1Z2;

    std::vector<double> preFilter2Z0;
    std::vector<double> preFilter2Z1;
    std::vector<double> preFilter2Z2;
};

#endif // LOUDNESSCALCULATOR_H
