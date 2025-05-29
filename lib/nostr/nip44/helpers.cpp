#pragma once

#include "helpers.h"
#include <Arduino.h>
#include <Bitcoin.h>
#include <gmp-ino.h>
#include <bootloader_random.h>
#include <mbedtls/base64.h>
#include <mbedtls/md.h>
#include <mbedtls/chacha20.h>

void logInfo(const String msg) {
    // Serial.println("/log " + msg);
}

void generateRandomIV(uint8_t *iv, int length) {
    for (int i = 0; i < length; i++) {
        iv[i] = random(0, 256);
    }
}

String getTokenAtPosition(String str, String separator, int position) {
  String s = str.substring(0);
  int separatorPos = 0;
  int i = 0;
  while (separatorPos != -1) {
    separatorPos = s.indexOf(separator);
    if (i == position) {
      if (separatorPos == -1) return s;
      return s.substring(0, separatorPos);
    }
    s = s.substring(separatorPos + 1);
    i++;
  }

  return "";
}

String generateSharedSecret(String privateKeyHex, String publicKeyHex) {
  // Reconstruct full public key if only X-coordinate is provided
  if (publicKeyHex.length() == 64) {
    publicKeyHex = reconstructPublicKey(publicKeyHex);
  }

  int byteSize =  32;
  byte privateKeyBytes[byteSize];
  fromHex(privateKeyHex, privateKeyBytes, byteSize);
  PrivateKey privateKey(privateKeyBytes);

  byte sharedSecret[32];

  byte publicKeyBin[64];
  fromHex(publicKeyHex, publicKeyBin, 64);
  PublicKey otherPublicKey(publicKeyBin, true);
  privateKey.ecdh(otherPublicKey, sharedSecret, false);

  return toHex(sharedSecret, sizeof(sharedSecret));
}

String reconstructPublicKey(const String &xHex) {
    mpz_t x, y, rhs, p, a, b;
    mpz_init(x);
    mpz_init(y);
    mpz_init(rhs);
    mpz_init(p);
    mpz_init(a);
    mpz_init(b);

    // Set curve parameters for secp256k1
    mpz_set_str(p, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F", 16);
    mpz_set_ui(a, 0); // a = 0
    mpz_set_ui(b, 7); // b = 7

    // Parse X-coordinate
    mpz_set_str(x, xHex.c_str(), 16);

    // Calculate rhs = x^3 + ax + b mod p
    mpz_powm_ui(rhs, x, 3, p); // rhs = x^3 mod p
    mpz_add(rhs, rhs, b); // rhs = x^3 + b mod p
    mpz_mod(rhs, rhs, p); // Normalize rhs mod p

    // Calculate Y-coordinate
    if (!modular_sqrt(y, rhs, p)) {
        // Handle error: invalid public key
        logInfo("Error: No modular square root exists for the given X-coordinate.");
        mpz_clear(x);
        mpz_clear(y);
        mpz_clear(rhs);
        mpz_clear(p);
        mpz_clear(a);
        mpz_clear(b);
        return "";
    }

    // Ensure even Y (mimic '02' prefix behavior)
    if (mpz_odd_p(y)) {
        mpz_sub(y, p, y);
    }

    // Convert X and Y back to hex strings
    char xHexStr[65], yHexStr[65];
    mpz_get_str(xHexStr, 16, x);
    mpz_get_str(yHexStr, 16, y);

    // Cleanup
    mpz_clear(x);
    mpz_clear(y);
    mpz_clear(rhs);
    mpz_clear(p);
    mpz_clear(a);
    mpz_clear(b);

    // Return full 128-character public key
    return String(xHexStr) + String(yHexStr);
}

bool modular_sqrt(mpz_t result, const mpz_t n, const mpz_t p) {
    mpz_t legendre, exponent, temp;
    mpz_init(legendre);
    mpz_init(exponent);
    mpz_init(temp);

    // Legendre symbol: (n/p) = n^((p-1)/2) mod p
    mpz_sub_ui(temp, p, 1); // temp = p - 1
    mpz_fdiv_q_ui(exponent, temp, 2); // exponent = (p - 1) / 2
    mpz_powm(legendre, n, exponent, p); // legendre = n^((p-1)/2) mod p

    if (mpz_cmp_ui(legendre, 1) != 0) {
        // No modular square root exists
        mpz_clear(legendre);
        mpz_clear(exponent);
        mpz_clear(temp);
        return false;
    }

    // Direct calculation for p ≡ 3 (mod 4)
    mpz_add_ui(temp, p, 1); // temp = p + 1
    mpz_fdiv_q_ui(temp, temp, 4); // temp = (p + 1) / 4
    mpz_powm(result, n, temp, p); // result = n^((p+1)/4) mod p

    // Cleanup
    mpz_clear(legendre);
    mpz_clear(exponent);
    mpz_clear(temp);
    return true;
}

// Function to check if a string is 64 characters long and contains only lowercase hex characters
bool isValidHexKey(const String& input) {
    String trimmed = input;
    trimmed.trim();

    // Check if the length is 64
    if (trimmed.length() != 64) {
        return false;
    }

    // Check if all characters are valid lowercase hex
    for (size_t i = 0; i < trimmed.length(); i++) {
        char c = trimmed.charAt(i);
        if (!(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'f')) {
            return false;
        }
    }

    return true;
}

// function to log time between calls to this function in the serial console
unsigned long lastLogTime = 0;
void logTime(const String& message) {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - lastLogTime;
    lastLogTime = currentTime;

    Serial.printf("%s: %lu ms since last log\n", message.c_str(), elapsedTime);
}