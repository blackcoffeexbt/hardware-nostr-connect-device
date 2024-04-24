#include "nip19.h"

namespace nip19
{
    // Function to convert hex string to byte array
    void hexStringToByteArray(const char *hexString, uint8_t *bytesArray)
    {
        for (size_t i = 0; hexString[i] && hexString[i + 1]; i += 2)
        {
            sscanf(&hexString[i], "%2hhx", &bytesArray[i / 2]);
        }
    }

    void convertTo5bitArray(const uint8_t *byteArray, size_t byteArrayLen, uint8_t *bit5Array, size_t *bit5ArrayLen)
    {
        *bit5ArrayLen = 0;
        int bitPos = 0;
        uint32_t buffer = 0;

        for (size_t i = 0; i < byteArrayLen; i++)
        {
            buffer = (buffer << 8) | byteArray[i];
            bitPos += 8;

            while (bitPos >= 5)
            {
                bitPos -= 5;
                bit5Array[(*bit5ArrayLen)++] = (buffer >> bitPos) & 0x1F;
            }
        }

        // Handle remaining bits in buffer if any
        if (bitPos > 0)
        {
            bit5Array[(*bit5ArrayLen)++] = (buffer << (5 - bitPos)) & 0x1F;
        }
    }

    // Function to convert 5-bit array back to byte array
    void convert5bitArrayToByteArray(const uint8_t *data5bit, size_t data5bitLen, uint8_t *byteArray, size_t *byteArrayLen)
    {
        *byteArrayLen = 0;
        int buffer = 0; // Use an int to ensure there's enough space for bit manipulation.
        int bitPos = 0; // Tracks the current bit position in buffer.

        for (size_t i = 0; i < data5bitLen; ++i)
        {
            // Shift the buffer to the left by 5 bits and add the current 5-bit group.
            buffer = (buffer << 5) | (data5bit[i] & 0x1F); // Ensure only the lower 5 bits are added.
            bitPos += 5;                                   // We've added 5 bits to the buffer.

            // While we have at least 8 bits in the buffer, extract each byte.
            while (bitPos >= 8)
            {
                bitPos -= 8; // Decrease bitPos by 8 as we're extracting a byte.
                // Extract the byte from the buffer, leaving any remaining bits in place.
                // Shift right by the remaining bit count to get the next byte to the rightmost position.
                byteArray[(*byteArrayLen)++] = (buffer >> bitPos) & 0xFF;
            }
        }
    }

    // Function to convert byte array to hex string
    void byteArrayToHexString(const uint8_t *byteArray, size_t byteArrayLen, char *hexString)
    {
        for (size_t i = 0; i < byteArrayLen; ++i)
        {
            sprintf(hexString + (i * 2), "%02x", byteArray[i]);
        }
    }

    const char *encodeHexToBech32(const char *nsecHex, const char *hrp)
    {
        static char output[1024]; // Static storage for the Bech32 encoded string
        size_t byteArrayLen = strlen(nsecHex) / 2;
        uint8_t byteArray[byteArrayLen];

        hexStringToByteArray(nsecHex, byteArray);

        uint8_t bit5Array[byteArrayLen * 8 / 5 + 1]; // Max possible size needed
        size_t bit5ArrayLen = 0;

        convertTo5bitArray(byteArray, byteArrayLen, bit5Array, &bit5ArrayLen);

        bech32_encode(output, hrp, bit5Array, bit5ArrayLen);

        return output;
    }

    const char *decodeBech32ToHexString(const char *input, const char *expected_hrp)
    {
        static char hexString[2048]; // Static storage for the return value
        char hrp[1024];
        uint8_t data[1024];
        size_t data_len;

        if (bech32_decode(hrp, data, &data_len, input))
        {

            if (strcmp(hrp, expected_hrp) == 0)
            {
                uint8_t byteArray[1024];
                size_t byteArrayLen;
                convert5bitArrayToByteArray(data, data_len, byteArray, &byteArrayLen);

                byteArrayToHexString(byteArray, byteArrayLen, hexString);
            }
            else
            {
                Serial.println("Error: HRP does not match expected value.");
                strcpy(hexString, "Error: HRP mismatch");
            }
        }
        else
        {
            Serial.println("Bech32 decoding failed.");
            strcpy(hexString, "Error: Decoding failed");
        }

        return hexString;
    }
}