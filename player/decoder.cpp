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

    msg = QString("Max low resolution (lowres) value supported by the decoder: %1")
            .arg(codec->max_lowres);
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

        if (decoderParams.empty())
        {
            retrieveDecoderParams();
            logDecoderParams();
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
void Decoder::retrieveDecoderParams()
{
    char buf[256];

    /// TODO: remove parameters not used during decoding

    decoderParams["codec_type"] = mapAvMediaTypeToString(codecContext->codec_type);
    decoderParams["codec / name"] = codecContext->codec->name;
    decoderParams["codec / long_name"] = codecContext->codec->long_name;


    decoderParams["bit_rate"] = QString::number(codecContext->bit_rate);
    decoderParams["bit_rate_tolerance"] = QString::number(codecContext->bit_rate_tolerance);
    decoderParams["global_quality"] = QString::number(codecContext->global_quality);
    decoderParams["compression_level"] = QString::number(codecContext->compression_level);
    decoderParams["flags"] = QString::number(codecContext->flags, 2);
    decoderParams["flags2"] = QString::number(codecContext->flags2, 2);
    decoderParams["time_base"] = QString("%1 / %2").arg(codecContext->time_base.num).arg(codecContext->time_base.den);
    decoderParams["ticks_per_frame"] = QString::number(codecContext->ticks_per_frame);
    decoderParams["delay"] = QString::number(codecContext->delay);
    decoderParams["width / height"] = QString("%1 / %2").arg(codecContext->width).arg(codecContext->height);
    decoderParams["coded_width / coded_height"] = QString("%1 / %2").arg(codecContext->coded_width).arg(codecContext->coded_height);
    decoderParams["gop_size"] = QString::number(codecContext->gop_size);

    av_get_pix_fmt_string(buf, sizeof(buf), static_cast<AVPixelFormat>(codecContext->pix_fmt));
    decoderParams["pix_fmt"] = buf;

    decoderParams["max_b_frames"] = QString::number(codecContext->max_b_frames);
    decoderParams["b_quant_factor"] = QString::number(codecContext->b_quant_factor);
    decoderParams["b_quant_offset"] = QString::number(codecContext->b_quant_offset);
    decoderParams["has_b_frames"] = QString::number(codecContext->has_b_frames);
    decoderParams["i_quant_factor"] = QString::number(codecContext->i_quant_factor);
    decoderParams["i_quant_offset"] = QString::number(codecContext->i_quant_offset);
    decoderParams["lumi_masking"] = QString::number(codecContext->lumi_masking);
    decoderParams["temporal_cplx_masking"] = QString::number(codecContext->temporal_cplx_masking);
    decoderParams["spatial_cplx_masking"] = QString::number(codecContext->spatial_cplx_masking);
    decoderParams["p_masking"] = QString::number(codecContext->p_masking);
    decoderParams["dark_masking"] = QString::number(codecContext->dark_masking);
    decoderParams["slice_count"] = QString::number(codecContext->slice_count);
    decoderParams["sample_aspect_ratio"] = QString("%1 / %2").arg(codecContext->sample_aspect_ratio.num).arg(codecContext->sample_aspect_ratio.den);
    decoderParams["skip_top"] = QString::number(codecContext->skip_top);
    decoderParams["skip_bottom"] = QString::number(codecContext->skip_bottom);
    decoderParams["refs"] = QString::number(codecContext->refs);

    decoderParams["color_primaries"] = av_color_primaries_name(codecContext->color_primaries);
    decoderParams["color_trc"] = av_color_transfer_name(codecContext->color_trc);
    decoderParams["colorspace"] = av_color_space_name(codecContext->colorspace);
    decoderParams["color_range"] = av_color_range_name(codecContext->color_range);
    decoderParams["chroma_sample_location"] = av_chroma_location_name(codecContext->chroma_sample_location);

    decoderParams["slices"] = QString::number(codecContext->slices);
    decoderParams["field_order"] = mapAvFieldOrderToString(codecContext->field_order);
    decoderParams["sample_rate"] = QString::number(codecContext->sample_rate);

    AVSampleFormat sampleFormat = codecContext->sample_fmt;
    switch (codecContext->codec->type)
    {
    case AVMEDIA_TYPE_VIDEO:
        av_get_pix_fmt_string(buf, sizeof(buf), static_cast<AVPixelFormat>(sampleFormat));
        break;
    case AVMEDIA_TYPE_AUDIO:
        av_get_sample_fmt_string(buf, sizeof(buf), static_cast<AVSampleFormat>(sampleFormat));
        break;
    default:
        break;
    }
    frameParams["sample_fmt"] = buf;

    decoderParams["frame_size"] = QString::number(codecContext->frame_size);
    decoderParams["frame_number"] = QString::number(codecContext->frame_number);
    decoderParams["block_align"] = QString::number(codecContext->block_align);

    decoderParams["audio_service_type"] = mapAvAudioServiceTypeToString(codecContext->audio_service_type);

    decoderParams["qcompress"] = QString::number(codecContext->qcompress);
    decoderParams["qblur"] = QString::number(codecContext->qblur);
    decoderParams["rc_max_rate"] = QString::number(codecContext->rc_max_rate);

    decoderParams["idct_algo"] = QString::number(codecContext->idct_algo);
    decoderParams["bits_per_coded_sample"] = QString::number(codecContext->bits_per_coded_sample);
    decoderParams["bits_per_raw_sample"] = QString::number(codecContext->bits_per_raw_sample);

    /// TODO: test low resolution setting
    decoderParams["lowres"] = QString::number(codecContext->lowres);

    decoderParams["thread_count"] = QString::number(codecContext->thread_count);
    decoderParams["thread_type"] = QString::number(codecContext->thread_type);
    decoderParams["active_thread_type"] = QString::number(codecContext->active_thread_type);
    decoderParams["profile"] = QString::number(codecContext->profile);

    /// TODO: test setting
    decoderParams["level"] = QString::number(codecContext->level);

    decoderParams["skip_idct"] = mapAvDiscardToString(codecContext->skip_idct);
    decoderParams["skip_frame"] = mapAvDiscardToString(codecContext->skip_frame);

    decoderParams["subtitle_header_size"] = QString::number(codecContext->subtitle_header_size);
    decoderParams["initial_padding"] = QString::number(codecContext->initial_padding);

    decoderParams["framerate"] = QString("%1 / %2").arg(codecContext->framerate.num).arg(codecContext->framerate.den);
    decoderParams["pkt_timebase"] = QString("%1 / %2").arg(codecContext->pkt_timebase.num).arg(codecContext->pkt_timebase.den);

    decoderParams["pts_correction_num_faulty_pts"] = QString::number(codecContext->pts_correction_num_faulty_pts);
    decoderParams["pts_correction_num_faulty_dts"] = QString::number(codecContext->pts_correction_num_faulty_dts);
    decoderParams["pts_correction_last_pts"] = QString::number(codecContext->pts_correction_last_pts);
    decoderParams["pts_correction_last_dts"] = QString::number(codecContext->pts_correction_last_dts);

    decoderParams["seek_preroll"] = QString::number(codecContext->seek_preroll);
    decoderParams["nb_coded_side_data"] = QString::number(codecContext->nb_coded_side_data);

    /// TODO: test
    decoderParams["apply_cropping"] = QString::number(codecContext->apply_cropping);

}

//---------------------------------------------------------------------------------------
void Decoder::retrieveFrameParams()
{
    frameParams["width / height"] = QString("%1 / %2")
            .arg(frame->width)
            .arg(frame->height);

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
void Decoder::logDecoderParams()
{
    QString msg{"Decoder parameters:\n"};
    logParams(msg, decoderParams);
}

//---------------------------------------------------------------------------------------
void Decoder::logFrameParams()
{
    QString msg{"Frame parameters:\n"};
    logParams(msg, frameParams);
}

//---------------------------------------------------------------------------------------
void Decoder::logParams(QString &initMessage, const std::map<QString, QString> &params)
{
    for (auto& [key, val] : params)
    {
        initMessage.append(QString("- %1: %2\n").arg(key, val));
    }
    loggable.logMessage(objectName(), QtInfoMsg, initMessage);
}

//---------------------------------------------------------------------------------------
