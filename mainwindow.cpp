#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <QFileDialog>


#include "openstreamdialog.h"

#include "utils.h"

//---------------------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Yet Another FFmpeg Player");

    createMenu();
    createMainUiLayout();
    createDetailsWidget();
    createSettingsWidget();

    connect(detailsButton, &QPushButton::clicked, detailsDockWidget, &DetailsDockWidget::setVisible);
    connect(detailsDockWidget, &DetailsDockWidget::visibilityChanged, detailsButton, &QPushButton::setChecked);

    demuxer = new Demuxer();
    demuxer->setVideoSink(videoWidget->videoSink());

    connect(this, &MainWindow::configureAndStartPlayer, demuxer, &Demuxer::setSourceAndStart, Qt::QueuedConnection);

    connect(playAction, &QAction::triggered, demuxer, &Demuxer::play, Qt::QueuedConnection);
    connect(pauseAction, &QAction::triggered, demuxer, &Demuxer::pause, Qt::QueuedConnection);
    connect(stopAction, &QAction::triggered, demuxer, &Demuxer::stop, Qt::QueuedConnection);
    connect(demuxer, &Demuxer::playbackStateChanged, this, &MainWindow::processPlayerStateChange);

    connect(demuxer, &Demuxer::streamsFound, this, &MainWindow::updateStreamLists, Qt::QueuedConnection);
    connect(demuxer, &Demuxer::programsFound, this, &MainWindow::updateProgramList, Qt::QueuedConnection);

    connect(this, &MainWindow::selectedStreamChanged, demuxer, &Demuxer::changeSelectedStream, Qt::QueuedConnection);

    demuxer->moveToThread(&demuxThread);
    connect(&demuxThread, &QThread::finished, demuxer, &QObject::deleteLater);
    demuxThread.start();
}

//---------------------------------------------------------------------------------------
MainWindow::~MainWindow()
{
    demuxer->stop();
    demuxThread.quit();
    demuxThread.wait();

    delete ui;
}

//---------------------------------------------------------------------------------------
void MainWindow::openMediaFile()
{
    /// TODO: use recently used directory

    QString filePath = QFileDialog::getOpenFileName(this, "Open media file", "E:/tmp/media");

    if (!filePath.isEmpty())
    {
        QFileInfo info{filePath};
        setWindowTitle(QString("File: '%1'").arg(info.fileName()));

        loggable.logMessage(objectName(), QtDebugMsg, QString("Select file: %1").arg(filePath));
        openMedia(filePath, Demuxer::SourceType::File);
    }
}

//---------------------------------------------------------------------------------------
void MainWindow::openMediaStream()
{
    OpenStreamDialog dlg;

    if (dlg.exec() == QDialog::Accepted)
    {
        QString uri = dlg.getStreamFullUri();
        if (uri.isEmpty())
            return;

        setWindowTitle(QString("Stream: %1").arg(uri.split('?').at(0)));

        loggable.logMessage(objectName(), QtDebugMsg,
                            QString("Select stream (with params): '%1'").arg(uri));

        demuxer->setRwTimeout(dlg.getRwTimeout());
        openMedia(uri, Demuxer::SourceType::Stream);
    }
}

//---------------------------------------------------------------------------------------
void MainWindow::processStartPauseButtonClick()
{

}

