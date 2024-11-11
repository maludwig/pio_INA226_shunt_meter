#include <time.h>

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include "Arduino.h"
#include <INA.h> // Zanshin INA Library
#include <DS3231RTC.h>
#include <Wire.h>
#include <WireScanner.h>
#include <vector>

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <dirent.h>
#include <stdio.h>

#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>

#include "functions.h"
#include <ESP32Time.h>
#include <SimpleStats.h>

#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "sd_functions.h"

#include "ArduinoJson.h"
#include "site.h"

#include "shunt_version.h"




const uint32_t SERIAL_SPEED{115200}; ///< Use fast serial speed

const uint32_t SHUNT_MICRO_OHM{100}; ///< Shunt resistance in Micro-Ohm, e.g. 100000 is 0.1 Ohm

const uint16_t MAXIMUM_AMPS{1}; ///< Max expected amps, clamped from 1A to a max of 1022A
uint8_t devicesFound{0};        ///< Number of INAs found
INA_Class* ina_a;                  ///< INA class instantiation to use EEPROM
INA_Class* ina_b;                  ///< INA class instantiation to use EEPROM

std::vector<INA_Class*> inaVector;

DS3231RTC rtc;

#define LED_PIN 27

TwoWire* wire_a;
TwoWire* wire_b;
// #define WIRE_A_SDA 21
// #define WIRE_A_SCL 22
#define WIRE_A_SDA 33
#define WIRE_A_SCL 32
#define WIRE_B_SDA 21
#define WIRE_B_SCL 22


// 2.5 uV per LSB
#define SHUNT_VOLTS_PER_LSB 0.0000025

// 1.25 mV per LSB
#define BUS_VOLTS_PER_LSB 0.00125

ESP32Time esp32rtc;
// Replace with your network credentials
const char* ssid = "blah";
const char* password = "blahblah";

// const char* ssid = "Trustpower_2.4GHz_1279";
// const char* password = "Pucucacani";


// Esp32 SoftAP
const char* ap_ssid = "CurrentShunt";
const char* ap_password = "yellowwhitered";

File log_file;

struct ShuntStats {
    SimpleStats busVoltageStats;
    SimpleStats shuntVoltageStats;
    int32_t lastShuntRawVoltage;
    uint32_t lastBusRawVoltage;
};

ShuntStats shuntStatsArray[5];

#define SerialAndLogLn(...) { \
    Serial.println(__VA_ARGS__); \
    log_file.println(__VA_ARGS__); \
    ws.printfAll(__VA_ARGS__); \
}

void initSoftAP() {
    Serial.println("\n[*] Creating AP");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, password);
    Serial.print("[+] AP Created with IP Gateway ");
    Serial.println(WiFi.softAPIP());
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void ina_setup() {

  ina_a = new INA_Class(0, WIRE_A_SDA, WIRE_A_SCL, 0);
  ina_b = new INA_Class(0, WIRE_B_SDA, WIRE_B_SCL, 1);
  inaVector = {ina_a, ina_b};
  // inaVector = {ina_a};

  Serial.print("\n\nDisplay INA Readings V1.0.8\n");
  Serial.print(" - Searching & Initializing INA devices\n");
  for (INA_Class* ina : inaVector) {
    Serial.print("   - Begin\n");
    devicesFound = ina->begin(MAXIMUM_AMPS, SHUNT_MICRO_OHM); // Expected max Amp & shunt resistance
    while (devicesFound == 0)
    {
      Serial.println(F("No INA device found, retrying in 10 seconds..."));
      delay(10000);                                            // Wait 10 seconds before retrying
      devicesFound = ina->begin(MAXIMUM_AMPS, SHUNT_MICRO_OHM); // Expected max Amp & shunt resistance
    }                                                          // while no devices detected
    Serial.print(F(" - Detected "));
    Serial.print(devicesFound);
    Serial.println(F(" INA devices on the I2C bus"));
    ina->setBusConversion(8500);            // Maximum conversion time 8.244ms
    ina->setShuntConversion(8500);          // Maximum conversion time 8.244ms
    ina->setAveraging(16);                  // Average each reading n-times
    ina->setMode(INA_MODE_CONTINUOUS_BOTH); // Bus/shunt measured continuously
  }

  Serial.print(F("Lp   Nr AdrPin Type   Bus         Shunt       Bus         Bus\n"));
  Serial.print(F("==== == ====== ====== =========== =========== =========== ===========\n"));

}

