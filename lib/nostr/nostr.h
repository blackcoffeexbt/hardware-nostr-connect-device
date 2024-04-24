#ifndef NOSTR_H
#define NOSTR_H

#include "Bitcoin.h"
#include "Hash.h"
#include <math.h>
#include <iostream>
#include <base64.h>
#include <aes.h>
#include <ArduinoJson.h>

namespace nostr
{
    void initMemorySpace(size_t nostrEventDocCapacity, size_t encryptedMessageBinSize);

    void _logToSerialWithTitle(String title, String message);

    void stringToByteArray(const char *input, int padding_diff, byte *output);

    String getContent(const String &serialisedJson);

    String getSenderPubKeyHex(const String &serialisedJson);

    std::pair<String, String> getPubKeyAndContent(const String &serialisedJson);

    String nip04Decrypt(const char *privateKeyHex, String serialisedJson);

    String decryptData(byte key[32], byte iv[16], byte* encryptedMessageBin, int byteSize);

    String decryptNip04Ciphertext(String &cipherText, String privateKeyHex, String senderPubKeyHex);

    String getNote(char const *privateKeyHex, char const *pubKeyHex, unsigned long timestamp, String &content, uint16_t kind, String tags = "[]");

    String encryptData(byte key[32], byte iv[16], String &msg);

    String getCipherText(const char *privateKeyHex, const char *recipientPubKeyHex, String &content);

    String getSerialisedEncryptedDmObject(const char *pubKeyHex, const char *recipientPubKeyHex, uint16_t kind, String &msgHash, int timestamp, String &encryptedMessageWithIv, String &schnorrSig);

    String getSerialisedEncryptedDmArray(char const *pubKeyHex, char const *recipientPubKeyHex, uint16_t kind, int timestamp, String &encryptedMessageWithIv);

    String getEncryptedDm(char const *privateKeyHex, char const *pubKeyHex, char const *recipientPubKeyHex, uint16_t kind, unsigned long timestamp, String content);


}
#endif