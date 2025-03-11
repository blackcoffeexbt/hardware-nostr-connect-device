#include <Arduino.h>
#include "FS.h"
#include "SPIFFS.h"

#include <esp_adc_cal.h>
#include "Bitcoin.h"
#include "Hash.h"
#include <math.h>
#include <ArduinoJson.h>
#include <NostrEvent.h>
#include <WebSocketsClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TFT_eSPI.h>
#include <OneButton.h>
#include "media/face.h"
#include <qrcode.h>
#include "config.h"
#include "utilities.h"

#include "nostr.h"
#include "nip19.h"
#include "nostr-config.h"

#define PIN_BAT_VOLT 4
#define CONFIG_FILE_PATH "/data.json"

TFT_eSPI tft = TFT_eSPI();

OneButton button1(PIN_BUTTON_1, true);
OneButton button2(PIN_BUTTON_2, true);
OneButton button3(PIN_BUTTON_3, true);
OneButton button4(PIN_BUTTON_4, true);

WebSocketsClient webSocket;
WebSocketsClient webSocketTemp;

unsigned long unixTimestamp;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "uk.pool.ntp.org", 0, 60000);

// lets declare global vars for nostr event data
// define sizes in bytes for memory spaces for global variables
#define EVENT_NOTE_SIZE 2000000
#define EVENT_DOC_SIZE 2000000
#define EVENT_PARAMS_DOC_SIZE 2000000
#define ENCRYPTED_MESSAGE_BIN_SIZE 100000

const char *serialisedJson;
DynamicJsonDocument eventDoc(0);
DynamicJsonDocument eventParamsDoc(0);

struct DocumentData
{
  String tags;
  uint16_t kind;
  String content;
  unsigned long timestamp;

  DocumentData(String t, uint16_t k, String c, unsigned long ts) : tags(t), kind(k), content(c), timestamp(ts) {}
};

String ssidStr;
String passwordStr;
String relayStr;
String nsecStr;
String npubStr;

const char *nsecHex;
const char *npubHex;
const char *ssid;
const char *password;
const char *nsecbunkerRelay;
const char *adminNpubHex = "40321dde7756769c8e509538d328af9712300fe5c36aa5960faaf880202f1a31";
const char *secretKey = "faf68770560f9300346af4393746c7371cfed27bdd5db1155b3f2d358638772c"; // this must be a 64 byte hex

/**
 * @brief Get the Battery Voltage in Volts
 *
 * @return float
 */
float getBatteryVoltage()
{
  esp_adc_cal_characteristics_t adc_chars;
  // Get the internal calibration value of the chip
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  uint32_t raw = analogRead(PIN_BAT_VOLT);
  uint32_t v1 = esp_adc_cal_raw_to_voltage(raw, &adc_chars) * 2; // The partial pressure is one-half
  // get the voltage
  return (v1 / 1000.0);
}

// function to calculate the percentage of battery remaining based on using a 1s lipo batteru
uint8_t getBatteryPercentage()
{
  float voltage = getBatteryVoltage();
  if (voltage > 4.3)
  {
    return 100;
  }
  if (voltage < 3.3)
  {
    return 0;
  }
  return (voltage - 3.3) * 100 / (4.3 - 3.3);
}

void _logToSerialWithTitle(String title, String message)
{
  Serial.println(title + ": " + message);
}

bool isDisplayOff = false;

void turnOffDisplay()
{
  if (isDisplayOff)
  {
    return;
  }
  tft.writecommand(ST7789_DISPOFF); // turn off lcd display
  digitalWrite(38, LOW);            // turn of lcd backlight
  tft.fillScreen(TFT_WHITE);
  isDisplayOff = true;
}

void turnOnDisplay()
{
  if (!isDisplayOff)
  {
    return;
  }
  tft.init();
  tft.writecommand(ST7789_DISPON); // turn on display
  digitalWrite(38, HIGH);          // turn on lcd backlight
  isDisplayOff = false;
}

/**
 * @brief Clean the message by removing newline characters and ACK characters
 *
 * @param message
 */
void cleanMessage(String &message)
{
  message.trim();
  message.replace("\n", ""); // Remove newline characters
  message.replace("\r", ""); // Remove carriage return characters
  // char asciiChars[] = {1, 5, 6, 7, 8, 4, 16, 2, 15, 3};
  // remove any of these characters from the message
  // for (int i = 0; i < sizeof(asciiChars); i++)
  // {
  //   message.replace(String(asciiChars[i]), "");
  // }
  // remove any ascii characters < 32 (non-printable chars)
  for (int i = 0; i < message.length(); i++)
  {
    if (message[i] < 32)
    {
      message.remove(i);
    }
  }
}

void getRandom64ByteHex(char *output)
{
  for (int i = 0; i < 64; i++)
  {
    output[i] = "0123456789abcdef"[esp_random() % 16];
  }
  output[64] = '\0';
}

uint8_t buttonResponse = 0;

