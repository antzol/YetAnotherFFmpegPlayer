#include "audiolevelcalculator.h"

//---------------------------------------------------------------------------------------
AudioLevelCalculator::AudioLevelCalculator(int numberOfChannels)
    : channelsCount(numberOfChannels)
    , calculatedLevels(numberOfChannels)
{

}

//---------------------------------------------------------------------------------------
void AudioLevelCalculator::setChannelsCount(int cnt)
{
    channelsCount = cnt;
    calculatedLevels.resize(channelsCount);
}

//---------------------------------------------------------------------------------------
std::vector<double> AudioLevelCalculator::getLastCalculatedLevels() const
{
    return calculatedLevels;
}

//---------------------------------------------------------------------------------------
size_t AudioLevelCalculator::getChannelsCount() const
{
    return calculatedLevels.size();
}

//---------------------------------------------------------------------------------------
