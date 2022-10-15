#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>

extern "C" {
#include <libavutil/frame.h>
}

class Frame
{
public:
    Frame(int64_t pts);
    virtual ~Frame();

    int64_t getPresentationTimestamp() const;
    virtual int fromAvFrame(const AVFrame *avFrame) = 0;

protected:
    int64_t pts;

};

#endif // FRAME_H
