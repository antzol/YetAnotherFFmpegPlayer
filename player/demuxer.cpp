#include "demuxer.h"

#include <QCoreApplication>
#include <QAudioDevice>
#include <QMediaDevices>

#include <optional>

#include "utils.h"

//---------------------------------------------------------------------------------------
constexpr std::optional<const char*> getSourceTypeString(Demuxer::SourceType type)
{
    switch (type)
    {
    case Demuxer::SourceType::File:   return "FILE";
    case Demuxer::SourceType::Stream: return "STREAM";
    default:
        return std::nullopt;
    }
}

//---------------------------------------------------------------------------------------
int interruptCallback(void *ctx)
{
    Demuxer *demuxer = reinterpret_cast<Demuxer*>(ctx);

    if (demuxer->rwTimeoutInMilliseconds == 0)
        return 0;

    bool isTimeout = demuxer->timer.elapsed() > demuxer->rwTimeoutInMilliseconds;

    if (isTimeout)
    {
        demuxer->loggable.logMessage(demuxer->objectName(), QtWarningMsg, "Input blocking operation TIMEOUT!");
        demuxer->desiredState.store(QMediaPlayer::StoppedState);
    }
    return isTimeout;
}

//---------------------------------------------------------------------------------------
Demuxer::Demuxer(QObject *parent)
    : QObject{parent}
{
    setObjectName("Demuxer");

    receivedPacket = av_packet_alloc();
    if (!receivedPacket)
    {
        loggable.logMessage(objectName(), QtCriticalMsg, "Could not allocate packet.");
        return;
    }
}

//---------------------------------------------------------------------------------------
Demuxer::~Demuxer()
{
    stop();

    if (receivedPacket)
        av_packet_free(&receivedPacket);
}

//---------------------------------------------------------------------------------------
void Demuxer::setRwTimeout(int seconds)
{
    rwTimeoutInMilliseconds = seconds * 1000;
}

//---------------------------------------------------------------------------------------
QMediaPlayer::PlaybackState Demuxer::getCurrentState() const
{
    return currentState;
}

//---------------------------------------------------------------------------------------
void Demuxer::setVideoSink(QVideoSink *sink)
{
    videoSink = sink;
}

//---------------------------------------------------------------------------------------
int Demuxer::getFirstProgramStreamByType(int programId, AVMediaType type)
{
    int idx = -1;
    auto it = programs.find(programId);
    if (it != programs.end())
        idx = it->second->findFirstStreamByType(type);

    return idx;
}

//---------------------------------------------------------------------------------------
int Demuxer::getFirstStreamByType(AVMediaType type)
{
    auto it = std::find_if(streams.begin(), streams.end(), [&type](const auto& item){
        return item->type == type;
    });

    int idx = (it != streams.end()) ? std::distance(streams.begin(), it) : -1;
    return idx;
}

//---------------------------------------------------------------------------------------
void Demuxer::setSourceAndStart(const QString &path, Demuxer::SourceType type)
{
    std::optional<const char*> typeString = getSourceTypeString(type);

    if (!typeString)
    {
        loggable.logMessage(objectName(), QtWarningMsg, "Attempt to start with invalid source type value.");
        return;
    }

    QString msg = QString("Start with source '%1', type '%2'.").arg(path, typeString.value());
    loggable.logMessage(objectName(), QtDebugMsg, msg);

    sourcePath = path;
    sourceType = type;

    if (currentState.load() != QMediaPlayer::StoppedState)
    {
        stop();
    }
    initPlaybackThread();
}

//---------------------------------------------------------------------------------------
void Demuxer::play()
{
    loggable.logMessage(objectName(), QtDebugMsg, "Switch state to playing...");
    if (currentState.load() == QMediaPlayer::StoppedState)
    {
        loggable.logMessage(objectName(), QtDebugMsg, "Switch to playing from the stopped state.");
        initPlaybackThread();
    }
    else if (currentState.load() == QMediaPlayer::PausedState)
    {
        loggable.logMessage(objectName(), QtDebugMsg, "Switch to playing from the paused state.");
        desiredState.store(QMediaPlayer::PlayingState);
        desiredStateChanged.notify_all();
    }
    else
    {
        loggable.logMessage(objectName(), QtDebugMsg, "Current state already is playing.");
    }
}

