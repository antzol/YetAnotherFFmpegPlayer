#include "utils.h"

QString mapAVActiveFormatDescriptionToString(AVActiveFormatDescription afd)
{
    QString descr;
    QString templ{"%1 = %2"};
    switch (afd)
    {
    case AV_AFD_SAME:
        descr = "Active format is the same as the coded frame";
        break;
    case AV_AFD_4_3:
        descr = "4:3 (centre)";
        break;
    case AV_AFD_16_9:
        descr = "16:9 (centre)";
        break;
    case AV_AFD_14_9:
        descr = "14:9 (centre)";
        break;
    case AV_AFD_4_3_SP_14_9:
        descr = "4:3 (with shoot and protect 14:9 centre)";
        break;
    case AV_AFD_16_9_SP_14_9:
        descr = "16:9 (with shoot and protect 14:9 centre)";
        break;
    case AV_AFD_SP_4_3:
        descr = "16:9 (with shoot and protect 4:4 centre)";
        break;
    }
    return templ.arg(afd, 4, 2, QChar('0')).arg(descr);
}
