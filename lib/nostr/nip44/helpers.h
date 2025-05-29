#pragma once

#include <Arduino.h>
#include <Bitcoin.h>
#include <gmp-ino.h>
#include <bootloader_random.h>
#include <mbedtls/base64.h>
#include <mbedtls/md.h>
#include <mbedtls/chacha20.h>

// Utility functions
void logInfo(const String msg);
void generateRandomIV(uint8_t *iv, int length);
String getTokenAtPosition(String str, String separator, int position);

// Cryptographic functions
String generateSharedSecret(String privateKeyHex, String publicKeyHex);
String reconstructPublicKey(const String &xHex);
bool modular_sqrt(mpz_t result, const mpz_t n, const mpz_t p);

// Validation functions
bool isValidHexKey(const String& input);

// Timing functions
void logTime(const String& message);
