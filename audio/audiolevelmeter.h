#ifndef AUDIOLEVELMETER_H
#define AUDIOLEVELMETER_H

#include <QObject>

#include "audioframe.h"
#include "audiolevelcalculator.h"
#include "averagelevelcalculator.h"
#include "samplesextractor.h"
#include "loggable.h"
#include "utils.h"

class AudioLevelMeter : public QObject
{
    Q_OBJECT
public:
    explicit AudioLevelMeter(QObject *parent = nullptr);
    virtual ~AudioLevelMeter();

    void setChannelCount(int numberOfChannels);
    void setSampleRate(int rate);
    /// TODO: set update rate from the configuration
    void setUpdateRate(int rate);

public slots:
    void receiveAudioSample(AVFrame *avFrame);

signals:
    void audioLevelsCalculated(const std::vector<double> &levels);

private:
    void createSamplesExtractor(AVFrame *avFrame);

    int channelsCount{0};
    int sampleRate{48000};
    int updateRate{20};
    int numberOfSamplesToUpdate{sampleRate / updateRate};
    int sampleCount{0};

    std::unique_ptr<SamplesExtractor> samplesExtractor;
    std::unique_ptr<AudioLevelCalculator> levelCalculator;

    Loggable loggable;
};

#endif // AUDIOLEVELMETER_H
