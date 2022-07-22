#pragma once

#include <stdint.h>

class Lin
{
public:
    enum Errors {
        ReadSyncFail = -1,
        ReadSyncWrongValue = -2,
        ReadIdFail = -3,
        ReadIdWrongValue = -4,
        ReadTimeout = -5,
        ReadWrongChecksum = -6
    };

    Lin();
    void setup();

    void writeFrame(uint8_t id, const uint8_t *data, int dataLen);
    int receiveFrame(uint8_t id, uint8_t *data, int dataLen);
private:
    void writeSyncBreak();
    uint8_t dataChecksum(const uint8_t* message, uint8_t messageLen, uint8_t addr);
    uint8_t addrParity(uint8_t addr);
    int readByte(int64_t timeout);
};