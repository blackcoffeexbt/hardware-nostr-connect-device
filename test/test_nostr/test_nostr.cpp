#include <Arduino.h>
#include <unity.h>
#include <WebSocketsClient.h>
#include "Bitcoin.h"
#include "nostr.h"
#include "nip19.h"

#define EVENT_NOTE_SIZE 2000000
#define EVENT_DOC_SIZE 2000000
#define EVENT_PARAMS_DOC_SIZE 2000000
#define ENCRYPTED_MESSAGE_BIN_SIZE 100000

const char *nsecHex = "b69c3f384080cc6bd396039c5ee6199a8bd7d8125ca1cfa0e13e36c6b5031006";
const char *npubHex = "e0b8dcc860e88d2e0da687d2c801813b03c77d3d87c4bf164416acc59cd257b6";

// function to get lipmsum like content of a specified length, include spaces at word type lengths
String randomContent(int length)
{
    String content = "";
    String words[] = {"Lorem", "ipsum", "dolor", "sit", "amet", "consectetur", "adipiscing", "elit", "sed", "do", "eiusmod", "tempor", "incididunt", "ut", "labore", "et", "dolore", "magna", "aliqua", "Ut", "enim", "ad", "minim", "veniam", "quis", "nostrud", "exercitation", "ullamco", "laboris", "nisi", "ut", "aliquip", "ex", "ea", "commodo", "consequat", "Duis", "aute", "irure", "dolor", "in", "reprehenderit", "in", "voluptate", "velit", "esse", "cillum", "dolore", "eu", "fugiat", "nulla", "pariatur", "Excepteur", "sint", "occaecat", "cupidatat", "non", "proident", "sunt", "in", "culpa", "qui", "officia", "deserunt", "mollit", "anim", "id", "est", "laborum"};
    int wordCount = sizeof(words) / sizeof(words[0]);
    int wordLength = 0;
    while (content.length() < length)
    {
        if (wordLength == 0)
        {
            wordLength = random(1, 10);
        }
        if (content.length() + wordLength > length)
        {
            wordLength = length - content.length();
        }
        content += words[random(0, wordCount)];
        content += " ";
        wordLength = 0;
    }
    return content;
}

void test_create_nostr_note(void)
{
    // create a nostr::getNote with a private key, public key, timestamp, content, kind, and tags
    String content = "Hello, World!";
    unsigned long timestamp = 1711876747;
    String note = nostr::getNote(nsecHex, npubHex, timestamp, content, 1, "[]");
    std::string expected = "{\"id\":\"1d3b098b56955db8d8870d6040230640652d88aa8daf3d953dc791b94235e1a6\",\"pubkey\":\"e0b8dcc860e88d2e0da687d2c801813b03c77d3d87c4bf164416acc59cd257b6\",\"created_at\":1711876747,\"kind\":1,\"tags\":[],\"content\":\"Hello, World!\",\"sig\":\"6a0696d1700b67ec1ff18c0c808133693f4e755424d1900b3558e166cad1c1dfb86311385c5710684462913c34a71ccd00025b9b81d6a07e6000e2e37d0f7ae0\"}";
    TEST_ASSERT_EQUAL_STRING(expected.c_str(), note.c_str());
}

void test_create_nostr_long_note(void)
{
    uint32_t length = 5000;
    String content = randomContent(length);
    unsigned long timestamp = 1711876747;
    String note = nostr::getNote(nsecHex, npubHex, timestamp, content, 1, "[]");
    TEST_ASSERT_EQUAL_STRING("", "");
}

void test_create_nostr_encrypted_dm(void)
{
    String content = "Hello, World!";
    unsigned long timestamp = 1711876747;
    String encryptedDm = nostr::getEncryptedDm(nsecHex, npubHex, npubHex, 1, timestamp, content);
    TEST_ASSERT_EQUAL_STRING("", "");
}

void test_create_nostr_encrypted_dm_long(void)
{
    uint32_t length = 5000;
    String content = randomContent(length);
    unsigned long timestamp = 1711876747;
    String encryptedDm = nostr::getEncryptedDm(nsecHex, npubHex, npubHex, 1, timestamp, content);
    TEST_ASSERT_EQUAL_STRING("", "");
}

void test_decrypt_nip04(void)
{
    nostr::initMemorySpace(EVENT_NOTE_SIZE, ENCRYPTED_MESSAGE_BIN_SIZE);
    const char *nsecHex = ""; // insert nsec hex here
    String payload = "[\"EVENT\",\"e81e31a089bf821912000021f7ca00d8240e8750c4d0a4051175d06892d477b1\",{\"content\":\"zAQZYjMcNMJXi1Io3fzw3zNRJXAqBkXsP9CNd6orEXwzxQ1yfZFYK7DWQtKosg7JgKsGsQuSovEb1FyRLFLMs4OgglZTV8umR28+QQEBPNLX2RlvCsCdc7mRBIHtQNk6dh6UQroNPYlbJcsmriGkPiutWtHLtaIy2UOW6ih8cLYX+zvoPxU4jMcNsUrdMoaEcKI9jrHNRvg+iSB/Al+Mc8cMlLy4LWp0uTMNOVVeXxHgwT/nC1GW7xXzCXlBo9hDR6Pu+ubTk651FAJVjhY4AQ==?iv=4UuwMtp8XHWSpIbPzOo8gg==\",\"created_at\":1712820220,\"id\":\"c0efee8409d000d0285f6869c6a96e5aeeb2a4276b90fc84867adb7a16c23db7\",\"kind\":24133,\"pubkey\":\"b1efdd301df76b5aaa16467ef4a0863385ba907e72350beb101fd77da2e558c1\",\"sig\":\"f10fc8cd64099809f4ca353fe7ba423d1bb02ccf23481c5ae87929a872bb02c4138c23fdc081e1b4aa8662ec52a39f4b04ad637fe9a69ae2bbb2682ea4c0a65e\",\"tags\":[[\"p\",\"683211bd155c7b764e4b99ba263a151d81209be7a566a2bb1971dc1bbd3b715e\"]]}]";
    String decrypted = nostr::nip04Decrypt(nsecHex, payload);
    Serial.println("Decrypted: " + decrypted);
    TEST_ASSERT_EQUAL_STRING("", "");
}

void setup()
{
    delay(500);
    Serial.begin(115200);

    UNITY_BEGIN();
    // RUN_TEST(test_create_nostr_note);
    // RUN_TEST(test_create_nostr_long_note);
    // RUN_TEST(test_create_nostr_encrypted_dm);
    // RUN_TEST(test_create_nostr_encrypted_dm_long);
    RUN_TEST(test_decrypt_nip04);
    UNITY_END();
}

void loop()
{
    // Not used in this example.
}
