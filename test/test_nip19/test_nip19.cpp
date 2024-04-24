#include <Arduino.h>
#include <unity.h>
#include <WebSocketsClient.h>
#include "Bitcoin.h"
#include "nostr.h"
#include "nip19.h"

void test_bech32_decode_nsec(void)
{
    const char *input = "nsec1k6wr7wzqsrxxh5ukqww9aesen29a0kqjtjsulg8p8cmvddgrzqrq96vewm";
    const char *output = nip19::decodeBech32ToHexString(input, "nsec");

    String expected = "b69c3f384080cc6bd396039c5ee6199a8bd7d8125ca1cfa0e13e36c6b5031006";

    TEST_ASSERT_EQUAL_STRING(expected.c_str(), output);
    
}

void test_bech32_decode_npub(void)
{
    const char *input = "npub1uzudejrqazxjurdxslfvsqvp8vpuwlfaslzt79jyz6kvt8xj27mqc00jrp"; // Replace with your actual Bech32 string
    const char *output = nip19::decodeBech32ToHexString(input, "npub");

    String expected = "e0b8dcc860e88d2e0da687d2c801813b03c77d3d87c4bf164416acc59cd257b6";

    TEST_ASSERT_EQUAL_STRING(expected.c_str(), output);
    
}

void test_bech32_encode_nsec(void)
{
    const char *expectedBech32Output = "nsec1k6wr7wzqsrxxh5ukqww9aesen29a0kqjtjsulg8p8cmvddgrzqrq96vewm";
    const char *nsecHex = "b69c3f384080cc6bd396039c5ee6199a8bd7d8125ca1cfa0e13e36c6b5031006";

    const char *encodedNsec = nip19::encodeHexToBech32(nsecHex, "nsec");
    
    TEST_ASSERT_EQUAL_STRING(expectedBech32Output, encodedNsec);
}

void test_bech32_encode_npub(void)
{
    const char *expectedBech32Output = "npub1ytxeugjc7yv3kc43grz2eal80vltkzjdgla372gs6c5u5a2wkxqqz565zm";
    const char *nsecHex = "22cd9e2258f1191b62b140c4acf7e77b3ebb0a4d47fb1f2910d629ca754eb180";

    const char *encodedNsec = nip19::encodeHexToBech32(nsecHex, "npub");
    
    TEST_ASSERT_EQUAL_STRING(expectedBech32Output, encodedNsec);
}

void setup()
{
    delay(500);
    Serial.begin(115200);

    UNITY_BEGIN();
    RUN_TEST(test_bech32_encode_nsec);
    RUN_TEST(test_bech32_encode_npub);
    RUN_TEST(test_bech32_decode_nsec);
    RUN_TEST(test_bech32_decode_npub);
    UNITY_END();
}

void loop()
{
    // Not used in this example.
}
