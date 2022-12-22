#ifndef DECODER_H
#define DECODER_H

#include <QObject>

#include "loggable.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libavdevice/avdevice.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

class Decoder : public QObject
{
    Q_OBJECT
public:
    explicit Decoder(const QString& name, QObject *parent = nullptr);
    virtual ~Decoder();

    virtual bool open(AVStream *stream);
    bool isOpen() const;

    int decodePacket(const AVPacket *pkt);
    virtual int outputFrame(AVFrame *avFrame) = 0;

protected:
    void retrieveDecoderParams();
    void retrieveFrameParams();

    void logDecoderParams();
    void logFrameParams();

    void logParams(QString& initString, const std::map<QString, QString>& params);

    AVCodecContext *codecContext{nullptr};
    AVFrame *frame{nullptr};

    Loggable loggable;

    std::map<QString, QString> decoderParams;
    std::map<QString, QString> frameParams;
};

#endif // DECODER_H