//---------------------------------------------------------------------------------------
void Demuxer::pause()
{
    loggable.logMessage(objectName(), QtDebugMsg, "Switch state to pause...");
    desiredState.store(QMediaPlayer::PausedState);

    std::unique_lock<std::mutex> locker(stateMutex);
    while (currentState.load() != QMediaPlayer::PausedState)
    {
        currentStateChanged.wait(locker, [this](){return currentState.load() == QMediaPlayer::PausedState;});
    }
}

//---------------------------------------------------------------------------------------
void Demuxer::stop()
{
    loggable.logMessage(objectName(), QtDebugMsg, "Switch state to stop...");
    desiredState.store(QMediaPlayer::StoppedState);

    if (playbackThread.joinable())
    {
        playbackThread.join();
        loggable.logMessage(objectName(), QtDebugMsg, "Playback thread joined.");
    }

    std::unique_lock<std::mutex> locker(stateMutex);
    while (currentState.load() != QMediaPlayer::StoppedState)
    {
        currentStateChanged.wait(locker, [this](){return currentState.load() == QMediaPlayer::StoppedState;});
    }
}

//---------------------------------------------------------------------------------------
void Demuxer::changeSelectedStream(AVMediaType type, int streamIndex)
{
    bool needResume = false;

    QString msg = QString("Request for change %1 stream to index = %2...")
            .arg(mapAvMediaTypeToString(type)).arg(streamIndex);
    loggable.logMessage(objectName(), QtDebugMsg, msg);

    switch (type)
    {
    case AVMEDIA_TYPE_VIDEO:
        if (activeVideoStreamIndex.load() == streamIndex)
        {
            loggable.logMessage(objectName(), QtDebugMsg, "Requested stream and current stream area same.");
            return;
        }
        if (currentState.load() == QMediaPlayer::PlayingState)
        {
            needResume = true;
            pause();
        }
        resetVideoDecoder();
        prepareVideoDecoder(streamIndex);
        break;
    case AVMEDIA_TYPE_AUDIO:
        if (activeAudioStreamIndex.load() == streamIndex)
        {
            loggable.logMessage(objectName(), QtDebugMsg, "Requested stream and current stream area same.");
            return;
        }
        if (currentState.load() == QMediaPlayer::PlayingState)
        {
            needResume = true;
            pause();
        }
        resetAudioDecoder();
        prepareAudioDecoder(streamIndex);
        break;
    default:
        break;
    }

    if (needResume)
    {
        desiredState.store(QMediaPlayer::PlayingState);
        desiredStateChanged.notify_all();
    }

    msg = QString("Stream %1 changed. New stream index = %2.")
            .arg(mapAvMediaTypeToString(type)).arg(streamIndex);
    loggable.logMessage(objectName(), QtDebugMsg, msg);
}

//---------------------------------------------------------------------------------------
void Demuxer::writeVideoFrameToSink(const std::shared_ptr<VideoFrame> videoFrame)
{
    videoSink->setVideoFrame(*videoFrame->getVideoFrame());
}

//---------------------------------------------------------------------------------------
void Demuxer::writeAudioSampleToSink(const std::shared_ptr<AudioFrame> audioFrame)
{
    if (audioOutput && audioSink)
        audioOutput->write(audioFrame->getData(), audioFrame->getSize());
}

//---------------------------------------------------------------------------------------
void Demuxer::initPlaybackThread()
{
    loggable.logMessage(objectName(), QtDebugMsg, "Init playback thread.");

    std::unique_lock<std::mutex> locker(stateMutex);
    while (currentState.load() != QMediaPlayer::StoppedState)
    {
        currentStateChanged.wait(locker, [this](){return currentState.load() == QMediaPlayer::StoppedState;});
    }

    if (!ready)
        ready = prepare();

    if (!ready)
    {
        loggable.logMessage(objectName(), QtCriticalMsg, "Preparing of the demuxer and decoders was failed!");
        return;
    }

    loggable.logMessage(objectName(), QtDebugMsg, "Start playback thread...");
    desiredState.store(QMediaPlayer::PlayingState, std::memory_order_seq_cst);
    playbackThread = std::thread{&Demuxer::playing, this};
    playbackThread.detach();

}

