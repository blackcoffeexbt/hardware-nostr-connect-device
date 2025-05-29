#pragma once

#include <Arduino.h>
#include <vector>

// NIP-44 constant salt
extern const uint8_t NIP44_SALT[8];

// Helper functions
uint32_t _math_int_log2(uint32_t x);
size_t calcPaddedLen(size_t unpadded_len);
std::vector<uint8_t> padMessage(const String &plaintext);
String unpadMessage(const std::vector<uint8_t> &padded);

// Cryptographic functions
void hkdf_sha256(const uint8_t *salt, size_t salt_len,
                 const uint8_t *ikm, size_t ikm_len,
                 const uint8_t *info, size_t info_len,
                 uint8_t *okm, size_t okm_len);

bool getMessageKeys(const uint8_t *conversation_key, const uint8_t *nonce,
                   uint8_t *chacha_key, uint8_t *chacha_nonce, uint8_t *hmac_key);

void hmac_aad(const uint8_t *key, size_t key_len,
              const uint8_t *message, size_t msg_len,
              const uint8_t *aad, size_t aad_len,
              uint8_t *output);

bool verifyBase64(const std::vector<uint8_t>& input, String& output);
String base64_encode(const uint8_t* input, size_t length);

// Main encryption/decryption functions
String encryptMessageNip44(const String &plaintext, const String &sharedSecretHex);
String decryptMessageNip44(const String &payload, const String &sharedSecretHex);

// HMAC-SHA256 implementation
void hmac_sha256(const uint8_t* key, size_t key_len,
                 const uint8_t* msg, size_t msg_len,
                 uint8_t* hmac);

// High-level execution functions
String executeEncryptMessageNip44(String data, String privateKeyHex, String thirdPartyPublicKeyHex);
String executeDecryptMessageNip44(String data, String privateKeyHex, String thirdPartyPublicKeyHex);