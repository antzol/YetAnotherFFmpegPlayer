#include "loudnesscalculator.h"

namespace FilterCoefficients
{
static const double preFilter1A1 = -1.69065929318241;
static const double preFilter1A2 =  0.73248077421585;
static const double preFilter1B0 =  1.53512485958697;
static const double preFilter1B1 = -2.69169618940638;
static const double preFilter1B2 =  1.19839281085285;

static const double preFilter2A1 = -1.99004745483398;
static const double preFilter2A2 =  0.99007225036621;
static const double preFilter2B0 =  1.0;
static const double preFilter2B1 = -2.0;
static const double preFilter2B2 =  1.0;
}

//---------------------------------------------------------------------------------------
LoudnessCalculator::LoudnessCalculator(int channelsCount)
    : AudioLevelCalculator(channelsCount)
    , preFilter1Z0(channelsCount)
    , preFilter1Z1(channelsCount)
    , preFilter1Z2(channelsCount)
    , preFilter2Z0(channelsCount)
    , preFilter2Z1(channelsCount)
    , preFilter2Z2(channelsCount)
{

}

//---------------------------------------------------------------------------------------
void LoudnessCalculator::setChannelsCount(int cnt)
{
    AudioLevelCalculator::setChannelsCount(cnt);

    preFilter1Z0.resize(channelsCount);
    preFilter1Z1.resize(channelsCount);
    preFilter1Z2.resize(channelsCount);
    preFilter2Z0.resize(channelsCount);
    preFilter2Z1.resize(channelsCount);
    preFilter2Z2.resize(channelsCount);
}

//---------------------------------------------------------------------------------------
std::vector<double> LoudnessCalculator::pushSamples(const std::vector<int16_t> &samples)
{
    for (int channelIndex = 0; channelIndex < channelsCount; ++channelIndex)
    {
        int16_t sample = abs(samples[channelIndex]);

        preFilter1Z0[channelIndex] = sample
                - FilterCoefficients::preFilter1A1 * preFilter1Z1[channelIndex]
                - FilterCoefficients::preFilter1A2 * preFilter1Z2[channelIndex];

        double preFilter2Input = FilterCoefficients::preFilter1B0 * preFilter1Z0[channelIndex]
                + FilterCoefficients::preFilter1B1 * preFilter1Z1[channelIndex]
                + FilterCoefficients::preFilter1B2 * preFilter1Z2[channelIndex];

        preFilter1Z2[channelIndex] = preFilter1Z1[channelIndex];
        preFilter1Z1[channelIndex] = preFilter1Z0[channelIndex];

        preFilter2Z0[channelIndex] = preFilter2Input
                - FilterCoefficients::preFilter2A1 * preFilter2Z1[channelIndex]
                - FilterCoefficients::preFilter2A2 * preFilter2Z2[channelIndex];

        double preFilter2Output = FilterCoefficients::preFilter2B0 * preFilter2Z0[channelIndex]
                + FilterCoefficients::preFilter2B1 * preFilter2Z1[channelIndex]
                + FilterCoefficients::preFilter2B2 * preFilter2Z2[channelIndex];

        preFilter2Z2[channelIndex] = preFilter2Z1[channelIndex];
        preFilter2Z1[channelIndex] = preFilter2Z0[channelIndex];

        double output = abs(preFilter2Output);

        if (output < 1)
            output = 1;

        calculatedLevels[channelIndex] = -0.691 + 10 * log(output/32767.0);
    }
    return calculatedLevels;
}

//---------------------------------------------------------------------------------------