void promptUser(String question, OneButton &button1, OneButton &button2, String button1Answer, String button2Answer)
{
  turnOnDisplay();
  buttonResponse = 0;
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_RED, TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(5, 5);
  // define a rectangle to display the question
  tft.println(question);

  tft.setTextSize(2);

  int16_t textWidth = tft.textWidth(button1Answer);
  int16_t textHeight = tft.fontHeight();
  tft.setCursor(0, TFT_WIDTH - textHeight - 10);
  tft.println(button1Answer);

  textWidth = tft.textWidth(button2Answer);
  tft.setCursor(TFT_HEIGHT / 3, TFT_WIDTH - textHeight - 10);
  tft.println(button2Answer);

  button1.attachClick([]()
                      { buttonResponse = 1; });

  button2.attachClick([]()
                      { buttonResponse = 2; });

  while (buttonResponse == 0)
  {
    button1.tick();
    button2.tick();
  }
  tft.fillScreen(TFT_WHITE);
}

void showMessage(String message, String additional)
{
  turnOnDisplay();
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_RED, TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 30);
  tft.println(message);
  // Serial.println(message);
  tft.setCursor(10, 80);
  tft.setTextSize(2);
  tft.println(additional);
  // Serial.println(additional);
}

// check if the client is authorised
bool isClientNpubAuthorised(const char *clientPubKey)
{
  String authorisedClients = getConfigProperty("authorised_clients");
  // Serial.println("Authorised clients: " + authorisedClients);
  if (authorisedClients == "")
  {
    return false;
  }

  // Serial.println("Checking if client is authorised |" + String(clientPubKey) + "|");
  if (authorisedClients.indexOf(clientPubKey) != -1)
  {
    // Serial.println("Client is authorised");
    return true;
  }
  // Serial.println("Client is not authorised");
  return false;
}

bool promptUserShouldAllowClientNpub(String requestingNpub)
{
  String requestingNpubSplit;
  for (int i = 0; i < requestingNpub.length(); i++)
  {
    if (i % 25 == 0 && i != 0)
    {
      requestingNpubSplit += '\n';
    }
    requestingNpubSplit += requestingNpub[i];
  }
  Serial.println("Requesting npub split: " + requestingNpubSplit);
  promptUser(requestingNpubSplit + "\nwants to connect", button1, button2, "Accept", "Reject");
  if (buttonResponse == 2)
  {
    showMessage("Connection rejected", "");
    delay(1000);
    return false;
  }
  else if (buttonResponse == 1)
  {
    showMessage("Connection accepted", "");
    return true;
  }

  return true;
}

/**
 * @brief Add a client to the list of authorised clients
 *
 * @param clientPubKey
 */
void addAuthorisedClientPubKey(const char *clientPubKey)
{
  String authorisedClients = getConfigProperty("authorised_clients");
  // Serial.println("Adding to authorisedClients: " + authorisedClients);

  if (authorisedClients.indexOf(clientPubKey) != -1)
  {
    // Serial.println("Client already authorised");
    return;
  }
  // add the client to the list of authorised clients with a pipe separator
  authorisedClients += clientPubKey;
  authorisedClients += "|";
  setConfigProperty("authorised_clients", authorisedClients.c_str());
}

/**
 * @brief Check if a client requesting to connect, sign or otherwise is authorised
 *
 * @param clientPubKey
 * @param secret
 * @return true
 * @return false
 */
bool checkClientIsAuthorised(const char *clientPubKey, const char *secret)
{
  bool isClientNpubAllowed = isClientNpubAuthorised(clientPubKey);
  if (!isClientNpubAllowed)
  {
    bool isClientAllowed = promptUserShouldAllowClientNpub(clientPubKey);
    if (!isClientAllowed)
    {
      showMessage("Client is not authorised", "Rejected!");
      return false;
    }
    addAuthorisedClientPubKey(clientPubKey);
  }
  else if (!isClientNpubAllowed && (secret != secretKey))
  {
    Serial.println("Secret key does not match. Prompting user.");
    bool isClientAllowed = promptUserShouldAllowClientNpub(clientPubKey);
    if (isClientAllowed)
    {
      addAuthorisedClientPubKey(clientPubKey);
    }
    else
    {
      return false;
    }
  }
  else if (!isClientNpubAllowed && (secret == secretKey))
  {
    Serial.println("Secret key matches. Connecting.");
    addAuthorisedClientPubKey(clientPubKey);
  }
  return true;
}

/**
 * @brief Get the Free Memory Capacity
 *
 * @param message
 * @return size_t
 */
size_t getFreeMemoryCapacity(String message)
{
  // Serial.println("--------------------");
  // Serial.println(message);
  // use hald
  const size_t capacity = esp_get_free_heap_size() / 2;
  // Serial.println("ESP.getMaxAllocHeap : " + String(capacity));
  // Serial.print("Total free memory: ");
  // Serial.println(esp_get_free_heap_size());
  // Serial.println("--------------------");
  return capacity;
}

