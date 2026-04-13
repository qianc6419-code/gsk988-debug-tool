#ifndef MAZAKFRAMEBUILDER_H
#define MAZAKFRAMEBUILDER_H

#include <QString>
#include <QByteArray>
#include <cstring>

namespace MazakConst {
    constexpr int AXISMAX   = 16;
    constexpr int TOOLMAX   = 20;
    constexpr int ALMMAX    = 24;
    constexpr int ALMHISMAX = 72;
}

namespace MazakError {
    constexpr int OK        = 0;
    constexpr int SOCK      = -10;
    constexpr int HNDL      = -20;
    constexpr int CLIMAX    = -21;
    constexpr int SERVERMAX = -22;
    constexpr int VER       = -30;
    constexpr int BUSY      = -40;
    constexpr int RUNNING   = -50;
    constexpr int OVER      = -51;
    constexpr int NONE      = -52;
    constexpr int TYPE      = -53;
    constexpr int EDIT      = -54;
    constexpr int PROSIZE   = -55;
    constexpr int PRONUM    = -56;
    constexpr int ARG       = -60;
    constexpr int SYS       = -70;
    constexpr int FUNC      = -80;
    constexpr int TIMEOUT   = -90;
}

#pragma pack(push, 1)

struct MAZ_NCTIME {
    quint32 hor;
    quint32 min;
    quint32 sec;
    char    dummy[4];
};

struct MAZ_NCONTIME {
    quint32 year;
    quint32 mon;
    quint32 day;
    quint32 hor;
    quint32 min;
    quint32 sec;
};

struct MAZ_ACCUM_TIME {
    MAZ_NCTIME power_on;
    MAZ_NCTIME auto_ope;
    MAZ_NCTIME auto_cut;
    MAZ_NCTIME total_cut;
    MAZ_NCTIME total_time;
};

struct MAZ_VERINFO {
    quint16 majorver;
    quint16 minorver;
    quint16 releasever;
    quint16 buildver;
};

struct MTC_ONE_RUNNING_INFO {
    quint8  sts;
    char    dummy[7];
    qint16  ncmode;
    qint16  ncsts;
    qint16  tno;
    quint8  suffix;
    quint8  sufattr;
    qint16  figA;
    qint16  figB;
    qint32  nom;
    qint32  gno;
    qint16  sp_override;
    qint16  ax_override;
    qint16  rpid_override;
    qint16  almno;
    qint32  alm_fore_color;
    qint32  alm_back_color;
    qint16  prtcnt;
    qint16  prtcnt_clamp;
    qint32  autotim;
    qint32  cuttim;
    qint32  totaltim;
    char    dummy3[112];
};

struct MTC_RUNNING_INFO {
    MTC_ONE_RUNNING_INFO run_info[4];
    MTC_ONE_RUNNING_INFO dummy[2];
};

struct MTC_ONE_AXIS_INFO {
    quint8  sts;
    quint8  dummy;
    char    axis_name[4];
    char    dummy1[2];
    double  pos;
    double  mc_pos;
    double  load;
    double  feed;
    double  dummy2[17];
};

struct MTC_COMP_FEED_INFO {
    quint8  sts;
    char    dummy[7];
    double  feed;
};

struct MTC_AXIS_INFO {
    MTC_COMP_FEED_INFO  composited_feedrate[4];
    MTC_ONE_AXIS_INFO   axis[16];
    MTC_ONE_AXIS_INFO   reserved[8];
};

struct MTC_ONE_SPINDLE_INFO {
    quint8  sts;
    quint8  type;
    char    dummy[6];
    double  rot;
    double  load;
    double  temp;
    double  dummy2[17];
};

struct MTC_SPINDLE_INFO {
    MTC_ONE_SPINDLE_INFO sp_info[8];
    MTC_ONE_SPINDLE_INFO dummy[4];
};

struct MAZ_NCPOS {
    char    status[16];
    qint32  data[16];
};

struct MAZ_FEED {
    qint32 fmin;
    qint32 frev;
};

struct MAZ_ALARM {
    qint16  eno;
    quint8  sts;
    char    dummy[5];
    char    pa1[32];
    char    pa2[16];
    char    pa3[16];
    quint8  mon;
    quint8  day;
    quint8  hor;
    quint8  min;
    char    dummy2[4];
    char    extmes[32];
    char    head[4];
    char    dummy3[4];
    char    message[64];
};

