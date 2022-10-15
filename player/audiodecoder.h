#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include "decoder.h"
#include "audioframe.h"

#include <QAudioFormat>

class AudioDecoder : public Decoder
{
    Q_OBJECT
public:
    explicit AudioDecoder(const QString& name, QObject *parent = nullptr);

    QAudioFormat audioFormat();

    int outputFrame(AVFrame *avFrame) override;

signals:
     void audioSampleReady(const std::shared_ptr<AudioFrame> audioFrame);

};

#endif // AUDIODECODER_H
