#include "nostr.h"

namespace nostr
{

    DynamicJsonDocument nostrEventDoc(0);
    byte *encryptedMessageBin;

    unsigned long timer = 0;
    void _startTimer(const char *timedEvent)
    {
        timer = millis();
        // Serial.print("Starting timer for ");
        // Serial.println(timedEvent);
    }

    void _stopTimer(const char *timedEvent)
    {
        unsigned long elapsedTime = millis() - timer;
        Serial.print(elapsedTime);
        Serial.print(" ms - ");
        Serial.println(timedEvent);
        timer = millis();
    }

    void initMemorySpace(size_t nostrEventDocCapacity, size_t encryptedMessageBinSize)
    {
        nostrEventDoc = DynamicJsonDocument(nostrEventDocCapacity);
        encryptedMessageBin = (byte *)ps_malloc(encryptedMessageBinSize);
    }

    void _logToSerialWithTitle(String title, String message)
    {
        Serial.println(title + ": " + message);
    }

    void _logOkWithHeapSize(const char *message)
    {
        Serial.print(message);
        Serial.print(" OK.");
        Serial.print(" Free heap size: ");
        Serial.println(esp_get_free_heap_size());
    }

    String decryptData(byte key[32], byte iv[16], byte *encryptedMessageBin, int byteSize)
    {
        if (!encryptedMessageBin)
        {
            Serial.println("Invalid encryptedMessageBin");
            return ""; // Handle invalid input
        }

        // The encryptedMessageBin is already a byte pointer to the binary data
        // and byteSize is the size of this binary data

        AES_ctx ctx;
        AES_init_ctx_iv(&ctx, key, iv);
        AES_CBC_decrypt_buffer(&ctx, encryptedMessageBin, byteSize); // Use byteSize directly

        // //_logOkWithHeapSize("Decrypted AES data");

        // Convert the decrypted data to a String. Ensure that the data is null-terminated if treating as a C-string.
        // If the decrypted data may not be null-terminated or might contain null bytes within,
        // you'll need a different strategy to create the String object correctly.
        String decryptedData;
        for (int i = 0; i < byteSize; i++)
        {
            if (encryptedMessageBin[i] != '\0') // Assuming the decrypted text does not include null characters
                decryptedData += (char)encryptedMessageBin[i];
            else
                break; // Stop if a null character is found
        }

        // Note: No need to free encryptedMessageBin here if it's managed outside this function

        return decryptedData;
    }

    String decryptNip04Ciphertext(String &cipherText, String privateKeyHex, String senderPubKeyHex)
    {
        _startTimer("decryptNip04Ciphertext");
        String encryptedMessage = cipherText.substring(0, cipherText.indexOf("?iv="));
        // //_logOkWithHeapSize("Got encryptedMessage");
        int encryptedMessageSize = (encryptedMessage.length() * 3) / 4;
        fromBase64(encryptedMessage, encryptedMessageBin, encryptedMessageSize); // Assuming fromBase64 modifies encryptedMessageSize to actual decoded size

        String iv = cipherText.substring(cipherText.indexOf("?iv=") + 4);
        // calculate the ivSize for ivBin based on the iv size
        int ivSize = (iv.length() * 3) / 4;
        byte ivBin[ivSize];
        fromBase64(iv, ivBin, ivSize);
        _logToSerialWithTitle("iv", iv);
        _stopTimer("decryptNip04Ciphertext: Got ivBin");

        int byteSize = 32;
        byte privateKeyBytes[byteSize];
        fromHex(privateKeyHex, privateKeyBytes, byteSize);
        PrivateKey privateKey(privateKeyBytes);
        _stopTimer("decryptNip04Ciphertext: Got privateKey");

        _logToSerialWithTitle("senderPubKeyHex", senderPubKeyHex);
        byte senderPublicKeyBin[64];
        fromHex("02" + String(senderPubKeyHex), senderPublicKeyBin, 64);
        PublicKey senderPublicKey(senderPublicKeyBin);
        _stopTimer("decryptNip04Ciphertext: Got senderPublicKey");
        _logToSerialWithTitle("senderPublicKey.toString() is", senderPublicKey.toString());

        byte sharedPointX[32];
        privateKey.ecdh(senderPublicKey, sharedPointX, false);
        String sharedPointXHex = toHex(sharedPointX, sizeof(sharedPointX));
        _logToSerialWithTitle("sharedPointXHex is", sharedPointXHex);
        _stopTimer("decryptNip04Ciphertext: Got sharedPointX");

        String message = decryptData(sharedPointX, ivBin, encryptedMessageBin, encryptedMessageSize);
        message.trim();
        _stopTimer("decryptNip04Ciphertext: Got message");

        _logToSerialWithTitle("message", message);

        return message;
    }