void initOTA() {

  ArduinoOTA
    .onStart([]() {
      dual_log("Starting OTA update");
      // Print EOF character to close connections
      char eof_msg[2] = {0x04, 0};
      ws.printfAll(eof_msg);
      ws.closeAll();
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
}


/**
 * @brief SETUP
 * 
 */
void setup()
{
  Serial.begin(SERIAL_SPEED);

  // SD Card Setup
  Serial.println("Initializing SD card...");
  int sd_mounted = sd_setup();
  while(sd_mounted != 0) {
    Serial.println("FAILED: Waiting 10 seconds...");
    delay(10000);
    Serial.println("Reinitializing SD card...");
    sd_mounted = sd_setup();
  }

  // Set up directory structure (if the dirs do not exist)
  // Add "/daily"
  if (!SD.exists("/daily")) {
    SD.mkdir("/daily");
  }
  // Add "/full"
  if (!SD.exists("/full")) {
    SD.mkdir("/full");
  }

  // Connect to a wifi hotspot
  initWiFi();

  // // Host an Access Point
  // initSoftAP();

  // DS3231 Setup
  Serial.println("Initializing DS3231...");
  Wire.begin(WIRE_A_SDA, WIRE_A_SCL, INA_I2C_STANDARD_MODE);
  // rtc.begin();
  Serial.println("Setting time mode...");
  rtc.set24HourMode();
  // // Set the initial date and time
  // rtc.setTimeDate(2023, 7, 13, 1, 18, 0);

  // The the date and time at startup to set the internal ESP32 RTC
  uint8_t second, minute, hour, dayOfWeek, dayOfMonth, month;
  uint16_t year;
  rtc.getTimeDate(&year, &month, &dayOfMonth, &hour, &minute, &second, &dayOfWeek);
  // Initialize the internal ESP32 RTC
  esp32rtc.setTime(second,minute, hour, dayOfMonth, month, year);
  Serial.println("Time set to: " + esp32rtc.getTime("%Y-%m-%dT%H:%M:%S"));


  // INA226 Setup
  Serial.println("Initializing INA226...");
  ina_setup();

  // Website
  setupWebHandlers();

  // Over The Air Updates
  initOTA();

  // Record start time
  log_file = SD.open("/log.txt", FILE_WRITE);
  String logMessage = "Started at: " + rtc.getTimestamp();
  log_file.println(logMessage);
  log_file.println(WiFi.localIP());
}





String getINAMeasurements(INA_Class* ina, uint8_t deviceIndex, ShuntStats* stats) {

  static char sprintfBuffer[100];                                       // Buffer to format output
  static char busChar[20], shuntChar[20], busMAChar[20], busMWChar[20]; // Output buffers
  int32_t shuntRawVoltage = ina->getShuntRaw(deviceIndex);
  uint32_t busRawVoltage = ina->getBusRaw(deviceIndex);
  float shuntVolts = SHUNT_VOLTS_PER_LSB * shuntRawVoltage;
  float busVolts = BUS_VOLTS_PER_LSB * busRawVoltage;
  float busAmps = shuntVolts * 10000.0;
  float busWatts = busAmps * busVolts;
  sprintf(sprintfBuffer, "%f,%f,%f,%f,", busVolts, shuntVolts, busAmps, busWatts);
  
  stats->busVoltageStats.add_measurement(busRawVoltage);
  stats->shuntVoltageStats.add_measurement(shuntRawVoltage);
  stats->lastBusRawVoltage = busRawVoltage;
  stats->lastShuntRawVoltage = shuntRawVoltage;

  String buffer = sprintfBuffer;
  return buffer;
}


void getSimpleINAMeasurements(INA_Class* ina, uint8_t deviceIndex, ShuntStats& stats) {

  int32_t shuntRawVoltage = ina->getShuntRaw(deviceIndex);
  uint32_t busRawVoltage = ina->getBusRaw(deviceIndex);
  stats.busVoltageStats.add_measurement(busRawVoltage);
  stats.shuntVoltageStats.add_measurement(shuntRawVoltage);
  stats.lastBusRawVoltage = busRawVoltage;
  stats.lastShuntRawVoltage = shuntRawVoltage;
}

String getINAMeasurementsForCSV(INA_Class* ina, uint8_t deviceIndex) {

  static char sprintfBuffer[100];                                       // Buffer to format output
  static char busChar[20], shuntChar[20], busMAChar[20], busMWChar[20]; // Output buffers
  int32_t shuntRawVoltage = ina->getShuntRaw(deviceIndex);
  uint32_t busRawVoltage = ina->getBusRaw(deviceIndex);
  float shuntVolts = SHUNT_VOLTS_PER_LSB * shuntRawVoltage;
  float busVolts = BUS_VOLTS_PER_LSB * busRawVoltage;
  float busAmps = shuntVolts * 10000.0;
  float busWatts = busAmps * busVolts;
  sprintf(sprintfBuffer, "%f,%f,%f,%f,", busVolts, shuntVolts, busAmps, busWatts);
  return String(sprintfBuffer);
}

void showINAMeasurements()
{
  int statsIdx = 0;
  for (INA_Class* ina : inaVector) {
    for (uint8_t i = 0; i < ina->device_count; i++) // Loop through all devices
    {
      dual_log("showedINAMeasurements: stats: %u", statsIdx);

      // getINAMeasurementsForCSV(ina, i);
      String buffer = getINAMeasurements(ina, i, &shuntStatsArray[statsIdx]);
      // dual_log("%u: %s", statsIdx, buffer.c_str());
      statsIdx++;
    } 
  }
}

void showTime() {
  Serial.println(rtc.getTimestamp());
}

String getESP32RTCFSSafeTimestamp() {
  return esp32rtc.getTime("%Y%m%dT%H%M%S");
}

String getESP32RTCFSSafeDatestamp() {
  return esp32rtc.getTime("%Y%m%d");
}

String getESP32RTCISO8601Timestamp() {
  return esp32rtc.getTime("%Y-%m-%dT%H:%M:%S");
}

int32_t remainingMillisThisSecond() {
  struct timeval tv_now;
  gettimeofday(&tv_now, NULL);
  return ((1000000 - tv_now.tv_usec) / 1000) + 1;
}

// Calculate a checksum
uint64_t checksum = 0;

template<typename T>
void writeWithSize(const T& value) {
    uint16_t size = sizeof(value);
    int32_t zero = 0;
    log_file.write((uint8_t*)&zero, sizeof(zero));
    log_file.write((uint8_t*)&size, sizeof(size));
    log_file.write((uint8_t*)&value, sizeof(value));
    log_file.write((uint8_t*)&zero, sizeof(zero));
    checksum += value;
}

void appendAggregationsToDailyFile() {
  log_file.close();
  String timestampedLogFilePath = "/daily/" + getESP32RTCFSSafeDatestamp() + ".bin0";
  Serial.print("Daily log file path: ");
  Serial.println(timestampedLogFilePath);
  log_file = SD.open(timestampedLogFilePath, FILE_APPEND);
  
  // Reset the checksum
  checksum = 0;
  // Write the current timestamp
  struct timeval tv_now;
  gettimeofday(&tv_now, NULL);
  writeWithSize(tv_now.tv_sec);
  // Write the average voltages from the stats for each shunt

  // Loop through each shunt stats
  for (ShuntStats& stats : shuntStatsArray) {
    // Write the bus voltage stats
    writeWithSize(stats.busVoltageStats.min);
    writeWithSize(stats.busVoltageStats.get_mean());
    writeWithSize(stats.busVoltageStats.max);
    // Write the shunt voltage stats
    writeWithSize(stats.shuntVoltageStats.min);
    writeWithSize(stats.shuntVoltageStats.get_mean());
    writeWithSize(stats.shuntVoltageStats.max);
    // Reset the stats
    stats.busVoltageStats.reset();
    stats.shuntVoltageStats.reset();
  }

  // Write the checksum
  writeWithSize(checksum);
}

void openNewLogFile() {
  log_file.close();
  String timestampedLogFilePath = "/full/" + getESP32RTCFSSafeTimestamp() + ".bin0";
  Serial.print("Log file path: ");
  Serial.println(timestampedLogFilePath);
  log_file = SD.open(timestampedLogFilePath, FILE_APPEND);
}

void loop() {

  static int lastMinute = 255;
  static String timestampedLogFilePath;
  int minute = esp32rtc.getMinute();
  if (minute != lastMinute) {
    if (lastMinute != 255) {
      dual_log("appendAggregationsToDailyFile");
      appendAggregationsToDailyFile();
    }
    dual_log("openNewLogFile");
    openNewLogFile();
    lastMinute = minute;
  }

  // Calculate remaining milliseconds this second
  int32_t remainingMillis = remainingMillisThisSecond();
  // Use sprintf to make a string with the log message "Remaining: 53ms"
  char logMessage[20];
  dual_log("Remaining: %dms", remainingMillis);

  // ws.printfAll("{\"type\": \"LOG_MESSAGE\", \"data\": { \"message\": \"Remaining millis: %d\" } }", remainingMillis);
  // Calculate millis to delay to hit the .5 second
  int32_t delayMillis = remainingMillis - 500;
  // If we are past the half second mark, don't delay
  if (delayMillis > 0) {
    delay(delayMillis);
  }
  
  struct timeval tv_now;
  // gettimeofday(&tv_now, NULL);
  // Serial.print("Unix Time: ");
  // Serial.println(tv_now.tv_sec);
  // Serial.print("Micros: ");
  // Serial.println(tv_now.tv_usec);
  
  // Reset the checksum
  checksum = 0;

  // Write the current timestamp
  gettimeofday(&tv_now, NULL);
  time_t unix_timestamp = tv_now.tv_sec;
  writeWithSize(unix_timestamp);
  dual_log("showINAMeasurements");

  // Show the INA226 measurements
  showINAMeasurements();
  dual_log("showedINAMeasurements");

  // Loop through each shunt stats
  int shunt_idx = 0;
  for (ShuntStats& stats : shuntStatsArray) {
    // Write the bus voltage stats
    writeWithSize(stats.lastBusRawVoltage);
    // Write the shunt voltage stats
    writeWithSize(stats.lastShuntRawVoltage);
    dual_log("Shunt %d: {bus_voltage:%d, shunt_voltage:%d}", shunt_idx, stats.lastBusRawVoltage, stats.lastShuntRawVoltage);
    shunt_idx++;
  }
  dual_log("Writing checksum");

  // Write the checksum
  writeWithSize(checksum);

  Serial.println();
  log_file.println();
  dual_log("OTA handle");

  ArduinoOTA.handle();
  dual_log("Delay");

  delay(500);
}