// handleConnect function with DynamicJsonDocument passed by reference
void handleConnect(DynamicJsonDocument &doc, String &requestingNpub)
{
  // showMessage("Request to connect", "received");

  String requestId = doc["id"];

  String secret = doc["params"][1];
  Serial.println("secret from client is: " + secret);

  if (!checkClientIsAuthorised(requestingNpub.c_str(), secret.c_str()))
  {
    return;
  }

  _logToSerialWithTitle("requestId is: ", requestId);
  _logToSerialWithTitle("requestingNpub is: ", requestingNpub);
  // send ack
  String responseMsg = "{\"id\":\"" + requestId + "\",\"result\":\"ack\"}";
  String dm = nostr::getEncryptedDm(nsecHex, npubHex, requestingNpub.c_str(), 24133, unixTimestamp, responseMsg);

  webSocket.sendTXT(dm);

  // showMessage("Client requested connection", "Acknowledged.");
}

DocumentData parseDocumentData(const String &paramString)
{
  // Calculate available memory, adjust this based on your specific needs
  const size_t freeMemory = ESP.getFreeHeap(); // Example for ESP8266/ESP32
  DeserializationError error = deserializeJson(eventParamsDoc, paramString);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
  }

  // Extracting and preparing the data
  String tags = eventParamsDoc["tags"].as<String>();

  uint16_t kind = eventParamsDoc["kind"];

  String content = eventParamsDoc["content"].as<String>();
  content.trim();
  // Perform content modifications
  content.replace("\\", "\\\\");
  content.replace("\n", "\\n");
  content.replace("\"", "\\\"");
  content.replace(String((char)6), ""); // Remove ACK characters

  unsigned long timestamp = eventParamsDoc["created_at"];

  // Clearing the document is not strictly necessary since it will be destructed
  // paramsDoc.clear();

  // Constructing and returning the DocumentData object
  return DocumentData(tags, kind, content, timestamp);
}

void animateTyping(String message)
{
  turnOnDisplay();
  // display vertical center and 10px left. animate 1 letter at a time
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_RED, TFT_WHITE);
  tft.setTextSize(4);
  int16_t textHeight = tft.fontHeight();
  tft.setCursor(10, (TFT_WIDTH - textHeight) / 2);
  for (int i = 0; i < message.length(); i++)
  {
    tft.print(message[i]);
    delay(100);
  }
}

void showSetupScreen()
{
  tft.fillScreen(TFT_BLACK);
  String message = "Scan to set up signer";
  // show centered at top of screen
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  int16_t textWidth = tft.textWidth(message);
  int16_t textHeight = tft.fontHeight();
  tft.setCursor((TFT_HEIGHT - textWidth) / 2, 10);
  tft.println(message);

  String qrCodeUrl = "https://shop.lnbits.com/user-guides/remote-nostr-signer-set-up-guide";

  uint8_t qrVersion = 4;
  uint8_t qrNumBlocks = 33; // num of blocks wide from table here https://github.com/ricmoo/QRCode
  uint8_t pixelSize = 3;

  // calculate the size of the QR code
  uint8_t qrPixelSize = qrNumBlocks * pixelSize;
  // use this to centre on the display
  uint16_t screenWidth = 320;
  uint16_t screenHeight = 170;
  uint16_t startX = TFT_HEIGHT / 2 - qrPixelSize / 2;
  uint16_t startY = 10 + textHeight + 15;

  // draw a white rectangle to contain the QR code with a 5px white border
  tft.fillRect(startX - 5, startY - 5, qrPixelSize + 10, qrPixelSize + 10, TFT_WHITE);

  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(qrVersion)];
  qrcode_initText(&qrcode, qrcodeData, qrVersion, 0, qrCodeUrl.c_str());
  // draw the QR code
  for (uint8_t y = 0; y < qrcode.size; y++)
  {
    for (uint8_t x = 0; x < qrcode.size; x++)
    {
      if (qrcode_getModule(&qrcode, x, y))
      {
        tft.fillRect(startX + x * pixelSize, startY + y * pixelSize, pixelSize, pixelSize, TFT_BLACK);
      }
    }
  }
  // forever while loop with button ticks
  while (true)
  {
    button1.tick();
    button2.tick();
    button3.tick();
    button4.tick();
    delay(10);
  }
}

void showWelcomeScreen()
{
  animateTyping("PURA VIDA!");
}