//---------------------------------------------------------------------------------------
//   Fill stream comboboxes with values found by the demuxer.
//   Items in comboboxes is sorted by PID (for MPEG-TS - stream indexes provided by
// the input format context of the FFmpeg) or, if PID equeals 0, by stream id (also provided
// by the input format context).
//   UserRole data of the items is stream id.
//   DisplayRole data (text) of the item is:
// "PID value (if it is not equals 0) - language (if it's presented) - title (if it's presented)"
//   or, if all elements are not presented:
// "stream id value"
void MainWindow::updateStreamLists(const std::vector<std::shared_ptr<StreamInfo>> &streams)
{
    videoStreamsComboBox->clear();
    audioStreamsComboBox->clear();

    if (streams.empty())
        return;

    // key - stream id (if not 0), else stream index;
    // value:
    // - first - stream index;
    // - second - string for the representation in the combobox.
    std::map<int, std::pair<int, std::string>> videoStreams;
    std::map<int, std::pair<int, std::string>> audioStreams;

    for (int idx = 0; idx < streams.size(); ++idx)
    {
        std::shared_ptr<StreamInfo> streamInfo = streams[idx];
        std::string txt = streamInfo->id ? std::to_string(streamInfo->id) : "";
        auto it = streamInfo->properties.find(AVStrings::Language);
        if (it != streamInfo->properties.end())
        {
            if (!txt.empty())
                txt.append(" - ");
            txt.append(it->second);
        }

        it = streamInfo->properties.find(AVStrings::Title);
        if (it != streamInfo->properties.end())
        {
            if (!txt.empty())
                txt.append(" - ");
            txt.append(it->second);
        }

        if (txt.empty())
            txt = std::to_string(streamInfo->index);

        int key = streamInfo->id ? streamInfo->id : streamInfo->index;
        std::pair<int, std::string> val{idx, txt};
        switch(streamInfo->type)
        {
        case AVMEDIA_TYPE_VIDEO: videoStreams[key] = val; break;
        case AVMEDIA_TYPE_AUDIO: audioStreams[key] = val; break;
        default:
            break;
        }
    }

    for (auto& [key, val] : videoStreams)
        videoStreamsComboBox->addItem(QString::fromStdString(val.second), val.first);

    for (auto& [key, val] : audioStreams)
        audioStreamsComboBox->addItem(QString::fromStdString(val.second), val.first);

}

//---------------------------------------------------------------------------------------
//   Fill combobox with values found by the demuxer.
//   Items in the combobox is sorted by program id.
//   UserRole data of the items is program id.
//   DisplayRole data (text) of the item is:
// "program id value - service name (if it's presented)"
void MainWindow::updateProgramList(const std::map<int, std::shared_ptr<ProgramInfo>> &programs)
{
    programsComboBox->clear();

    if (programs.empty())
        return;

    for (auto& [id, programInfo] : programs)
    {
        std::string txt = std::to_string(id);
        auto it = programInfo->properties.find(AVStrings::ServiceName);
        if (it != programInfo->properties.end())
            txt += " - " + it->second;
        programsComboBox->addItem(QString::fromStdString(txt), id);
    }
}

//---------------------------------------------------------------------------------------
void MainWindow::processProgramChange()
{
    bool ok;
    int programId = programsComboBox->currentData().toInt(&ok);

    if (!ok)
        return;

    int streamIndex = demuxer->getFirstProgramStreamByType(programId, AVMEDIA_TYPE_VIDEO);
    int itemIndex = (streamIndex != -1) ? videoStreamsComboBox->findData(streamIndex) : -1;
    videoStreamsComboBox->setCurrentIndex(itemIndex);

    streamIndex = demuxer->getFirstProgramStreamByType(programId, AVMEDIA_TYPE_AUDIO);
    itemIndex = (streamIndex != -1) ? audioStreamsComboBox->findData(streamIndex) : -1;
    audioStreamsComboBox->setCurrentIndex(itemIndex);
}

//---------------------------------------------------------------------------------------
void MainWindow::processStreamChange(int index)
{
    QComboBox *cb = qobject_cast<QComboBox*>(sender());

    if (!cb)
        return;

    auto it = comboBoxToMediaTypeMap.find(cb);
    if (it == comboBoxToMediaTypeMap.end())
        return;

    AVMediaType type = it->second;
    bool ok = false;
    int streamIndex = cb->itemData(index).toInt(&ok);

    emit selectedStreamChanged(type, ok ? streamIndex : -1);
}

//---------------------------------------------------------------------------------------
void MainWindow::processPlayerStateChange(QMediaPlayer::PlaybackState state)
{
    switch (state)
    {
    case QMediaPlayer::PlayingState:
        playPauseButton->removeAction(playAction);
        playPauseButton->setDefaultAction(pauseAction);
        stopAction->setEnabled(true);
        break;
    case QMediaPlayer::PausedState:
        playPauseButton->removeAction(pauseAction);
        playPauseButton->setDefaultAction(playAction);
        stopAction->setEnabled(true);
        break;
    case QMediaPlayer::StoppedState:
        playPauseButton->removeAction(pauseAction);
        playPauseButton->setDefaultAction(playAction);
        stopAction->setEnabled(false);
        break;
    }
}