//---------------------------------------------------------------------------------------
bool Demuxer::prepare()
{
    [[maybe_unused]] bool ok{true};

    emit startLockRequired(true);
    loggable.logMessage(objectName(), QtDebugMsg, QString("Load media source: %1").arg(sourcePath));

    try
    {
        loggable.logMessage(objectName(), QtDebugMsg, "Allocate input context...");
        inputFormatContext = avformat_alloc_context();
        if (!inputFormatContext)
        {
            loggable.logMessage(objectName(), QtCriticalMsg, "Could not allocate AVFormatContext.");
            throw false;
        }

        AVDictionary *options = nullptr;
        if (sourceType == SourceType::Stream && rwTimeoutInMilliseconds > 0)
        {
            QString msg = QString("Set interrupt callback and blocking RW operations timeout: "
                                  "%1 milliseconds.").arg(rwTimeoutInMilliseconds);
            loggable.logMessage(objectName(), QtDebugMsg, msg);

            inputFormatContext->interrupt_callback.callback = interruptCallback;
            inputFormatContext->interrupt_callback.opaque = this;// inputFormatContext;
        }

        timer.restart();
        loggable.logMessage(objectName(), QtDebugMsg, "Open input context...");
        if (avformat_open_input(&inputFormatContext, sourcePath.toUtf8().data(), NULL, &options) < 0)
        {
            loggable.logMessage(objectName(), QtCriticalMsg, QString("Could not open source: %1").arg(sourcePath));
            throw false;
        }

        loggable.logMessage(objectName(), QtDebugMsg, "Find stream info...");
        if (avformat_find_stream_info(inputFormatContext, NULL) < 0)
        {
            loggable.logMessage(objectName(), QtCriticalMsg, "Could not find stream information.");
            throw false;
        }

        if (!findStreams())
        {
            loggable.logMessage(objectName(), QtWarningMsg, "Valid streams does not found.");
            throw false;
        }

        findPrograms();

        // it's left for debugging
        //    int videoIndex = getFirstStreamByType(AVMEDIA_TYPE_VIDEO);
        //    if (videoIndex != -1)
        //    {
        //        ok = prepareVideoDecoder(videoIndex);
        //        if (!ok)
        //            throw false;
        //    }

        //    int audioIndex = getFirstStreamByType(AVMEDIA_TYPE_AUDIO);
        //    if (audioIndex != -1)
        //    {
        //        ok = prepareAudioDecoder(audioIndex);
        //        if (!ok)
        //            throw false;
        //    }
    }
    catch (const bool &e)
    {
        ok = e;
    }

    emit startLockRequired(false);
    return ok;
}

//---------------------------------------------------------------------------------------
bool Demuxer::findStreams()
{
    loggable.logMessage(objectName(), QtDebugMsg, "Find streams...");

    streams.clear();

    if (!inputFormatContext)
        return false;

    streams.resize(inputFormatContext->nb_streams);

    QString msg;
    QString msgPattern = QString("Stream: index - %1, id - %2, type - %3.");

    std::unordered_map<AVMediaType, int> streamsCounts;

    for (unsigned int i = 0; i < inputFormatContext->nb_streams; ++i)
    {
        AVStream *stream = inputFormatContext->streams[i];
        AVMediaType type = stream->codecpar->codec_type;
        int idx = stream->index;

        auto streamInfo = std::make_shared<StreamInfo>();
        streamInfo->index = idx;
        streamInfo->stream = stream;
        streamInfo->type = type;
        streamInfo->id = stream->id;

        streamsCounts[type]++;

        msg = msgPattern.arg(idx).arg(stream->id).arg(mapAvMediaTypeToString(type));

        AVDictionary *meta = stream->metadata;
        AVDictionaryEntry *entry = av_dict_get(meta, "", nullptr, AV_DICT_IGNORE_SUFFIX);
        if (entry)
        {
            msg.append("\nMetadata:");
            while (entry)
            {
                streamInfo->properties[entry->key] = entry->value;
                QString str = QString("\n%1 = %2");
                msg.append(str.arg(entry->key, entry->value));
                entry = av_dict_get(meta, "", entry, AV_DICT_IGNORE_SUFFIX);
            }
        }
        streams[idx] = streamInfo;
        loggable.logMessage(objectName(), QtDebugMsg, msg);
    }

    msg = QString("Found %1 video and %2 audio streams")
            .arg(streamsCounts[AVMEDIA_TYPE_VIDEO])
            .arg(streamsCounts[AVMEDIA_TYPE_AUDIO]);
    loggable.logMessage(objectName(), QtDebugMsg, msg);

    emit streamsFound(streams);

    return streamsCounts[AVMEDIA_TYPE_VIDEO] + streamsCounts[AVMEDIA_TYPE_AUDIO];
}