void handleSignEvent(DynamicJsonDocument &doc, char const *requestingPubKey)
{
  utilities::startTimer("handleSignEvent");
  // showMessage("Signing request received", "");
  utilities::stopTimer("showMessage");

  String secret = doc["params"][1];
  // Serial.println("secret from client is: " + secret);

  if (!checkClientIsAuthorised(requestingPubKey, secret.c_str()))
  {
    return;
  }

  // Debug this to see why tags arent being parsed
  String docParams = doc["params"][0];
  // Serial.println("docParams is: " + docParams);
  DocumentData docData = parseDocumentData(doc["params"][0].as<const char *>());
  utilities::stopTimer("parseDocumentData");
  // print all docData values
  // _logToSerialWithTitle("tags is: ", docData.tags);
  // _logToSerialWithTitle("kind is: ", String(docData.kind));
  // _logToSerialWithTitle("content is: ", docData.content);
  // _logToSerialWithTitle("timestamp is: ", String(docData.timestamp));

  uint16_t kind = docData.kind;
  unsigned long timestamp = docData.timestamp;
  _logToSerialWithTitle("timestamp is: ", String(timestamp));

  String requestId = doc["id"];
  String signedEvent = nostr::getNote(nsecHex, npubHex, timestamp, docData.content, kind, docData.tags);
  utilities::stopTimer("nostr::getNote");
  // _logToSerialWithTitle("signedEvent is: ", signedEvent);
  // escape the double quotes and slashes
  signedEvent.replace("\\", "\\\\");
  signedEvent.replace("\"", "\\\"");
  int responseMsgCharSize = signedEvent.length() + requestId.length() + 40;
  char *responseMsgChar = (char *)ps_malloc(responseMsgCharSize);
  sprintf(responseMsgChar, "{\"id\":\"%s\",\"result\":\"%s\"}", requestId.c_str(), signedEvent.c_str());
  // _logToSerialWithTitle("responseMsg is: ", responseMsg);
  String dm = nostr::getEncryptedDm(nsecHex, npubHex, requestingPubKey, 24133, timestamp, String(responseMsgChar));
  utilities::stopTimer("nostr::getEncryptedDm");
  free(responseMsgChar);
  // _logToSerialWithTitle("dm is: ", dm);
  webSocket.sendTXT(dm);
  utilities::stopTimer("webSocket.sendTXT");
  // define an array of messages to add to the screen
  const char *messages[] = {"Nice!", "GM", "#coffeechain", "Pura vida", "Zap zap", "Bloomer not Doomer", "MICHAEL SAYLOR KICKED MY DOG."};
  // showMessage("Event signed.", messages[random(0, 7)]);
  utilities::stopTimer("handleSignEvent");
  delay(500);
}

void signerStartedConnectedEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("[WSc] Disconnected!\n");
    showMessage("Disconnected from relay.damus.io relay", "Reconnecting..");
    webSocketTemp.beginSSL("relay.damus.io", 443);
    webSocketTemp.onEvent(signerStartedConnectedEvent);
    break;
  case WStype_CONNECTED:
  {
    showMessage("Connected to relay.damus.io relay", "Sending connected message");
    Serial.printf("[WSc] Connected to %s\n", nsecbunkerRelay);
    String message = "Signer started";
    String dm = nostr::getEncryptedDm(nsecHex, npubHex, npubHex, 4, unixTimestamp, message);
    webSocketTemp.sendTXT(dm);
    break;
  }
  case WStype_TEXT:
  {
    Serial.printf("[WSc] got WS TEXT\n");
    break;
  }
  case WStype_BIN:
    Serial.printf("[WSc] get binary length: %u\n", length);
    break;
  }
}

void sendSignerStartedMsg()
{
  // create a new websocket connectio nto nos.lol and send the dm when connected
  showMessage("Connecting to relay", "Sending connected message");
  webSocketTemp.beginSSL("relay.damus.io", 443);
  webSocketTemp.onEvent(signerStartedConnectedEvent);
  webSocketTemp.setReconnectInterval(5000);
}

/**
 * @brief Respond to a ping request
 *
 * @param doc
 */
void handlePing(DynamicJsonDocument &doc, const char *requestingNpub)
{
  showMessage("Ping received", "Ponging");
  if (!isClientNpubAuthorised(requestingNpub))
  {
    return;
  }
  String requestId = doc["id"];
  String responseMsg = "{\"id\":\"" + requestId + "\",\"result\":\"pong\"}";
  String dm = nostr::getEncryptedDm(nsecHex, npubHex, requestingNpub, 24133, unixTimestamp, responseMsg);
  webSocket.sendTXT(dm);
}

/**
 * @brief Send the relays to the client
 *
 * @param doc
 */
void handleGetRelays(DynamicJsonDocument &doc, const char *requestingNpub)
{
  showMessage("Request for relays received", "Sending relays");
  if (!isClientNpubAuthorised(requestingNpub))
  {
    return;
  }
  String requestId = doc["id"];
  // response should be serialised JSON {<relay_url>: {read: <boolean>, write: <boolean>}}
  String relays = "{\"" + String(nsecbunkerRelay) + "\": {\"read\": true, \"write\": true}}";
  String responseMsg = "{\"id\":\"" + requestId + "\",\"result\":" + relays + "}";
  String dm = nostr::getEncryptedDm(nsecHex, npubHex, requestingNpub, 24133, unixTimestamp, responseMsg);
  webSocket.sendTXT(dm);
}