struct MAZ_ALARMALL {
    MAZ_ALARM alarm[24];
};

struct MAZ_ALARMHIS {
    MAZ_ALARM alahis[72];
};

struct MAZ_PROINFO {
    char    wno[33];
    char    dummy[7];
    char    comment[49];
    char    dummy2[7];
    quint8  type;
    char    dummy3[7];
};

struct MAZ_PROINFO2 {
    char    wno[33];
    char    dummy[7];
    char    comment[49];
    char    dummy2[7];
    quint8  type;
    char    dummy3[7];
    qint32  uno;
    char    dummy4[4];
    qint32  sno;
    char    dummy5[4];
    qint32  bno;
    char    dummy6[4];
};

struct MTC_ONE_PROGRAM_INFO {
    quint8  sts;
    char    dummy[7];
    qint32  wno;
    qint32  subwno;
    qint32  blockno;
    qint32  seqno;
    qint32  unitno;
    char    wno_string[33];
    char    subwno_string[33];
    char    wno_comment[49];
    char    subwno_comment[49];
    char    dummy3[44];
};

struct MTC_PROGRAM_INFO {
    MTC_ONE_PROGRAM_INFO prog_info[4];
    MTC_ONE_PROGRAM_INFO dummy[2];
};

struct MAZ_AXISNAME {
    char status[16];
    char axisname[64];
};

struct MAZ_AXISLOAD {
    char     status[16];
    quint16  data[16];
};

struct MAZ_TOOLINFO {
    quint16 tno;
    quint8  suf;
    quint8  sufatr;
    quint8  name;
    quint8  part;
    quint8  sts;
    char    dummy[1];
    qint32  yob;
    char    dummy2[4];
};

struct MAZ_TDCOMP {
    MAZ_TOOLINFO info;
    quint32 sts;
    qint32  toolsetX;
    qint32  toolsetZ;
    qint32  offsetX;
    qint32  offsetY;
    qint32  offsetZ;
    qint32  lengthA;
    qint32  lengthB;
    qint32  radius;
    qint32  LenOffset;
    qint32  RadOffset;
    qint32  LenCompno;
    qint32  RadCompno;
    char    dummy[4];
};

struct MAZ_TDCOMPALL {
    MAZ_TDCOMP offset[20];
};

struct MAZ_TLIFE {
    MAZ_TOOLINFO info;
    quint32 sts;
    quint32 lif;
    quint32 use;
    quint32 clif;
    quint32 cuse;
    char    dummy[4];
};

struct MAZ_TLIFEALL {
    MAZ_TLIFE toolLife[20];
};

struct MAZ_TOFFCOMP {
    qint32 type;
    qint32 offset1;
    qint32 offset2;
    qint32 offset3;
    qint32 offset4;
    qint32 offset5;
    qint32 offset6;
    qint32 offset7;
    qint32 offset8;
    qint32 offset9;
};

struct MAZ_NC_VERINFO {
    char szMainA[17];    char dummy1[7];
    char szMainB[17];    char dummy2[7];
    char szLadder[17];   char dummy3[7];
    char szUnitName[17]; char dummy4[7];
    char szNCSerial[17]; char dummy5[7];
    char szNCModel[17];  char dummy6[7];
};

struct MAZ_MAINTE {
    char    lpszComment[49];
    char    dummy[7];
    quint16 uYear;
    quint16 uMonth;
    quint16 uDay;
    qint16  nTargetTime;
    qint32  lProgressSec;
};

struct MAZ_LMAINTE {
    char    lpszComment[1568];
    char    dummy[4];
    quint16 uYear;
    quint16 uMonth;
    quint16 uDay;
    qint16  nTargetTime;
    qint32  lProgressSec;
    quint32 flgReversed;
};

struct MAZ_MAINTE_CHECK {
    MAZ_MAINTE  mainte[22];
    MAZ_LMAINTE lmainte[2];
};

#pragma pack(pop)

namespace MazakFrameBuilder {
    QString errorToString(int code);
    QString fixedString(const char* data, int maxSize);
}

#endif // MAZAKFRAMEBUILDER_H
