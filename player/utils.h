#ifndef UTILS_H
#define UTILS_H

#include <QAudioFormat>
#include <QMediaPlayer>
#include <QVideoFrame>

extern "C" {
#include <libavformat/avformat.h>
}

namespace AVStrings
{
constexpr const char* Language{"language"};
constexpr const char* Title{"title"};
constexpr const char* ServiceName{"service_name"};
}

template<typename T>
void safeDelete(T*& p)
{
    delete p;
    p = nullptr;
}

//---------------------------------------------------------------------------------------
constexpr const char* mapAvMediaTypeToString(AVMediaType type)
{
    switch (type)
    {
    case AVMEDIA_TYPE_VIDEO:      return "VIDEO";
    case AVMEDIA_TYPE_AUDIO:      return "AUDIO";
    case AVMEDIA_TYPE_SUBTITLE:   return "SUBTITLE";
    case AVMEDIA_TYPE_DATA:       return "DATA";
    case AVMEDIA_TYPE_ATTACHMENT: return "ATTACHMENT";
    case AVMEDIA_TYPE_NB:         return "NB";
    case AVMEDIA_TYPE_UNKNOWN:
    default:
                                  return "UNKNOWN";
    }
}

//---------------------------------------------------------------------------------------
constexpr const char* mapPlaybackStateToString(QMediaPlayer::PlaybackState state)
{
    switch (state)
    {
    case QMediaPlayer::PlayingState: return "PlAYING";
    case QMediaPlayer::PausedState:  return "PAUSED";
    case QMediaPlayer::StoppedState: return "STOPPED";
    }
    return "UNKNOWN";
}

//=======================================================================================
// Video formats...

constexpr AVPixelFormat mapPixelFormat(QVideoFrameFormat::PixelFormat pixelFormat)
{
    switch (pixelFormat)
    {
    case QVideoFrameFormat::Format_ARGB8888: return AV_PIX_FMT_ARGB;
    case QVideoFrameFormat::Format_XRGB8888: return AV_PIX_FMT_RGB32;

    case QVideoFrameFormat::Format_BGRA8888: return AV_PIX_FMT_BGRA;
    case QVideoFrameFormat::Format_XBGR8888: return AV_PIX_FMT_BGR32;

    case QVideoFrameFormat::Format_UYVY:     return AV_PIX_FMT_UYVY422;

    case QVideoFrameFormat::Format_YUYV:     return AV_PIX_FMT_YUYV422;
    case QVideoFrameFormat::Format_YUV420P:  return AV_PIX_FMT_YUV420P;

    case QVideoFrameFormat::Format_NV12:     return AV_PIX_FMT_NV12;
    case QVideoFrameFormat::Format_NV21:     return AV_PIX_FMT_NV21;

    case QVideoFrameFormat::Format_Y8:       return AV_PIX_FMT_GRAY8;
    case QVideoFrameFormat::Format_Y16:      return AV_PIX_FMT_GRAY16LE;
    case QVideoFrameFormat::Format_Jpeg:     return AV_PIX_FMT_YUVJ422P;

    default:
        return AV_PIX_FMT_NONE;
    }
}

//---------------------------------------------------------------------------------------
constexpr QVideoFrameFormat::PixelFormat mapPixelFormat(AVPixelFormat avFormat)
{
    switch (avFormat)
    {
    case AV_PIX_FMT_ARGB:     return QVideoFrameFormat::Format_ARGB8888;
    case AV_PIX_FMT_RGB32:    return QVideoFrameFormat::Format_XRGB8888;

    case AV_PIX_FMT_BGR32:    return QVideoFrameFormat::Format_XBGR8888;

    case AV_PIX_FMT_UYVY422:  return QVideoFrameFormat::Format_UYVY;

    case AV_PIX_FMT_YUYV422:  return QVideoFrameFormat::Format_YUYV;

    case AV_PIX_FMT_YUVJ420P:
    case AV_PIX_FMT_YUV420P:  return QVideoFrameFormat::Format_YUV420P;

    case AV_PIX_FMT_NV12:     return QVideoFrameFormat::Format_NV12;
    case AV_PIX_FMT_NV21:     return QVideoFrameFormat::Format_NV21;

    case AV_PIX_FMT_GRAY8:    return QVideoFrameFormat::Format_Y8;
    case AV_PIX_FMT_GRAY16LE: return QVideoFrameFormat::Format_Y16;
    case AV_PIX_FMT_YUVJ422P: return QVideoFrameFormat::Format_Jpeg;

    default:
        return QVideoFrameFormat::Format_Invalid;
    }
}