/**
 * @brief Send the public key to the client
 *
 * @param doc
 */
void handleGetPublicKey(DynamicJsonDocument &doc, const char *requestingNpub)
{
  showMessage("Request for public key received", "Sending public key");
  if (!isClientNpubAuthorised(requestingNpub))
  {
    return;
  }
  String requestId = doc["id"];
  String responseMsg = "{\"id\":\"" + requestId + "\",\"result\":\"" + npubHex + "\"}";
  String dm = nostr::getEncryptedDm(nsecHex, npubHex, requestingNpub, 24133, unixTimestamp, responseMsg);
  webSocket.sendTXT(dm);
}

/**
 * @brief Encrypt some text with NIP04
 *
 * @param doc
 */
void handleNip04Encrypt(DynamicJsonDocument &doc, const char *requestingNpub)
{
  // showMessage("Request to encrypt NIP04", "received");
  if (!isClientNpubAuthorised(requestingNpub))
  {
    return;
  }
  String thirdPartyPubKey = doc["params"][0];
  String plaintext = doc["params"][1];
  String encryptedMessage = nostr::getCipherText(nsecHex, thirdPartyPubKey.c_str(), plaintext);

  // send the encrypted message back to the client
  String requestId = doc["id"];
  String responseMsg = "{\"id\":\"" + requestId + "\",\"result\":\"" + encryptedMessage + "\"}";
  String dm = nostr::getEncryptedDm(nsecHex, npubHex, requestingNpub, 24133, unixTimestamp, responseMsg);
  webSocket.sendTXT(dm);
}

void handleNip04Decrypt(DynamicJsonDocument &doc, const char *requestingNpub)
{
  // showMessage("Request to decrypt NIP04", "received");
  if (!isClientNpubAuthorised(requestingNpub))
  {
    return;
  }
  // message is in params: [<third_party_pubkey>, <nip04_ciphertext_to_decrypt>]
  String thirdPartyPubKey = doc["params"][0];
  String cipherText = doc["params"][1];
  _logToSerialWithTitle("thirdPartyPubKey is: ", thirdPartyPubKey);
  // _logToSerialWithTitle("cipherText is: ", cipherText);

  String decryptedMessage = nostr::decryptNip04Ciphertext(cipherText, nsecHex, thirdPartyPubKey);
  // _logToSerialWithTitle("decryptedMessage is: ", decryptedMessage);
  // now we need to send the decrypted message back to the client
  String requestId = doc["id"];

  // for (int i = 0; i < decryptedMessage.length(); i++)
  // {
  //   // Cast each character to an integer to see its ASCII value
  //   Serial.print(i); // Print the character position
  //   Serial.print(": '");
  //   Serial.print(decryptedMessage[i]); // Print the character itself if it's printable
  //   Serial.print("' (ASCII: ");
  //   Serial.print((int)decryptedMessage[i]); // Print the ASCII value
  //   Serial.println(")");
  // }
  cleanMessage(decryptedMessage);
  decryptedMessage.trim();
  // log decryptedMessage and show any ascii characters
  String responseMsg = "{\"id\":\"" + requestId + "\",\"result\":\"" + decryptedMessage + "\"}";
  String dm = nostr::getEncryptedDm(nsecHex, npubHex, requestingNpub, 24133, unixTimestamp, responseMsg);
  webSocket.sendTXT(dm);
  // showMessage("Decrypted:", decryptedMessage);
}

void handleSigningRequestEvent(uint8_t *data)
{
  utilities::startTimer("handleSigningRequestEvent");
  String requestingPubKey = nostr::getSenderPubKeyHex(String((char *)data));
  // utilities::stopTimer("nostr::getSenderPubKeyHex");

  String message = nostr::nip04Decrypt(nsecHex, String((char *)data));
  // utilities::stopTimer("nostr::nip04Decrypt");
  // _logToSerialWithTitle("Decrypted message is: ", message);
  // _logToSerialWithTitle("message is: ", message);
  DeserializationError error = deserializeJson(eventDoc, message);
  // utilities::stopTimer("deserializeJson");
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
  }

  String method = eventDoc["method"];
  _logToSerialWithTitle("method is: ", method);

  if (method == "connect")
  {
    // print the message
    Serial.println("message is: " + message);
    handleConnect(eventDoc, requestingPubKey);
  }
  else if (method == "sign_event")
  {
    handleSignEvent(eventDoc, requestingPubKey.c_str());
  }
  else if (method == "ping")
  {
    handlePing(eventDoc, requestingPubKey.c_str());
  }
  else if (method == "get_relays")
  {
    handleGetRelays(eventDoc, requestingPubKey.c_str());
  }
  else if (method == "get_public_key")
  {
    handleGetPublicKey(eventDoc, requestingPubKey.c_str());
  }
  else if (method == "nip04_encrypt")
  {
    handleNip04Encrypt(eventDoc, requestingPubKey.c_str());
  }
  else if (method == "nip04_decrypt")
  {
    handleNip04Decrypt(eventDoc, requestingPubKey.c_str());
  }
  else
  {
    Serial.println("default");
    // print the data
    Serial.println("data is: " + String((char *)data));
  }
  utilities::stopTimer("handleSigningRequestEvent");
}

