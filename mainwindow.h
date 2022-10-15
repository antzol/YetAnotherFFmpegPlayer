#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QAction>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QToolButton>

#include <QThread>

#include <QVideoWidget>
#include <QVideoSink>

#include "detailsdockwidget.h"
#include "loggable.h"

#include "demuxer.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void openMediaFile();
    void openMediaStream();

    void processStartPauseButtonClick();

    void updateStreamLists(const std::unordered_map<int, std::shared_ptr<StreamInfo>> &streams);
    void updateProgramList(const std::map<int, std::shared_ptr<ProgramInfo>> &programs);

    void processProgramChange();
    void processStreamChange(int index);
    void processPlayerStateChange(QMediaPlayer::PlaybackState state);

signals:
    void configureAndStartPlayer(const QString &path, Demuxer::SourceType type);

    void selectedStreamChanged(AVMediaType type, int streamIndex);

private:
    void createMenu();
    void createMainUiLayout();
    void createDetailsWidget();
    void createSettingsWidget();

    void openMedia(const QString& uri, Demuxer::SourceType type);

    Ui::MainWindow *ui;

    QComboBox *programsComboBox;
    QComboBox *videoStreamsComboBox;
    QComboBox *audioStreamsComboBox;
    std::unordered_map<QComboBox*, AVMediaType> comboBoxToMediaTypeMap;

    QPushButton *detailsButton;
    QPushButton *settingsButton;

    QVideoWidget *videoWidget;

    QToolButton *playPauseButton;
    QToolButton *stopButton;

    DetailsDockWidget *detailsDockWidget;
    QDockWidget *settingsDockWidget;

    QMenu *mediaMenu;

    QAction *openFileAction;
    QAction *openStreamAction;
    QAction *playAction;
    QAction *pauseAction;
    QAction *stopAction;

    QString mediaSourceUri;

    Demuxer *demuxer;
    QThread demuxThread;

    Loggable loggable;
};
#endif // MAINWINDOW_H
