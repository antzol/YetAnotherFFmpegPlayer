#include "audiodecoder.h"
#include "utils.h"

#include <QDebug>

//---------------------------------------------------------------------------------------
AudioDecoder::AudioDecoder(const QString &name, QObject *parent)
    : Decoder{name, parent}
{

}

//---------------------------------------------------------------------------------------
QAudioFormat AudioDecoder::audioFormat()
{
    QAudioFormat format;

    if (isOpen())
    {
        format.setSampleRate(codecContext->sample_rate);
        format.setChannelCount(codecContext->channels);
        format.setSampleFormat(mapSampleFormat(codecContext->sample_fmt));
    }

    QString msg = QString("Audio format:\n"
                          "sample rate   - %1\n"
                          "channel count - %2\n"
                          "sample format - %3" )
            .arg(format.sampleRate())
            .arg(format.channelCount())
            .arg(format.sampleFormat());

    loggable.logMessage(objectName(), QtDebugMsg, msg);

    return  format;
}

//---------------------------------------------------------------------------------------
int AudioDecoder::outputFrame(AVFrame *avFrame)
{
    std::shared_ptr<AudioFrame> audioFrame(new AudioFrame(avFrame->pts));
    int size = audioFrame->fromAvFrame(avFrame);

    if (size)
        emit audioSampleReady(audioFrame);

    return size;
}

//---------------------------------------------------------------------------------------