//---------------------------------------------------------------------------------------
bool Demuxer::findPrograms()
{
    loggable.logMessage(objectName(), QtDebugMsg, "Find programs...");

    programs.clear();

    if (!inputFormatContext)
        return false;

    QString msg;
    QString msgPattern = QString("Program: id - %1, program number - %2, PMT PID - %3, PCR PID - %4.");

    AVProgram *avProgram;
    for (unsigned int i = 0; i < inputFormatContext->nb_programs; ++i)
    {
        avProgram = inputFormatContext->programs[i];

        auto programInfo = std::make_shared<ProgramInfo>();
        programInfo->avProgram = avProgram;
        programInfo->lastPmtVersion = avProgram->pmt_version;

        msg = msgPattern.arg(avProgram->id)
                .arg(avProgram->program_num)
                .arg(avProgram->pmt_pid)
                .arg(avProgram->pcr_pid);

        AVDictionary *meta = avProgram->metadata;
        AVDictionaryEntry *entry = av_dict_get(meta, "", nullptr, AV_DICT_IGNORE_SUFFIX);
        if (entry)
        {
            msg.append("\nMetadata:");
            while (entry)
            {
                programInfo->properties[entry->key] = entry->value;
                QString str = QString("\n%1 = %2");
                msg.append(str.arg(entry->key, entry->value));
                entry = av_dict_get(meta, "", entry, AV_DICT_IGNORE_SUFFIX);
            }
            loggable.logMessage(objectName(), QtDebugMsg, msg);
        }

        fillProgramStreamsData(programInfo);
        programs[avProgram->id] = programInfo;

    }

    msg = QString("Found %1 programs").arg(programs.size());
    loggable.logMessage(objectName(), QtDebugMsg, msg);

    emit programsFound(programs);

    return !programs.empty();
}

//---------------------------------------------------------------------------------------
void Demuxer::fillProgramStreamsData(std::shared_ptr<ProgramInfo> program)
{
    QString msg = QString("Fill the stream data for the program %1").arg(program->avProgram->id);
    loggable.logMessage(objectName(), QtDebugMsg, msg);

    AVProgram *avProgram = program->avProgram;
    QString msgPattern = QString("Program %1: stream #%2 - PID %3, type '%4'");
    for (unsigned int i = 0; i < avProgram->nb_stream_indexes; ++i)
    {
        int idx = avProgram->stream_index[i];
        AVStream *stream = inputFormatContext->streams[idx];
        AVMediaType type = stream->codecpar->codec_type;

        program->streams[idx] = streams[idx];

        msg = msgPattern
                .arg(avProgram->id)
                .arg(i+1)
                .arg(stream->id)
                .arg(QString::fromStdString(mapAvMediaTypeToString(type)));

        loggable.logMessage(objectName(), QtDebugMsg, msg);
    }
}

