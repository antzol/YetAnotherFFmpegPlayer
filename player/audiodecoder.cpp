#include "audiodecoder.h"
#include "utils.h"

//---------------------------------------------------------------------------------------
AudioDecoder::AudioDecoder(const QString &name, QObject *parent)
    : Decoder{name, parent}
{

}

//---------------------------------------------------------------------------------------
void AudioDecoder::setAudioLevelMeter(const std::shared_ptr<AudioLevelMeter> &meter)
{
    levelMeter = meter;
}

//---------------------------------------------------------------------------------------
bool AudioDecoder::open(AVStream *stream)
{
    bool res = Decoder::open(stream);
    if (!res)
        return res;

    inChannelCount = codecContext->ch_layout.nb_channels;

    outChannelCount = inChannelCount;
    outSampleRate = codecContext->sample_rate;
    outSampleFormat = codecContext->sample_fmt;
    outChannelLayout = codecContext->ch_layout;

    QString msg = QString("Audio format (FFmpeg):\n"
                          "- channel count - %2\n"
                          "- sample rate   - %1\n"
                          "- sample format - %3" )
            .arg(outChannelCount)
            .arg(outSampleRate)
            .arg(mapAvSampleFormatToString(outSampleFormat));

    loggable.logMessage(objectName(), QtDebugMsg, msg);

    return res;
}

//---------------------------------------------------------------------------------------
QAudioFormat AudioDecoder::audioFormat()
{
    QAudioFormat format;

    if (isOpen())
    {
        format.setSampleRate(outSampleRate);
        format.setChannelCount(outChannelCount);
        format.setSampleFormat(mapSampleFormat(outSampleFormat));
    }

    QString msg = QString("Audio format (Qt):\n"
                          "- channel count - %2\n"
                          "- sample rate   - %1\n"
                          "- sample format - %3" )
            .arg(format.channelCount())
            .arg(format.sampleRate())
            .arg(mapQSampleFormatToString(format.sampleFormat()));

    loggable.logMessage(objectName(), QtDebugMsg, msg);

    return  format;
}

//---------------------------------------------------------------------------------------
int AudioDecoder::inputChannelCount() const
{
    return inChannelCount;
}

//---------------------------------------------------------------------------------------
int AudioDecoder::outputFrame(AVFrame *avFrame)
{
    std::shared_ptr<AudioFrame> audioFrame(new AudioFrame(avFrame->pts));

    int size = audioFrame->fromAvFrame(avFrame);

    if (size)
        emit audioSampleReady(audioFrame);

    if (levelMeter)
        levelMeter->receiveAudioSample(avFrame);

    return size;
}

//---------------------------------------------------------------------------------------
