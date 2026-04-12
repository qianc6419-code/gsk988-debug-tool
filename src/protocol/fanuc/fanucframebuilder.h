#ifndef FANUCFRAMEBUILDER_H
#define FANUCFRAMEBUILDER_H

#include <QByteArray>
#include <QVector>

class FanucFrameBuilder
{
public:
    // Constants
    static constexpr quint8 HEADER_BYTE = 0xA0;
    static const QByteArray FUNC_READ;
    static const QByteArray FUNC_RESPONSE;

    // Frame building
    static QByteArray buildHandshake();
    static QByteArray buildSingleBlockRequest(quint16 ncFlag, quint16 blockFuncCode,
                                               const QByteArray& blockParams);
    static QByteArray buildMultiBlockRequest(const QVector<QByteArray>& blocks);
    static QByteArray buildResponse(const QByteArray& dataPayload);

    // Block building
    static QByteArray buildBlock(quint16 ncFlag, quint16 blockFuncCode,
                                  const QByteArray& params20);
    static QByteArray buildReadPMCBlock(char addrType, int startAddr, int endAddr,
                                         int pmcTypeNum, int dataType);
    static QByteArray buildWritePMCBlockByte(char addrType, int startAddr, int endAddr,
                                              int pmcTypeNum, quint8 value);
    static QByteArray buildWritePMCBlockWord(char addrType, int startAddr, int endAddr,
                                              int pmcTypeNum, qint16 value);
    static QByteArray buildWritePMCBlockLong(char addrType, int startAddr, int endAddr,
                                              int pmcTypeNum, qint32 value);
    static QByteArray buildWritePMCBlockFloat(char addrType, int startAddr, int endAddr,
                                               int pmcTypeNum, float value);
    static QByteArray buildReadMacroBlock(int addr);
    static QByteArray buildWriteMacroBlock(int addr, int numerator, quint8 precision);
    static QByteArray buildReadParamBlock(int paramNo);
    static QByteArray buildPositionBlock(quint8 posType);

    // Frame parsing
    static QByteArray extractFrame(QByteArray& buffer);
    static bool validateHeader(const QByteArray& frame);
    static quint16 getDataLength(const QByteArray& frame);

    // Data encoding (big-endian)
    static QByteArray encodeInt32(qint32 value);
    static qint32 decodeInt32(const QByteArray& data, int offset = 0);
    static QByteArray encodeInt16(qint16 value);
    static qint16 decodeInt16(const QByteArray& data, int offset = 0);
    static double calcValue(const QByteArray& data, int offset = 0);

    // PMC helpers
    static quint8 pmcAddrTypeToNum(char addrType);
    static char pmcNumToAddrType(quint8 num);
};

#endif // FANUCFRAMEBUILDER_H