    String getContent(const String &serialisedJson)
    {
        DeserializationError error = deserializeJson(nostrEventDoc, serialisedJson);
        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
        }
        return nostrEventDoc[2]["content"];
    }

    String getSenderPubKeyHex(const String &serialisedJson)
    {
        DeserializationError error = deserializeJson(nostrEventDoc, serialisedJson);
        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
        }
        return nostrEventDoc[2]["pubkey"];
    }

    std::pair<String, String> getPubKeyAndContent(const String &serialisedJson)
    {
        DeserializationError error = deserializeJson(nostrEventDoc, serialisedJson);
        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            return std::make_pair("", "");
        }

        // Extract the sender public key and content from the document
        String senderPubKeyHex = nostrEventDoc[2]["pubkey"];
        String content = nostrEventDoc[2]["content"];

        // Return both values as a pair
        return std::make_pair(senderPubKeyHex, content);
    }

    // define a function that returns senderPubKeyHex and content

    String nip04Decrypt(const char *privateKeyHex, String serialisedJson)
    {
        _startTimer("nip04Decrypt: nip04Decrypt");
        auto result = getPubKeyAndContent(serialisedJson);
        String senderPubKeyHex = result.first;
        String content = result.second;
        _stopTimer("nip04Decrypt: Got result from getPubKeyAndContent");

        int ivIndex = content.indexOf("?iv=");
        if (ivIndex == -1)
        {
            Serial.println("IV not found in content");
            return "";
        }

        // Extracting encryptedMessage directly from content without using String for the intermediate steps
        int encryptedMessageLength = ivIndex; // Length of the encrypted message part
        // char *encryptedMessage = (char *)ps_malloc(encryptedMessageLength + 1); // +1 for null-terminator
        const char *encryptedMessage = content.c_str(); // Use the content directly
        if (!encryptedMessage)
        {
            Serial.println("Failed to allocate PSRAM for encryptedMessage");
            return "";
        }

        int encryptedMessageSize = encryptedMessageLength / 4 * 3; // Base64 encoded size is 4/3 of the binary size

        fromBase64(encryptedMessage, encryptedMessageBin, encryptedMessageSize);
        _stopTimer("nip04Decrypt: Got encryptedMessageBin");

        _logToSerialWithTitle("encryptedMessage", encryptedMessage);

        int byteSize = 32;
        byte privateKeyBytes[byteSize];
        fromHex(privateKeyHex, privateKeyBytes, byteSize);
        PrivateKey privateKey(privateKeyBytes);
        _stopTimer("nip04Decrypt: Got privateKey");

        _logToSerialWithTitle("senderPubKeyHex", senderPubKeyHex);
        byte senderPublicKeyBin[64];
        fromHex("02" + String(senderPubKeyHex), senderPublicKeyBin, 64);
        PublicKey senderPublicKey(senderPublicKeyBin);
        _stopTimer("nip04Decrypt: Got senderPublicKey");
        _logToSerialWithTitle("senderPublicKey.toString() is", senderPublicKey.toString());

        return decryptNip04Ciphertext(content, privateKeyHex, senderPubKeyHex);
    }

    /**
     * @brief Get a Note object
     *
     * @param privateKeyHex
     * @param pubKeyHex
     * @param timestamp
     * @param content
     * @param kind
     * @param tags
     * @return String
     */
    String getNote(char const *privateKeyHex, char const *pubKeyHex, unsigned long timestamp, String &content, uint16_t kind, String tags)
    {
        _startTimer("getNote");
        // convert
        // log timestamp
        _logToSerialWithTitle("timestamp is: ", String(timestamp));
        String message = "[0,\"" + String(pubKeyHex) + "\"," + String(timestamp) + "," + String(kind) + "," + tags + ",\"" + content + "\"]";
        _logToSerialWithTitle("message is: ", message);

        // sha256 of message converted to hex, assign to msghash
        byte hash[64] = {0}; // hash
        int hashLen = 0;

        // Get the sha256 hash of the message
        hashLen = sha256(message, hash);
        _stopTimer("get sha256 hash of message");
        String msgHash = toHex(hash, hashLen);
        _stopTimer("get msgHash as hex");
        _logToSerialWithTitle("SHA-256: ", msgHash);

        // Create the private key object
        int byteSize = 32;
        byte privateKeyBytes[byteSize];
        fromHex(privateKeyHex, privateKeyBytes, byteSize);
        _stopTimer("convert privateKeyHex to byte array");
        PrivateKey privateKey(privateKeyBytes);
        _stopTimer("create privateKey object");

        // Generate the schnorr sig of the messageHash
        byte messageBytes[byteSize];
        fromHex(msgHash, messageBytes, byteSize);
        _stopTimer("convert msgHash to byte array");
        SchnorrSignature signature = privateKey.schnorr_sign(messageBytes);
        _stopTimer("generate schnorr sig");
        String signatureHex = String(signature);
        _logToSerialWithTitle("Schnorr sig is: ", signatureHex);

        // Device the public key and verify the schnorr sig is valid
        PublicKey pub = privateKey.publicKey();
        _stopTimer("get public key");

        // if (pub.schnorr_verify(signature, messageBytes))
        // {
        //     _logToSerialWithTitle("All good, signature is valid", "");
        // }
        // else
        // {
        //     _logToSerialWithTitle("Something went wrong, signature is invalid", "");
        // }
        _stopTimer("verify schnorr sig");

        String serialisedDataString = "{\"id\":\"" + msgHash + "\",\"pubkey\":\"" + String(pubKeyHex) + "\",\"created_at\":" + String(timestamp) + ",\"kind\":" + String(kind) + ",\"tags\":" + tags + ",\"content\":\"" + content + "\",\"sig\":\"" + signatureHex + "\"}";
        _logToSerialWithTitle("serialisedEventDataString is: ", String(serialisedDataString));

        // Print the JSON to the serial monitor
        _logToSerialWithTitle("Event JSON", serialisedDataString);
        return serialisedDataString;
    }

    /**
     * @brief Convert a string to a byte array
     *
     * @param input
     * @param padding_diff
     * @param output
     */
    void stringToByteArray(const char *input, int padding_diff, byte *output)
    {
        int i = 0;
        // remove end-of-string char
        while (input[i] != '\0')
        {
            output[i] = input[i];
            i++;
        }

        // pad between 1 and 16 bytes
        for (int j = 0; j < padding_diff; j++)
        {
            output[i + j] = padding_diff;
        }
    }

    /**
     * @brief encrypt data using AES-256-CBC
     *
     * @param key
     * @param iv
     * @param msg
     * @return String
     */
    String encryptData(byte key[32], byte iv[16], String &msg)
    {
        // message has to be padded at the end so it is a multiple of 16
        int padding_diff = msg.length() % 16 == 0 ? 16 : 16 - (msg.length() % 16);

        int byteSize = msg.length() + padding_diff;
        byte *messageBin = (byte *)ps_malloc(byteSize); // Allocate memory in PSRAM

        if (messageBin == nullptr)
        {
            Serial.println("Failed to allocate PSRAM");
            return "";
        }

        stringToByteArray(msg.c_str(), padding_diff, messageBin);

        AES_ctx ctx;
        AES_init_ctx_iv(&ctx, key, iv);

        AES_CBC_encrypt_buffer(&ctx, messageBin, byteSize);

        return toHex(messageBin, byteSize);
    }

    /**
     * @brief Get the cipher text for a nip4 message
     *
     * @param privateKeyHex
     * @param recipientPubKeyHex
     * @param content
     * @return String
     */
    String getCipherText(const char *privateKeyHex, const char *recipientPubKeyHex, String &content)
    {
        _startTimer("getCipherText");
        // Get shared point
        // Create the private key object
        int byteSize = 32;
        byte privateKeyBytes[byteSize];
        fromHex(privateKeyHex, privateKeyBytes, byteSize);
        PrivateKey privateKey(privateKeyBytes);
        _stopTimer("getCipherText: create privateKey object");

        byte publicKeyBin[64];
        fromHex("02" + String(recipientPubKeyHex), publicKeyBin, 64);
        PublicKey otherDhPublicKey(publicKeyBin);
        _logToSerialWithTitle("otherDhPublicKey.toString() is", otherDhPublicKey.toString());
        _stopTimer("getCipherText: create otherDhPublicKey object");

        byte sharedPointX[32];
        privateKey.ecdh(otherDhPublicKey, sharedPointX, false);
        String sharedPointXHex = toHex(sharedPointX, sizeof(sharedPointX));
        _logToSerialWithTitle("sharedPointXHex is", sharedPointXHex);
        _stopTimer("getCipherText: get sharedPointX");

        // Create the initialization vector
        uint8_t iv[16];
        esp_random() % 256;
        for (int i = 0; i < sizeof(iv); i++)
        {
            iv[i] = esp_random() % 256;
        }
        _stopTimer("getCipherText: create iv");

        String ivHex = toHex(iv, 16);
        String ivBase64 = hexToBase64(ivHex);
        _stopTimer("getCipherText: get ivBase64");

        String encryptedMessageHex = encryptData(sharedPointX, iv, content);
        _stopTimer("getCipherText: get encryptedMessageHex");

        // divide the length of the hex string 2 to get the size of the byte array, since each byte consists of 2 hexadecimal characters.
        int encryptedMessageSize = encryptedMessageHex.length() / 2;
        // Allocate memory in PSRAM for the encrypted message array
        uint8_t *encryptedMessage = (uint8_t *)ps_malloc(encryptedMessageSize);
        if (encryptedMessage == nullptr)
        {
            Serial.println("Failed to allocate PSRAM for encryptedMessage");
        }
        fromHex(encryptedMessageHex, encryptedMessage, encryptedMessageSize);
        _stopTimer("getCipherText: get encryptedMessage fromHex");

        String encryptedMessageBase64 = hexToBase64(encryptedMessageHex);
        _stopTimer("getCipherText: get encryptedMessageBase64");

        encryptedMessageBase64 += "?iv=" + ivBase64;
        return encryptedMessageBase64;
    }

    /**
     * @brief Get the Serialised Encrypted Dm Object
     *
     * @param pubKeyHex
     * @param recipientPubKeyHex
     * @param kind
     * @param msgHash
     * @param timestamp
     * @param encryptedMessageWithIv
     * @param schnorrSig
     * @return String
     */
    String getSerialisedEncryptedDmObject(const char *pubKeyHex, const char *recipientPubKeyHex, uint16_t kind, String &msgHash, int timestamp, String &encryptedMessageWithIv, String &schnorrSig)
    {
        // parse a JSON array
        String serialisedTagsArray = "[[\"p\",\"" + String(recipientPubKeyHex) + "\"]]";
        // _logToSerialWithTitle("serialisedTagsArray is: ", serialisedTagsArray);

        // create the serialised fullEvent string using sprintf instead of the Arduino JSON library
        return "[\"EVENT\",{\"id\":\"" + msgHash + "\",\"pubkey\":\"" + pubKeyHex + "\",\"created_at\":" + String(timestamp) + ",\"kind\":" + String(kind) + ",\"tags\":" + serialisedTagsArray + ",\"content\":\"" + encryptedMessageWithIv + "\",\"sig\":\"" + schnorrSig + "\"}]";
    }

    String getSerialisedEncryptedDmArray(char const *pubKeyHex, char const *recipientPubKeyHex, uint16_t kind, int timestamp, String &encryptedMessageWithIv)
    {
        String serialisedTagsArray = "[[\"p\",\"" + String(recipientPubKeyHex) + "\"]]";
        String message = "[0,\"" + String(pubKeyHex) + "\"," + String(timestamp) + "," + String(kind) + "," + serialisedTagsArray + ",\"" + encryptedMessageWithIv + "\"]";

        // _logToSerialWithTitle("message is: ", message);

        return message;
    }

    /**
     * @brief Get the Encrypted Dm object
     *
     * @param privateKeyHex
     * @param pubKeyHex
     * @param recipientPubKeyHex
     * @param kind
     * @param timestamp
     * @param content
     * @return String
     */
    String getEncryptedDm(char const *privateKeyHex, char const *pubKeyHex, char const *recipientPubKeyHex, uint16_t kind, unsigned long timestamp, String content)
    {
        _startTimer("getEncryptedDm");
        String encryptedMessageBase64 = getCipherText(privateKeyHex, recipientPubKeyHex, content);
        _stopTimer("get cipher text");

        String message = nostr::getSerialisedEncryptedDmArray(pubKeyHex, recipientPubKeyHex, kind, timestamp, encryptedMessageBase64);
        _stopTimer("get serialised encrypted dm array");

        byte hash[64] = {0}; // hash
        int hashLen = 0;

        // Get the sha256 hash of the message
        hashLen = sha256(message, hash);
        String msgHash = toHex(hash, hashLen);
        _logToSerialWithTitle("SHA-256:", msgHash);
        _stopTimer("get sha256 hash of message");

        int byteSize = 32;
        byte privateKeyBytes[byteSize];
        fromHex(privateKeyHex, privateKeyBytes, byteSize);
        _stopTimer("get privateKeyBytes from hex");
        PrivateKey privateKey(privateKeyBytes);
        _stopTimer("create privateKey object");
        // Generate the schnorr sig of the messageHash
        SchnorrSignature signature = privateKey.schnorr_sign(hash);
        _stopTimer("generate schnorr sig");
        String signatureHex = String(signature);
        _logToSerialWithTitle("Schnorr sig is: ", signatureHex);

        String serialisedEventData = nostr::getSerialisedEncryptedDmObject(pubKeyHex, recipientPubKeyHex, kind, msgHash, timestamp, encryptedMessageBase64, signatureHex);
        _stopTimer("get serialised encrypted dm object");
        // _logToSerialWithTitle("serialisedEventData is", serialisedEventData);
        return serialisedEventData;
    }
}