// function to validate whether a string is a valid json string or not
bool isJson(String str)
{
  const size_t capacity = esp_get_free_heap_size() / 2;
  DynamicJsonDocument doc(capacity);
  DeserializationError error = deserializeJson(doc, str);
  if (error)
  {
    return false;
  }
  return true;
}

void handleConfigRequestEvent(uint8_t *data)
{
  String message = nostr::nip04Decrypt(nsecHex, String((char *)data));
  cleanMessage(message);
  // message can be "get_config" or some JSON. if it's json then it's a config update
  if (message == "get_config")
  {
    String configJson = getSerialisedConfigDoc();
    showMessage("Configuration request", "received");
    String configData = nostr::getEncryptedDm(nsecHex, npubHex, adminNpubHex, 24134, unixTimestamp, configJson);
    webSocket.sendTXT(configData);
    showMessage("Configuration sent to relay", "");
  }
  else
  {
    try
    {
      if (!isJson(message))
      {
        showMessage("Error", "Config update rejected. JSON is invalid.");
        return;
      }
      showMessage("Config update", "received");
      updateConfigDoc(message);
      // delay(500);
      showMessage("Config updated", message);
      // delay(500);
      String configJson = getSerialisedConfigDoc();
      String configData = nostr::getEncryptedDm(nsecHex, npubHex, adminNpubHex, 24134, unixTimestamp, configJson);
      Serial.println("Config data is: " + configData);
      webSocket.sendTXT(configData);
      showMessage("Updated config sent", "");
    }
    catch (const std::exception &e)
    {
      Serial.println("Error updating config: " + String(e.what()));
    }
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  // _logToSerialWithTitle("Received data", String((char *)data));
  // if data includes EVENT and 24133 kind, decrypt it
  if (strstr((char *)data, "EVENT") && strstr((char *)data, "24133"))
  {
    handleSigningRequestEvent(data);
  }
  // if data includes EVENT and 4 kind, decrypt it
  else if (strstr((char *)data, "EVENT") && strstr((char *)data, "24134"))
  {
    handleConfigRequestEvent(data);
  }
}

uint8_t socketDisconnectCount = 0;

// connect to web socket server. set up callbacks, on connect, on disconnect, on message
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    ++socketDisconnectCount;
    if (socketDisconnectCount > 3)
    {
      showMessage("Error", "Failed to connect to relay.");
      ESP.restart();
    }
    Serial.printf("[WSc] Disconnected!\n");
    showMessage("Disconnected from relay", "Reconnecting..");
    webSocket.beginSSL(nsecbunkerRelay, 443);
    webSocket.onEvent(webSocketEvent);
    break;
  case WStype_CONNECTED:
  {
    socketDisconnectCount = 0;
    Serial.printf("[WSc] Connected to %s\n", nsecbunkerRelay);
    showMessage("Connected to " + String(nsecbunkerRelay), "Awaiting requests.");
    // watch for 24133 kind events
    String npubHexString(npubHex);
    char *output = new char[64];
    getRandom64ByteHex(output);

    String req = "[\"REQ\", \"" + String(output) + "\",{\"kinds\":[24133],\"#p\":[\"" + npubHexString + "\"],\"limit\":0}";
    req += ",{\"kinds\":[24134],\"#p\":[\"" + npubHexString + "\"],\"authors\":[\"" + String(adminNpubHex) + "\"],\"limit\":0}";
    req += "]";
    Serial.println("req is: " + req);
    webSocket.sendTXT(req);
    // delay(1000);
    turnOffDisplay();
    break;
  }
  case WStype_TEXT:
  {
    Serial.printf("[WSc] Got WS TEXT\n");
    Serial.println("Payload is: " + String((char *)payload));
    handleWebSocketMessage(NULL, payload, length);
    // delay(1000);
    turnOffDisplay();
    break;
  }
  case WStype_BIN:
    Serial.printf("[WSc] get binary length: %u\n", length);
    break;
  }
}

/**
 * @brief Show the connection screen with QR code
 *
 */
