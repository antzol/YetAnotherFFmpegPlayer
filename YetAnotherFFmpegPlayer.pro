QT       += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = yaffplayer

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ui \
    logger \
    player \
    audio

SOURCES += \
    audio/audiolevelcalculator.cpp \
    audio/audiolevelmeter.cpp \
    audio/audiolevelwidget.cpp \
    audio/averagelevelcalculator.cpp \
    audio/loudnesscalculator.cpp \
    logger/loggable.cpp \
    logger/logger.cpp \
    main.cpp \
    mainwindow.cpp \
    player/audiodecoder.cpp \
    player/audioframe.cpp \
    player/decoder.cpp \
    player/demuxer.cpp \
    player/ffmpegfilter.cpp \
    player/frame.cpp \
    player/videodecoder.cpp \
    player/videoframe.cpp \
    ui/detailsdockwidget.cpp \
    ui/openstreamdialog.cpp

HEADERS += \
    audio/averagelevelcalculator.h \
    audio/concretesamplesextractor.h \
    audio/loudnesscalculator.h \
    audio/samplesextractor.h \
    audio/audiolevelwidget.h \
    audio/audiolevelcalculator.h \
    audio/audiolevelmeter.h \
    logger/loggable.h \
    logger/logger.h \
    mainwindow.h \
    player/audiodecoder.h \
    player/audioframe.h \
    player/decoder.h \
    player/demuxer.h \
    player/ffmpegfilter.h \
    player/frame.h \
    player/utils.h \
    player/videodecoder.h \
    player/videoframe.h \
    ui/detailsdockwidget.h \
    ui/openstreamdialog.h

FORMS += \
    mainwindow.ui \
    ui/openstreamdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += $$PWD/ffmpeg-5.1.2-full_build-shared/include
LIBS += $$PWD/ffmpeg-5.1.2-full_build-shared/lib/avcodec.lib \
        $$PWD/ffmpeg-5.1.2-full_build-shared/lib/avdevice.lib \
        $$PWD/ffmpeg-5.1.2-full_build-shared/lib/avfilter.lib \
        $$PWD/ffmpeg-5.1.2-full_build-shared/lib/avformat.lib \
        $$PWD/ffmpeg-5.1.2-full_build-shared/lib/avutil.lib \
        $$PWD/ffmpeg-5.1.2-full_build-shared/lib/postproc.lib \
        $$PWD/ffmpeg-5.1.2-full_build-shared/lib/swresample.lib \
        $$PWD/ffmpeg-5.1.2-full_build-shared/lib/swscale.lib

LIBS += -L$$PWD/ffmpeg-5.1.2-full_build-shared/lib
LIBS += -L$$PWD/ffmpeg-5.1.2-full_build-shared/bin
LIBS += -lavcodec \
        -lavformat \
        -lavutil

RESOURCES += \
    images.qrc

