#include "averagelevelcalculator.h"

//---------------------------------------------------------------------------------------
AverageLevelCalculator::AverageLevelCalculator(int channelsCount)
    : AudioLevelCalculator(channelsCount)
    , sums(channelsCount)
    , samplesDeque(channelsCount, std::deque<int16_t>(measurementWindowSize))
{

}

//---------------------------------------------------------------------------------------
void AverageLevelCalculator::setChannelsCount(int cnt)
{
    AudioLevelCalculator::setChannelsCount(cnt);

    sums.resize(channelsCount);
    samplesDeque.resize(channelsCount, std::deque<int16_t>(measurementWindowSize));
}

//---------------------------------------------------------------------------------------
std::vector<double> AverageLevelCalculator::pushSamples(const std::vector<int16_t> &samples)
{
    /// TODO: optimize the algorithm
    for (int channelIndex = 0; channelIndex < channelsCount; ++channelIndex)
    {
        int16_t sample = abs(samples[channelIndex]);

        sums[channelIndex] = sums[channelIndex] - samplesDeque[channelIndex].front() + sample;
        samplesDeque[channelIndex].pop_front();
        samplesDeque[channelIndex].push_back(sample);

        double avg = sums[channelIndex] / measurementWindowSize;
        if (avg < 1)
            avg = 1;
        calculatedLevels[channelIndex] = 20 * log(avg / 32767.0);
    }
    return calculatedLevels;
}

//---------------------------------------------------------------------------------------
