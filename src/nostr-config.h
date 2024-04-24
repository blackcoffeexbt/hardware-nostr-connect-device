#pragma once
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <FS.h>

// we want to 2kb static json doc for saving device configuration
#define CONFIG_JSON_SIZE 2048
// define config file name
#define CONFIG_FILE "/config.json"

StaticJsonDocument<CONFIG_JSON_SIZE> configDoc;

/**
 * @brief Get the Config Doc object
 * 
 * @return String 
 */
String getSerialisedConfigDoc() {
  String output;
  serializeJson(configDoc, output);
  Serial.println("Serialised Config: " + output);
  return output;
}

/**
 * @brief Save the configuration to SPIFFS
 * 
 */
void saveConfigToSPIFFS() {
  fs::File configFile = SPIFFS.open(CONFIG_FILE, "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return;
  }

  Serial.println("Saving config to SPIFFS: " + getSerialisedConfigDoc());
  serializeJson(configDoc, configFile);
  configFile.close();
}

// update configDoc using a json string
void updateConfigDoc(String json) {
  DeserializationError error = deserializeJson(configDoc, json);
  if (error) {
    Serial.println("Failed to read file, using default configuration");
    return;
  }
  saveConfigToSPIFFS();
}

/**
 * @brief Load the configuration from SPIFFS
 * 
 */
void loadConfigFromSPIFFS() {
  configDoc.clear();
  fs::File configFile = SPIFFS.open(CONFIG_FILE, "r");
  if (!configFile) {
    Serial.println("Failed to open config file for reading");
    return;
  }

  DeserializationError error = deserializeJson(configDoc, configFile);
  if (error) {
    Serial.println("Failed to read file, using default configuration");
    return;
  }

  // serialise the doc and serial.print it
  String output;
  serializeJson(configDoc, output);
  Serial.println("Config file is: " + output);
}

/**
 * @brief Set a Config Property
 * 
 * @param key 
 * @param value 
 */
void setConfigProperty(const char *key, const char *value) {
  Serial.println("Setting config property: " + String(key) + " to " + String(value));
  configDoc[key] = value;

  // save the config to SPIFFS
  saveConfigToSPIFFS();
  loadConfigFromSPIFFS();
}

/**
 * @brief Get a Config Property
 * 
 * @param key 
 * @return const char* 
 */
String getConfigProperty(const char *key) {
  return configDoc[key];
}

void testConfig() {
  setConfigProperty("ssid", "myssid");
  setConfigProperty("password", "mypassword");
  // get and print the values
    Serial.println(getConfigProperty("ssid"));
    Serial.println(getConfigProperty("password"));
}