#ifndef NIP19_H
#define NIP19_H

#include <Arduino.h>
#include "Bitcoin.h"

namespace nip19
{
    void hexStringToByteArray(const char *hexString, uint8_t *bytesArray);

    void convertTo5bitArray(const uint8_t *byteArray, size_t byteArrayLen, uint8_t *bit5Array, size_t *bit5ArrayLen);

    void convert5bitArrayToByteArray(const uint8_t *data5bit, size_t data5bitLen, uint8_t *byteArray, size_t *byteArrayLen);

    void byteArrayToHexString(const uint8_t *byteArray, size_t byteArrayLen, char *hexString);

    const char *decodeBech32ToHexString(const char *input, const char *expected_hrp);

    const char *encodeHexToBech32(const char *nsecHex, const char *hrp);
}

#endif