void showConnectionScreen()
{
  turnOnDisplay();
  // create conneciton string using npubhex and relay and secretKey in format bunker://683211bd155c7b764e4b99ba263a151d81209be7a566a2bb1971dc1bbd3b715e?relay=wss://relay.nsecbunker.com&secret=faf68770560f9300346af4393746c7371cfed27bdd5db1155b3f2d358638772c
  uint16_t charArraySize = 30 + strlen(npubHex) + strlen(nsecbunkerRelay) + strlen(secretKey);
  char connectionUrl[charArraySize];
  sprintf(connectionUrl, "bunker://%s?relay=wss://%s&secret=%s", npubHex, nsecbunkerRelay, secretKey);
  // sprintf(connectionUrl, "%s", "TEST");
  Serial.println("Connection URL is " + String(connectionUrl));

  uint8_t qrVersion = 8;
  uint8_t qrNumBlocks = 49; // num of blocks wide from table here https://github.com/ricmoo/QRCode
  uint8_t pixelSize = floor(TFT_WIDTH / qrNumBlocks);

  // calculate the size of the QR code
  uint8_t qrPixelSize = qrNumBlocks * pixelSize;
  // use this to centre on the display
  uint16_t screenWidth = 320;
  uint16_t screenHeight = 170;
  uint16_t startX = TFT_HEIGHT / 2 - qrPixelSize / 2;
  uint16_t startY = TFT_WIDTH / 2 - qrPixelSize / 2;

  tft.fillScreen(TFT_WHITE);
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(qrVersion)];
  qrcode_initText(&qrcode, qrcodeData, qrVersion, 0, connectionUrl);
  // draw the QR code
  for (uint8_t y = 0; y < qrcode.size; y++)
  {
    for (uint8_t x = 0; x < qrcode.size; x++)
    {
      if (qrcode_getModule(&qrcode, x, y))
      {
        tft.fillRect(startX + x * pixelSize, startY + y * pixelSize, pixelSize, pixelSize, TFT_BLACK);
      }
    }
  }
}

/**
 * @brief Used for testing hardware buttons
 *
 * @param buttonNumber
 */
void showButtonNumberPressed(int buttonNumber)
{
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(0, TFT_WIDTH - 20);
  tft.println("Button " + String(buttonNumber) + " pressed");
  delay(500);
  tft.fillRect(0, TFT_WIDTH - 20, TFT_HEIGHT, 20, TFT_BLACK);
}

/**
 * @brief Set up screen buttons for the setup screen
 *
 */
void setupSetupScreenButtons()
{
  button1.attachClick([]()
                      { showButtonNumberPressed(1); });
  button2.attachClick([]()
                      { showButtonNumberPressed(2); });
  button3.attachClick([]()
                      { showButtonNumberPressed(3); });
  button4.attachClick([]()
                      { showButtonNumberPressed(4); });
}

void setupButtons()
{
  // short press
  button1.attachClick([]()
                      { 
                        showMessage("Pura vida", "");
                        delay(1000);
                        turnOffDisplay(); });
  button2.attachClick([]()
                      { 
                        showMessage("The everything app of\nfreedom", "");
                        delay(1000);
                        turnOffDisplay(); });
  button3.attachClick([]()
                      {
                        float voltage = getBatteryVoltage();
                        if(voltage > 4.3) {
                          showMessage("Device is charging or powered externally.", "Input voltage: " + String(voltage) + "V");  
                        } else {
                          uint8_t percentage = getBatteryPercentage();
                          showMessage("Battery voltage: " + String(voltage) + "V", "Battery percentage: " + String(percentage) + "%");
                        }
                        delay(1000);
                        turnOffDisplay(); });
  button1.attachLongPressStart([]()
                               { 
                                Serial.println("Button 1 long press start");
                                showConnectionScreen(); });
  // long press stop
  button1.attachLongPressStop([]()
                              {
                                showMessage("Connected to " + String(nsecbunkerRelay), "Awaiting requests.");
                                delay(100);
    turnOffDisplay(); });
}

/**
 * @brief Save JSON data to SPIFFS
 *
 * @param jsonData The JSON string to save
 */
void saveToSPIFFS(String jsonData)
{
  File file = SPIFFS.open(CONFIG_FILE_PATH, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  file.print(jsonData);
  file.close();
  Serial.println("JSON saved to SPIFFS!");
}

/**
 * @brief Get a config value from SPIFFS
 *
 * @param key The key to look up
 * @return String The value or empty string if not found
 */
String getConfigValue(const char *key)
{
  File file = SPIFFS.open(CONFIG_FILE_PATH, FILE_READ);
  if (!file)
  {
    Serial.println("Failed to open config file for reading");
    return "";
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error)
  {
    Serial.println("Failed to parse JSON: " + String(error.c_str()));
    return "";
  }

  if (!doc.containsKey(key))
  {
    Serial.println("Key not found in config");
    return "";
  }

  return doc[key].as<String>();
}

/**
 * @brief Handle configuration mode
 */
void handleConfigMode()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.setTextColor(TFT_PINK, TFT_BLACK);
  tft.println("Setup Mode");
  tft.setCursor(10, 50);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.println("Go to nostrconnect.com");
  tft.setCursor(10, 70);
  tft.println("on a computer to setup");

  Serial.println("Waiting for data from serial");
  while (true)
  {
    if (Serial.available())
    {
      String receivedData = Serial.readStringUntil('\n');

      if (receivedData == "get_config")
      {
        File file = SPIFFS.open(CONFIG_FILE_PATH, FILE_READ);
        if (file)
        {
          Serial.println(file.readString());
          file.close();
        }
      }
      else
      {
        Serial.println("Reading data from serial");
        Serial.println("Received: " + receivedData);
        saveToSPIFFS(receivedData);
      }
    }
    delay(100);
  }
}