//---------------------------------------------------------------------------------------
QString mapAVActiveFormatDescriptionToString(AVActiveFormatDescription afd);

//=======================================================================================
// Audio formats...

constexpr QAudioFormat::SampleFormat mapSampleFormat(AVSampleFormat sampleFormat)
{
    switch (sampleFormat)
    {
    case AV_SAMPLE_FMT_U8:
    case AV_SAMPLE_FMT_U8P:
        return QAudioFormat::UInt8;
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_S16P:
        return QAudioFormat::Int16;
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_S32P:
        return QAudioFormat::Int32;
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_FLTP:
    case AV_SAMPLE_FMT_DBL:
    case AV_SAMPLE_FMT_DBLP:
        return QAudioFormat::Float;
    case AV_SAMPLE_FMT_NONE:
    default:
        return QAudioFormat::Unknown;
    }
}

//---------------------------------------------------------------------------------------
constexpr AVSampleFormat mapSampleFormat(QAudioFormat::SampleFormat sampleFormat)
{
    switch (sampleFormat)
    {
    case QAudioFormat::UInt8: return AV_SAMPLE_FMT_U8;
    case QAudioFormat::Int16: return AV_SAMPLE_FMT_S16;
    case QAudioFormat::Int32: return AV_SAMPLE_FMT_S32;
    case QAudioFormat::Float: return AV_SAMPLE_FMT_FLT;
    case QAudioFormat::Unknown:
    default:
        return AV_SAMPLE_FMT_NONE;
    }
}

//---------------------------------------------------------------------------------------
constexpr const char* mapAvSampleFormatToString(AVSampleFormat format)
{
    switch (format)
    {
    case AV_SAMPLE_FMT_NONE:  return "NONE";
    case AV_SAMPLE_FMT_U8:    return "UNSIGNED 8 BITS";
    case AV_SAMPLE_FMT_S16:   return "SIGNED 16 BITS";
    case AV_SAMPLE_FMT_S32:   return "SIGNED 32 BITS";
    case AV_SAMPLE_FMT_FLT:   return "FLOAT";
    case AV_SAMPLE_FMT_DBL:   return "DOUBLE";
    case AV_SAMPLE_FMT_U8P:   return "UNSIGNED 8 BITS, PLANAR";
    case AV_SAMPLE_FMT_S16P:  return "SIGNED 16 BITS, PLANAR";
    case AV_SAMPLE_FMT_S32P:  return "SIGNED 32 BITS, PLANAR";
    case AV_SAMPLE_FMT_FLTP:  return "FLOAT, PLANAR";
    case AV_SAMPLE_FMT_DBLP:  return "DOUBLE, PLANAR";
    case AV_SAMPLE_FMT_S64:   return "SIGNED 64 BITS";
    case AV_SAMPLE_FMT_S64P:  return "SIGNED 64 BITS, PLANAR";
    case AV_SAMPLE_FMT_NB:    return "NUMBER OF SAMPLE FORMATS";
    default:
        return "UNKNOWN";
    }
}

//---------------------------------------------------------------------------------------
constexpr const char* mapQSampleFormatToString(QAudioFormat::SampleFormat format)
{
    switch (format)
    {
    case QAudioFormat::Unknown: return "UNKNOWN";
    case QAudioFormat::UInt8:   return "UNSIGNED 8 BITS";
    case QAudioFormat::Int16:   return "SIGNED 16 BITS";
    case QAudioFormat::Int32:   return "SIGNED 32 BITS";
    case QAudioFormat::Float:   return "FLOAT";
    default:
        return "UNKNOWN";
    }
}

//---------------------------------------------------------------------------------------

#endif // UTILS_H
