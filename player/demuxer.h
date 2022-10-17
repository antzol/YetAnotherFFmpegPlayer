#ifndef DEMUXER_H
#define DEMUXER_H

#include <QObject>

#include <condition_variable>
#include <thread>

#include <QElapsedTimer>
#include <QMutex>

#include <QAudioOutput>
#include <QAudioSink>
#include <QVideoFrame>
#include <QVideoSink>
#include <QMediaPlayer>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include "loggable.h"

#include "audiodecoder.h"
#include "audioframe.h"
#include "videodecoder.h"
#include "videoframe.h"


struct StreamInfo
{
    int index;
    int id;
    AVMediaType type;
    AVStream *stream;
    std::unordered_map<std::string, std::string> properties;
};

struct ProgramInfo {
    int findFirstStreamByType(AVMediaType type);

    AVProgram *avProgram{nullptr};
    int lastPmtVersion{-1};
    std::unordered_map<int, std::weak_ptr<StreamInfo>> streams;
    std::unordered_map<std::string, std::string> properties;
};

static int interruptCallback(void *ctx);

class Demuxer : public QObject
{
    Q_OBJECT
public:
    enum SourceType{
        File,
        Stream
    };

    explicit Demuxer(QObject *parent = nullptr);
    virtual ~Demuxer();
    void setRwTimeout(int seconds);

    QMediaPlayer::PlaybackState getCurrentState() const;

    void setVideoSink(QVideoSink *sink);

    int getFirstProgramStreamByType(int programId, AVMediaType type);
    int getFirstStreamByType(AVMediaType type);

    friend int interruptCallback(void *ctx);

public slots:
    void setSourceAndStart(const QString &path, Demuxer::SourceType type);

    void play();
    void pause();
    void stop();

    void changeSelectedStream(AVMediaType type, int streamIndex);

    void writeVideoFrameToSink(const std::shared_ptr<VideoFrame> videoFrame);
    void writeAudioSampleToSink(const std::shared_ptr<AudioFrame> audioFrame);

signals:
    void streamsFound(const std::vector<std::shared_ptr<StreamInfo>> &streams);
    void programsFound(const std::map<int, std::shared_ptr<ProgramInfo>> &programs);

    void playbackStateChanged(QMediaPlayer::PlaybackState state);

    void startLockRequired(bool locked);

private:
    void initPlaybackThread();
    bool prepare();

    bool findStreams();
    bool findPrograms();

    void fillProgramStreamsData(std::shared_ptr<ProgramInfo> program);

    void playing();
    void waitForReachPtsTime(AVPacket *packet);

    void notifyPlaybackState();

    bool prepareVideoDecoder(int streamIndex);
    void resetVideoDecoder();

    bool prepareAudioDecoder(int streamIndex);
    void resetAudioDecoder();

    void reset();

    QString sourcePath;
    int sourceType{-1};
    int rwTimeoutInMilliseconds{0};

    // key = program id (SID, service id)
    std::map<int, std::shared_ptr<ProgramInfo>> programs;
    // index in the vector = index of the stream in the AVFormatContext
    std::vector<std::shared_ptr<StreamInfo>> streams;

    std::atomic<int> activeVideoStreamIndex{-1};
    std::atomic<int> activeAudioStreamIndex{-1};

    bool ready{false};
    std::atomic<QMediaPlayer::PlaybackState> desiredState{QMediaPlayer::StoppedState};
    std::atomic<QMediaPlayer::PlaybackState> currentState{QMediaPlayer::StoppedState};
    std::condition_variable currentStateChanged;
    std::condition_variable desiredStateChanged;
    QElapsedTimer timer;
    std::mutex stateMutex;
    std::thread playbackThread;

    int64_t startDTS{-1};
    int64_t startTime{-1};

    AVFormatContext *inputFormatContext{nullptr};
    AVPacket *receivedPacket{nullptr};

    VideoDecoder *videoDecoder{nullptr};
    AudioDecoder *audioDecoder{nullptr};

    QVideoSink *videoSink{nullptr};
    QAudioSink *audioSink{nullptr};
    QIODevice *audioOutput{nullptr};

    Loggable loggable;
};

#endif // DEMUXER_H