bool loadDeviceConfigFromSPIFFS() {
  File file = SPIFFS.open(CONFIG_FILE_PATH, FILE_READ);
  if (!file) {
    Serial.println("Failed to open config file for reading");
    return false;
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.println("Failed to parse JSON: " + String(error.c_str()));
    return false;
  }

  // Store values in String objects to maintain the lifetime of the char data
  ssidStr = doc["ssid"] | "";
  passwordStr = doc["password"] | "";
  relayStr = doc["relay_uri"] | "";
  nsecStr = doc["private_key"] | "";
  npubStr = doc["public_key"] | "";

  // Check if any required fields are missing
  if (ssidStr.isEmpty() || passwordStr.isEmpty() || relayStr.isEmpty() || 
      nsecStr.isEmpty() || npubStr.isEmpty()) {
    Serial.println("Missing required configuration fields");
    return false;
  }

  // Update the global variables
  ssid = ssidStr.c_str();
  password = passwordStr.c_str();
  nsecbunkerRelay = relayStr.c_str();
  nsecHex = nsecStr.c_str();
  npubHex = npubStr.c_str();

  Serial.println("Configuration loaded successfully");
  Serial.println("SSID: " + String(ssid));
  Serial.println("Password: " + String(password));
  Serial.println("Relay: " + String(nsecbunkerRelay));
  Serial.println("Private Key: " + String(nsecHex));
  Serial.println("Public Key: " + String(npubHex));

  return true;
}

void setup()
{
  delay(1000);
  Serial.begin(115200);
  Serial.println("boot");

  // turn on display when on battery
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);

  pinMode(PIN_BUTTON_1, INPUT_PULLUP);

  // set up reserved memory for the json document
  serialisedJson = (char *)ps_malloc(EVENT_NOTE_SIZE);
  eventDoc = DynamicJsonDocument(EVENT_NOTE_SIZE);
  eventParamsDoc = DynamicJsonDocument(EVENT_PARAMS_DOC_SIZE);
  nostr::initMemorySpace(EVENT_NOTE_SIZE, ENCRYPTED_MESSAGE_BIN_SIZE);

  // load screen
  tft.init();
  tft.setRotation(3);
  tft.invertDisplay(true);

  if (!SPIFFS.begin(true))
  {
    Serial.println("An error has occurred while mounting SPIFFS");
    return;
  }

  // Add this debug code
  if (SPIFFS.exists(CONFIG_FILE_PATH)) {
    Serial.println("Config file exists");
    File file = SPIFFS.open(CONFIG_FILE_PATH, "r");
    if (file) {
      Serial.println("Config file contents:");
      Serial.println(file.readString());
      file.close();
    }
  } else {
    Serial.println("Config file does not exist");
  }

  // If no config file or button 1 is pressed or config is empty, enter configuration mode
  if (!SPIFFS.exists(CONFIG_FILE_PATH) || digitalRead(PIN_BUTTON_1) == LOW || getConfigValue("ssid").isEmpty())
  {
    Serial.println("Entering configuration mode");
    handleConfigMode();
    setupSetupScreenButtons();
    return;
  }

  setupButtons();
  showWelcomeScreen();
  delay(1000);

  showMessage("Initialising", "Please wait...");
  if (!SPIFFS.begin(true))
  {
    Serial.println("An error has occurred while mounting SPIFFS");
    return;
  }
  if (!loadDeviceConfigFromSPIFFS()) {
    Serial.println("Failed to load configuration");
    return;
  }

  // connect to wifi
  Serial.println("Connecting to WiFi...");
  // with
  Serial.println("SSID: " + String(ssid));
  Serial.println("Password: " + String(password));
  WiFi.begin(ssid, password);
  WiFi.setAutoReconnect(true);

  showMessage("Connecting to WiFi...", "");
  unsigned long startWifiConnectionAttempt = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
    Serial.print(".");
    if (millis() - startWifiConnectionAttempt > 5000)
    {
      showMessage("Error", "Failed to connect to WiFi.");
      ESP.restart();
      return;
    }
  }

  timeClient.begin();

  // sendSignerStartedMsg();

  showMessage("Connecting to", String(nsecbunkerRelay));
  webSocket.beginSSL(nsecbunkerRelay, 443);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

unsigned long lastPing = 0;
void loop()
{
  webSocket.loop();
  // ping the relay every 10 seconds
  if (millis() - lastPing > 10000)
  {
    lastPing = millis();
    webSocket.sendPing();
  }

  timeClient.update();
  unixTimestamp = timeClient.getEpochTime();

  button1.tick();
  button2.tick();
  button3.tick();
}