//---------------------------------------------------------------------------------------
void MainWindow::createMenu()
{
    openFileAction = new QAction(tr("Open file..."), this);
    openStreamAction = new QAction(tr("Open stream.."), this);

    mediaMenu = menuBar()->addMenu(tr("Media"));
    mediaMenu->addAction(openFileAction);
    mediaMenu->addAction(openStreamAction);

    connect(openFileAction, &QAction::triggered, this, &MainWindow::openMediaFile);
    connect(openStreamAction, &QAction::triggered, this, &MainWindow::openMediaStream);
}

//---------------------------------------------------------------------------------------
void MainWindow::createMainUiLayout()
{
    videoWidget = new QVideoWidget();

    programsComboBox = new QComboBox();
    videoStreamsComboBox = new QComboBox();
    audioStreamsComboBox = new QComboBox();

    detailsButton = new QPushButton(tr("Details ❯❯"));
    settingsButton = new QPushButton(tr("Settings ❯❯"));

    detailsButton->setCheckable(true);
    detailsButton->setEnabled(false);
    settingsButton->setEnabled(false);

    QGridLayout *ctrlLayout = new QGridLayout();

    ctrlLayout->addWidget(new QLabel(tr("Service:")), 0, 0);
    ctrlLayout->addWidget(programsComboBox, 0, 1);

    ctrlLayout->addWidget(new QLabel(tr("Video:")), 0, 2);
    ctrlLayout->addWidget(videoStreamsComboBox, 0, 3);

    ctrlLayout->addWidget(new QLabel(tr("Audio:")), 1, 2);
    ctrlLayout->addWidget(audioStreamsComboBox, 1, 3);

    ctrlLayout->addWidget(detailsButton, 0, 4);
    ctrlLayout->addWidget(settingsButton, 1, 4);

    ctrlLayout->setColumnStretch(1, 1);
    ctrlLayout->setColumnStretch(3, 1);

    videoWidget = new QVideoWidget(this);

    playAction = new QAction(QIcon(":/icons/play"), tr("Play"), this);
    pauseAction = new QAction(QIcon(":/icons/pause"), tr("Pause"), this);
    stopAction = new QAction(QIcon(":/icons/stop"), tr("Stop"), this);
    stopAction->setEnabled(false);

    playPauseButton = new QToolButton();
    stopButton = new QToolButton();

    QSize ctrlButtonSize{32, 32};
    playPauseButton->setMinimumSize(ctrlButtonSize);
    stopButton->setMinimumSize(ctrlButtonSize);

    playPauseButton->setDefaultAction(playAction);
    stopButton->setDefaultAction(stopAction);

    QHBoxLayout *btnLayout = new QHBoxLayout();

    btnLayout->addStretch(1);
    btnLayout->addWidget(playPauseButton);
    btnLayout->addWidget(stopButton);
    btnLayout->addStretch(1);


    QVBoxLayout *vertLayout = new QVBoxLayout();
    vertLayout->addLayout(ctrlLayout);
    vertLayout->addWidget(videoWidget, 1);
    vertLayout->addLayout(btnLayout);

    QWidget *wgt = new QWidget(this);
    wgt->setLayout(vertLayout);

    setCentralWidget(wgt);

    comboBoxToMediaTypeMap[videoStreamsComboBox] = AVMEDIA_TYPE_VIDEO;
    comboBoxToMediaTypeMap[audioStreamsComboBox] = AVMEDIA_TYPE_AUDIO;

    connect(videoStreamsComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::processStreamChange);
    connect(audioStreamsComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::processStreamChange);
    connect(programsComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::processProgramChange);
}

//---------------------------------------------------------------------------------------
void MainWindow::createDetailsWidget()
{
    detailsDockWidget = new DetailsDockWidget(this);
    detailsDockWidget->setVisible(false);
    addDockWidget(Qt::RightDockWidgetArea, detailsDockWidget);
}

//---------------------------------------------------------------------------------------
void MainWindow::createSettingsWidget()
{

}

//---------------------------------------------------------------------------------------
void MainWindow::openMedia(const QString &uri, Demuxer::SourceType type)
{
    emit configureAndStartPlayer(uri, type);
}

//---------------------------------------------------------------------------------------