//---------------------------------------------------------------------------------------
void Demuxer::playing()
{
    loggable.logMessage(objectName(), QtDebugMsg, "Run decoding.");

    if (!ready || !receivedPacket)
    {
        loggable.logMessage(objectName(), QtCriticalMsg, "Player does not ready for decoding.");
        return;
    }

    currentState.store(QMediaPlayer::PlayingState);

    notifyPlaybackState();

    if (sourceType == SourceType::Stream)
        av_read_play(inputFormatContext);

    int result = 0;
    loggable.logMessage(objectName(), QtDebugMsg, "Enter to decoding loop...");
    while (desiredState.load() != QMediaPlayer::StoppedState)
    {

        if (desiredState.load() == QMediaPlayer::PausedState)
        {
            if (currentState.load() != QMediaPlayer::PausedState)
            {
                currentState.store(QMediaPlayer::PausedState);
                if (sourceType == SourceType::Stream)
                    av_read_pause(inputFormatContext);

                currentStateChanged.notify_all();
                notifyPlaybackState();
            }

            std::unique_lock<std::mutex> locker(stateMutex);
            while (desiredState.load() == QMediaPlayer::PausedState)
            {
                desiredStateChanged.wait(locker, [this](){return desiredState.load() != QMediaPlayer::PausedState;});
            }
            currentState.store(QMediaPlayer::PlayingState);
            currentStateChanged.notify_all();
            notifyPlaybackState();
            startDTS = -1;
        }

        timer.restart();
        if (av_read_frame(inputFormatContext, receivedPacket) < 0)
            break;

        if (receivedPacket->stream_index == activeVideoStreamIndex.load())
        {
            waitForReachPtsTime(receivedPacket);

            if (videoDecoder && videoDecoder->isOpen())
                result = videoDecoder->decodePacket(receivedPacket);

        }
        else if (receivedPacket->stream_index == activeAudioStreamIndex.load())
        {
            if (activeVideoStreamIndex.load() == -1)
                waitForReachPtsTime(receivedPacket);

            if (audioDecoder && audioDecoder->isOpen())
                result = audioDecoder->decodePacket(receivedPacket);
        }
        av_packet_unref(receivedPacket);
        if (result < 0)
        {
            // ignore decoding errors due to possible scrambling, only logging
//            QString msg = QString("ERROR of packet decoding (stream (index/id, type): %1/%2, %3.")
//                    .arg(receivedPacket->stream_index)
//                    .arg(streams[receivedPacket->stream_index]->id)
//                    .arg(mapAvMediaTypeToString(streams[receivedPacket->stream_index]->type));
//            loggable.logAvError(objectName(), QtWarningMsg, msg, result);
        }
    }
    loggable.logMessage(objectName(), QtDebugMsg, "Exit from the playing loop.");
    currentState.store(QMediaPlayer::StoppedState);
    if (sourceType == SourceType::Stream)
        av_read_pause(inputFormatContext);
    notifyPlaybackState();
    reset();
    currentStateChanged.notify_all();
}

//---------------------------------------------------------------------------------------
void Demuxer::waitForReachPtsTime(AVPacket *packet)
{
    AVRational time_base = streams[packet->stream_index]->stream->time_base;
    AVRational time_base_q = {1,AV_TIME_BASE};
    int64_t pts_time = av_rescale_q(packet->dts, time_base, time_base_q);
    if (startDTS < 0)
    {
        startDTS = pts_time;
        startTime = av_gettime();
    }
    else
    {
        int64_t nowTime = av_gettime() - startTime;
        if ((pts_time - startDTS) > nowTime)
            av_usleep(pts_time - startDTS - nowTime);
    }
}

//---------------------------------------------------------------------------------------
void Demuxer::notifyPlaybackState()
{
    QString msg = QString("Playback state changed: %1").arg(mapPlaybackStateToString(currentState));
    loggable.logMessage(objectName(), QtDebugMsg, msg);

    emit playbackStateChanged(currentState);
}

