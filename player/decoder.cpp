#include "decoder.h"

//---------------------------------------------------------------------------------------
Decoder::Decoder(const QString &name, QObject *parent)
    : QObject{parent}
{
    setObjectName(name);
}

//---------------------------------------------------------------------------------------
Decoder::~Decoder()
{
    loggable.logMessage(objectName(), QtDebugMsg, "Destroy decoder...");

    if (codecContext)
        avcodec_free_context(&codecContext);
    if (frame)
        av_frame_free(&frame);
}

//---------------------------------------------------------------------------------------
bool Decoder::open(AVStream *stream)
{
    int result;
    int streamIndex = stream->index;
    int streamId = stream->id;

    QString msg = QString("Open decoder for stream (index %1, id %2)...").arg(streamIndex).arg(streamId);
    loggable.logMessage(objectName(), QtDebugMsg, msg);

    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec)
    {
        loggable.logMessage(objectName(), QtCriticalMsg, "Unable find decoder.");
        return false;
    }

    msg = QString("Found decoder for stream (index %1, id %2) => %3 / %4")
            .arg(streamIndex).arg(streamId).arg(codec->name, codec->long_name);
    loggable.logMessage(objectName(), QtDebugMsg, msg);

    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext)
    {
        loggable.logMessage(objectName(), QtCriticalMsg, "Unable allocate codec context.");
        return false;
    }

    result = avcodec_parameters_to_context(codecContext, stream->codecpar);
    if (result < 0)
    {
        loggable.logAvError(objectName(), QtCriticalMsg, "Unable fill codec context.", result);
        return false;
    }

    if (avcodec_open2(codecContext, codec, NULL) < 0)
    {
        loggable.logMessage(objectName(), QtCriticalMsg, "Could not open codec.");
        return false;
    }

    loggable.logMessage(objectName(), QtDebugMsg, "Allocate frame and packet for decoding.");
    frame = av_frame_alloc();
    if (!frame)
    {
        loggable.logMessage(objectName(), QtCriticalMsg, "Could not allocate frame.");
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------------------
bool Decoder::isOpen() const
{
    return frame && codecContext && avcodec_is_open(codecContext) > 0;
}

//---------------------------------------------------------------------------------------
int Decoder::decodePacket(const AVPacket *pkt)
{
    int result = 0;

    // submit the packet to the decoder
    result = avcodec_send_packet(codecContext, pkt);
    if (result < 0)
    {
        loggable.logAvError(objectName(), QtWarningMsg, "Error submitting a packet for decoding.", result);
        return result;
    }

    // get all the available frames from the decoder
    while (result >= 0)
    {
        result = avcodec_receive_frame(codecContext, frame);
        if (result < 0)
        {
            // those two return values are special and mean there is no output
            // frame available, but there were no errors during decoding
            if (result == AVERROR_EOF || result == AVERROR(EAGAIN))
                return 0;

            loggable.logAvError(objectName(), QtWarningMsg, "Error during decoding.", result);
            return result;
        }

        // write the frame data to output
        if (codecContext->codec->type == AVMEDIA_TYPE_VIDEO
                || codecContext->codec->type == AVMEDIA_TYPE_AUDIO )
            result = outputFrame(frame);

        av_frame_unref(frame);
        if (result < 0)
            return result;
    }

    return 0;
}

//---------------------------------------------------------------------------------------
