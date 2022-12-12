#include "decoder.h"
#include "utils.h"

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

        if (frameParams.empty())
        {
            retrieveFrameParams();
            logFrameParams();
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
void Decoder::retrieveFrameParams()
{
    frameParams["width"] = QString::number(frame->width);
    frameParams["height"] = QString::number(frame->height);
    frameParams["nb_samples"] = QString::number(frame->nb_samples);

    int format = frame->format;
    char buf[256];
    switch (codecContext->codec->type)
    {
    case AVMEDIA_TYPE_VIDEO:
        av_get_pix_fmt_string(buf, sizeof(buf), static_cast<AVPixelFormat>(format));
        break;
    case AVMEDIA_TYPE_AUDIO:
        av_get_sample_fmt_string(buf, sizeof(buf), static_cast<AVSampleFormat>(format));
        break;
    default:
        break;
    }
    frameParams["format"] = buf;

    frameParams["sample_aspect_ratio"] = QString("%1 / %2")
            .arg(frame->sample_aspect_ratio.num)
            .arg(frame->sample_aspect_ratio.den);

    frameParams["pts"] = QString::number(frame->pts);
    frameParams["pkt_dts"] = QString::number(frame->pkt_dts);
    frameParams["time_base"] = QString("%1 / %2")
            .arg(frame->time_base.num).arg(frame->time_base.den);

    frameParams["coded_picture_number"] = QString::number(frame->coded_picture_number);
    frameParams["display_picture_number"] = QString::number(frame->display_picture_number);
    frameParams["quality"] = QString::number(frame->quality);
    frameParams["repeat_pict"] = QString::number(frame->repeat_pict);
    frameParams["interlaced_frame"] = QString::number(frame->interlaced_frame);
    frameParams["top_field_first"] = QString::number(frame->top_field_first);
    frameParams["palette_has_changed"] = QString::number(frame->palette_has_changed);
    frameParams["reordered_opaque"] = QString::number(frame->reordered_opaque);
    frameParams["sample_rate"] = QString::number(frame->sample_rate);
    frameParams["side_data / number"] = QString::number(frame->nb_side_data);
    if (frame->nb_side_data)
    {
        AVFrameSideData *sideData = static_cast<AVFrameSideData*>(*frame->side_data);
        frameParams["side_data / type"] = av_frame_side_data_name(sideData->type);

        if (sideData->type == AV_FRAME_DATA_AFD)
            frameParams["side_data / active format description"] =
                    mapAVActiveFormatDescriptionToString(static_cast<AVActiveFormatDescription>(*sideData->data));

        AVDictionary *meta = sideData->metadata;
        AVDictionaryEntry *entry = av_dict_get(meta, "", nullptr, AV_DICT_IGNORE_SUFFIX);
        if (entry)
        {
            while (entry)
            {
                QString key = QString("side_data / %1").arg(entry->key);
                frameParams[key] = entry->value;
                entry = av_dict_get(meta, "", entry, AV_DICT_IGNORE_SUFFIX);
            }
        }
    }

    AVDictionary *meta = frame->metadata;
    AVDictionaryEntry *entry = av_dict_get(meta, "", nullptr, AV_DICT_IGNORE_SUFFIX);
    if (entry)
    {
        while (entry)
        {
            QString key = QString("metadata / %1").arg(entry->key);
            frameParams[key] = entry->value;
            entry = av_dict_get(meta, "", entry, AV_DICT_IGNORE_SUFFIX);
        }
    }

    frameParams["color_range"] = av_color_range_name(frame->color_range);
    frameParams["color_primaries"] = av_color_primaries_name(frame->color_primaries);
    frameParams["colorspace"] = av_color_space_name(frame->colorspace);
    frameParams["color_trc"] = av_color_transfer_name(frame->color_trc);
    frameParams["chroma_location"] = av_chroma_location_name(frame->chroma_location);
    frameParams["decode_error_flags"] = QString::number(frame->decode_error_flags);
    frameParams["pkt_pos"] = QString::number(frame->pkt_pos);
    frameParams["pkt_size"] = QString::number(frame->pkt_size);
    frameParams["crop_top"] = QString::number(frame->crop_top);
    frameParams["crop_bottom"] = QString::number(frame->crop_bottom);
    frameParams["crop_left"] = QString::number(frame->crop_left);
    frameParams["crop_right"] = QString::number(frame->crop_right);

    frameParams["linesize[0]"] = QString::number(frame->linesize[0]);
    frameParams["linesize[1]"] = QString::number(frame->linesize[1]);
    frameParams["linesize[2]"] = QString::number(frame->linesize[2]);


}

//---------------------------------------------------------------------------------------
void Decoder::logFrameParams()
{
    QString msg{"Frame parameters:\n"};

    for (auto& [key, val] : frameParams)
    {
        msg.append(QString("- %1: %2\n").arg(key, val));
    }
    loggable.logMessage(objectName(), QtInfoMsg, msg);

}

//---------------------------------------------------------------------------------------
