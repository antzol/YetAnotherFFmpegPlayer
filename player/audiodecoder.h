#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include "decoder.h"

#include "audioframe.h"
#include "audiolevelmeter.h"
#include "utils.h"

#include <QAudioFormat>

class AudioDecoder : public Decoder
{
    Q_OBJECT
public:
    explicit AudioDecoder(const QString& name, QObject *parent = nullptr);

    void setAudioLevelMeter(const std::shared_ptr<AudioLevelMeter> &meter);

    bool open(AVStream *stream) override;

    QAudioFormat audioFormat();
    int inputChannelCount() const;

    int outputFrame(AVFrame *avFrame) override;

signals:
     void audioSampleReady(const std::shared_ptr<AudioFrame> audioFrame);

private:
     int inChannelCount{0};

     int outChannelCount{0};
     int outSampleRate{0};
     AVSampleFormat outSampleFormat;
     AVChannelLayout outChannelLayout;

     std::shared_ptr<AudioLevelMeter> levelMeter;
};

#endif // AUDIODECODER_H