//---------------------------------------------------------------------------------------
bool Demuxer::prepareVideoDecoder(int streamIndex)
{
    loggable.logMessage(objectName(), QtDebugMsg, "Prepare video decoder.");
    QString msg;

    if (streamIndex < 0 || streamIndex >= streams.size())
    {
        msg = QString("Selected video stream (index = %1) doesn't found.").arg(streamIndex);
        loggable.logMessage(objectName(), QtWarningMsg, msg);
        return false;
    }

    if (streams[streamIndex]->type != AVMEDIA_TYPE_VIDEO)
    {
        msg = QString("Selected stream (index = %1) doesn't video.").arg(streamIndex);
        loggable.logMessage(objectName(), QtWarningMsg, msg);
        return false;
    }

    loggable.logMessage(objectName(), QtDebugMsg,
                    QString("Video stream selected. Stream index: %1.").arg(streamIndex));

    videoDecoder = new VideoDecoder("Video Decoder");
    connect(videoDecoder, &VideoDecoder::videoFrameReady, this, &Demuxer::writeVideoFrameToSink);


    bool ok = videoDecoder->open(streams[streamIndex]->stream);
    if (!ok)
        return false;

    QSize pictureSize = videoDecoder->getPictureSize();
    msg = QString("Video image size: %1x%2.").arg(pictureSize.width()).arg(pictureSize.height());
    loggable.logMessage(objectName(), QtDebugMsg, msg);

    activeVideoStreamIndex.store(streamIndex);
    return true;
}

//---------------------------------------------------------------------------------------
void Demuxer::resetVideoDecoder()
{
    loggable.logMessage(objectName(), QtDebugMsg, "Reset video decoder.");
    activeVideoStreamIndex.store(-1);

    if (videoDecoder)
        videoDecoder->deleteLater();
    videoDecoder = nullptr;
    startDTS = -1;
    startTime = -1;
}

//---------------------------------------------------------------------------------------
bool Demuxer::prepareAudioDecoder(int streamIndex)
{
    loggable.logMessage(objectName(), QtDebugMsg, "Prepare audiodecoder.");

    QString msg;

    if (streamIndex < 0 || streamIndex >= streams.size())
    {
        msg = QString("Selected audio stream (index = %1) doesn't found.").arg(streamIndex);
        loggable.logMessage(objectName(), QtWarningMsg, msg);
        return false;
    }

    if (streams[streamIndex]->type != AVMEDIA_TYPE_AUDIO)
    {
        msg = QString("Selected stream (index = %1) doesn't audio.").arg(streamIndex);
        loggable.logMessage(objectName(), QtWarningMsg, msg);
        return false;
    }

    loggable.logMessage(objectName(), QtDebugMsg,
                    QString("Audio stream selected. Stream index: %1.").arg(streamIndex));

    audioDecoder = new AudioDecoder("Audio Decoder");
    connect(audioDecoder, &AudioDecoder::audioSampleReady, this, &Demuxer::writeAudioSampleToSink);

    bool ok = audioDecoder->open(streams[streamIndex]->stream);
    if (ok)
    {
        QAudioFormat format = audioDecoder->audioFormat();
        QAudioDevice defaultAudioOutput(QMediaDevices::defaultAudioOutput());

        if (defaultAudioOutput.mode() == QAudioDevice::Output)
        {
            audioSink = new QAudioSink(defaultAudioOutput, format);
            audioOutput = audioSink->start();
        }
        activeAudioStreamIndex.store(streamIndex);
    }

    return ok;
}

//---------------------------------------------------------------------------------------
void Demuxer::resetAudioDecoder()
{
    loggable.logMessage(objectName(), QtDebugMsg, "Reset audio decoder...");

    activeAudioStreamIndex.store(-1);

    if (audioDecoder)
        audioDecoder->deleteLater();
    audioDecoder = nullptr;

    startDTS = -1;
    startTime = -1;

    if (audioSink)
        audioSink->deleteLater();
    audioSink = nullptr;

    if (audioOutput)
        audioOutput->deleteLater();
    audioOutput = nullptr;
}

//---------------------------------------------------------------------------------------
void Demuxer::reset()
{
    loggable.logMessage(objectName(), QtDebugMsg, "Reset player...");
    ready = false;
    startDTS = -1;

    if (inputFormatContext)
        avformat_close_input(&inputFormatContext);

    resetVideoDecoder();
    resetAudioDecoder();

    programs.clear();
    streams.clear();

    emit programsFound(programs);
    emit streamsFound(streams);
}

//---------------------------------------------------------------------------------------
int ProgramInfo::findFirstStreamByType(AVMediaType type)
{
    auto it = std::find_if(streams.begin(), streams.end(), [&type](const auto& item){
        return item.second.lock()->type == type;
    });

    int idx = (it != streams.end()) ? it->second.lock()->index : -1;
    return idx;
}

//---------------------------------------------------------------------------------------
