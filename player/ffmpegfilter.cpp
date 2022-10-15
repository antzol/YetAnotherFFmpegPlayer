#include "ffmpegfilter.h"

//---------------------------------------------------------------------------------------
FFmpegFilter::FFmpegFilter(const QString &name, QObject *parent)
    : QObject{parent}
{
    setObjectName(name);
}

//---------------------------------------------------------------------------------------
FFmpegFilter::~FFmpegFilter()
{
    loggable.logMessage(objectName(), QtDebugMsg, QString("Destroy filter %1...").arg(filterName));

    if (graph)
        avfilter_graph_free(&graph);

    if (outFrame)
        av_frame_free(&outFrame);
}

//---------------------------------------------------------------------------------------
bool FFmpegFilter::init(const QString &filter, const QString &params, const QString &format)
{
    filterName = filter;
    filterParams = params;
    inputBufferParams = format;
    return init();
}

//---------------------------------------------------------------------------------------
bool FFmpegFilter::init()
{
    if (ready)
        return true;

    if (filterName.isEmpty() || filterParams.isEmpty() || inputBufferParams.isEmpty())
    {
        QString emptyParam;
        if (filterName.isEmpty())
            emptyParam = "Filter name";
        else if (filterParams.isEmpty())
            emptyParam = "Filter params";
        else
            emptyParam = "Input buffer params";
        loggable.logMessage(objectName(), QtCriticalMsg, QString("%1 must not be empty").arg(emptyParam));
        return false;
    }

    loggable.logMessage(objectName(), QtDebugMsg, QString("Init filter graph for %1...").arg(filterName));

    AVFilterInOut *inputs = nullptr;
    AVFilterInOut *outputs = nullptr;

    try
    {
        int result;

        const AVFilter *bufferSource   = avfilter_get_by_name("buffer");
        const AVFilter *bufferSink  = avfilter_get_by_name("buffersink");
        inputs  = avfilter_inout_alloc();
        outputs = avfilter_inout_alloc();

        graph = avfilter_graph_alloc();

        if (!inputs || !outputs || !graph)
        {
            result = ENOMEM;
            loggable.logAvError(objectName(), QtCriticalMsg, "Cannot allocate inputs, outputs or graph", result);
            throw result;
        }

        QByteArray argsArray = inputBufferParams.toLocal8Bit();
        char *args = argsArray.data();

        QByteArray descrArray = QString("%1=%2").arg(filterName, filterParams).toLocal8Bit();
        const char *description = descrArray.data();

        loggable.logMessage(objectName(), QtDebugMsg,
                        QString("Config:\n"
                                "Filter  : %1\n"
                                "Settings: %2").arg(description, args));

        graph = avfilter_graph_alloc();
        result = avfilter_graph_create_filter(&bufferSourceContext, bufferSource, "in",
                                              args, NULL, graph);
        if (result < 0)
        {
            loggable.logAvError(objectName(), QtCriticalMsg, "Filter graph - Unable to create buffer source.", result);
            throw result;
        }

        result = avfilter_graph_create_filter(&bufferSinkContext, bufferSink, "out",
                                              NULL, NULL, graph);
        if (result < 0)
        {
            loggable.logAvError(objectName(), QtCriticalMsg, "Filter graph - Unable to create buffer sink.", result);
            throw result;
        }

        inputs->name        = av_strdup("out");
        inputs->filter_ctx  = bufferSinkContext;
        inputs->pad_idx     = 0;
        inputs->next        = NULL;

        outputs->name       = av_strdup("in");
        outputs->filter_ctx = bufferSourceContext;
        outputs->pad_idx    = 0;
        outputs->next       = NULL;

        result = avfilter_graph_parse_ptr(graph, description, &inputs, &outputs, NULL);
        if (result < 0)
        {
            loggable.logAvError(objectName(), QtCriticalMsg, "ERROR of the avfilter_graph_parse_ptr.", result);
            throw result;
        }

        result = avfilter_graph_config(graph, NULL);
        if (result < 0)
        {
            loggable.logAvError(objectName(), QtCriticalMsg, "ERROR of the avfilter_graph_config.", result);
            throw result;
        }

        outFrame = av_frame_alloc();
        if (!outFrame)
        {
            result = ENOMEM;
            loggable.logAvError(objectName(), QtCriticalMsg, "Could not allocate frame for filtering.", result);
            throw result;
        }

        ready = true;

        loggable.logMessage(objectName(), QtDebugMsg, QString("Filter graph %1 initialized.").arg(filterName));
    }
    catch(int res)
    {
        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
        ready = false;

        loggable.logAvError(objectName(), QtCriticalMsg,
                        QString("ERROR of the initialization of the filter graph %1.").arg(filterName),
                        res);
    }

    return ready;
}

//---------------------------------------------------------------------------------------
bool FFmpegFilter::isReady() const
{
    return ready;
}

//---------------------------------------------------------------------------------------
int FFmpegFilter::feedGraph(AVFrame *frame)
{
    return av_buffersrc_add_frame_flags(bufferSourceContext, frame, AV_BUFFERSRC_FLAG_KEEP_REF);
}

//---------------------------------------------------------------------------------------
int FFmpegFilter::getOutputFrame(AVFrame **frame)
{
    int result = av_buffersink_get_frame(bufferSinkContext, outFrame);
    *frame = outFrame;
    return result;
}

//---------------------------------------------------------------------------------------
void FFmpegFilter::setNextFilter(FFmpegFilter *filter)
{
    nextFilter = filter;
}

//---------------------------------------------------------------------------------------
FFmpegFilter* FFmpegFilter::getNextFilter() const
{
    return nextFilter;
}

//---------------------------------------------------------------------------------------
