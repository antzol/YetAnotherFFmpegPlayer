#include "audiolevelmeter.h"

#include "concretesamplesextractor.h"
#include "averagelevelcalculator.h"
#include "loudnesscalculator.h"

//---------------------------------------------------------------------------------------
AudioLevelMeter::AudioLevelMeter(QObject *parent)
    : QObject{parent}
{
    setObjectName("AudioLevelMeter");
}

//---------------------------------------------------------------------------------------
AudioLevelMeter::~AudioLevelMeter()
{
}

//---------------------------------------------------------------------------------------
void AudioLevelMeter::setChannelCount(int numberOfChannels)
{
    channelsCount = numberOfChannels;
    if (levelCalculator)
        levelCalculator->setChannelsCount(channelsCount);
}

//---------------------------------------------------------------------------------------
void AudioLevelMeter::setSampleRate(int rate)
{
    if (rate <= 0)
    {
        QString msg = QString("INVALID value of the sampling rate: %1").arg(rate);
        loggable.logMessage(objectName(), QtDebugMsg, msg);
        return;
    }
    sampleRate = rate;
    numberOfSamplesToUpdate = sampleRate / updateRate;
}

//---------------------------------------------------------------------------------------
void AudioLevelMeter::setUpdateRate(int rate)
{
    if (rate <= 0)
    {
        QString msg = QString("INVALID value of the updating rate: %1").arg(rate);
        loggable.logMessage(objectName(), QtDebugMsg, msg);
        return;
    }
    updateRate = rate;
    numberOfSamplesToUpdate = sampleRate / updateRate;
}

//---------------------------------------------------------------------------------------
void AudioLevelMeter::receiveAudioSample(AVFrame *avFrame)
{
    AVSampleFormat avSampleFormat = static_cast<AVSampleFormat>(avFrame->format);
    int numberOfChannels = avFrame->ch_layout.nb_channels;

    if (!samplesExtractor)
    {
        loggable.logMessage(objectName(), QtDebugMsg, "Samples extractor doesn't exist.");
        createSamplesExtractor(avFrame);
    }
    else if (samplesExtractor->getSampleFormat() != avSampleFormat)
    {
        QString msg = QString("Samples extractor exists, but for different formant: %1 -> %2.")
                .arg(mapAvSampleFormatToString(samplesExtractor->getSampleFormat()),
                     mapAvSampleFormatToString(avSampleFormat));
        loggable.logMessage(objectName(), QtDebugMsg, msg);
        samplesExtractor.reset();
        createSamplesExtractor(avFrame);
    }

    /// TODO: add other methods of audio level measurement
    if (!levelCalculator)
        levelCalculator = std::make_unique<LoudnessCalculator>(numberOfChannels);
//        levelCalculator = std::make_unique<AverageLevelCalculator>(numberOfChannels);


    if (samplesExtractor && levelCalculator)
    {
        samplesExtractor->pushAvFrame(avFrame);

        std::vector<int16_t> samples = samplesExtractor->getNextSamplesForAllChannels();
        std::vector<double> outputLevels;
        while (!samples.empty())
        {
            outputLevels = levelCalculator->pushSamples(samples);
            samples = samplesExtractor->getNextSamplesForAllChannels();

            if (++sampleCount == numberOfSamplesToUpdate)
            {
                sampleCount = 0;
                emit audioLevelsCalculated(outputLevels);
            }
        }
    }
}

//---------------------------------------------------------------------------------------
void AudioLevelMeter::createSamplesExtractor(AVFrame *avFrame)
{
    loggable.logMessage(objectName(), QtDebugMsg, "Create samples extractor...");
    QAudioFormat::SampleFormat format = mapSampleFormat(static_cast<AVSampleFormat>(avFrame->format));

    switch (format)
    {
    case QAudioFormat::UInt8:
        loggable.logMessage(objectName(), QtDebugMsg, "Create samples extractor for uint8...");
        samplesExtractor = std::make_unique<ConcreteSamplesExtractor<uint8_t>>();
        break;
    case QAudioFormat::Int16:
        loggable.logMessage(objectName(), QtDebugMsg, "Create samples extractor for int16...");
        samplesExtractor = std::make_unique<ConcreteSamplesExtractor<int16_t>>();
        break;
    case QAudioFormat::Int32:
        loggable.logMessage(objectName(), QtDebugMsg, "Create samples extractor for int32...");
        samplesExtractor = std::make_unique<ConcreteSamplesExtractor<int32_t>>();
        break;
    case QAudioFormat::Float:
        loggable.logMessage(objectName(), QtDebugMsg, "Create samples extractor for float...");
        samplesExtractor = std::make_unique<ConcreteSamplesExtractor<float>>();
        break;
    default:
        loggable.logMessage(objectName(), QtWarningMsg, "Create samples extractor FAILED...");
        break;
    }
}

//---------------------------------------------------------------------------------------
