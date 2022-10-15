# Yet Another FFmpeg Player
Simple media player based on Qt framework and FFmpeg/LibAV library.

Allows to play media from:
- file
- UDP multicast

This project focuses on how FFmpeg works with MPEG2-TS transport streams and broadcast-specific video (e.g. interlaced video, or 720x576 resolution with 16 blanking pixels per line, etc.), but also works with regular media files.

## Some screenshots

- Playback of the MPTS from file with recorded MPEG2-TS stream:
![Screenshot 1 - Playback of the MPTS from file with recorded MPEG2-TS stream](https://github.com/antzol/YetAnotherFFmpegPlayer/blob/main/doc/Playing%20MPTS%20stream%20from%20the%20file.png)

## TODO

- Display information about currently playing video and audio streams.
- Show subtitles.
- Show audio level indicator.
- etc...
