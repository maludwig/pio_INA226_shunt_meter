/*!
 @file DisplayReadings.ino

 @brief Example program for the INA Library demonstrating reading an INA device and displaying
 results

 @section DisplayReadings_section Description

 Program to demonstrate the INA library for the Arduino. When started, the library searches the
 I2C bus for all INA2xx devices. Then the example program goes into an infinite loop and displays
 the power measurements (bus voltage and current) for all devices.\n\n

 Detailed documentation can be found on the GitHub Wiki pages at
 https://github.com/Zanduino/INA/wiki \n\n This example is for a INA set up to measure a 5-Volt
 load with a 0.1 Ohm resistor in place, this is the same setup that can be found in the Adafruit
 INA219 breakout board.  The complex calibration options are done at runtime using the 2
 parameters specified in the "begin()" call and the library has gone to great lengths to avoid the
 use of floating point to conserve space and minimize runtime.  This demo program uses floating
 point only to convert and display the data conveniently. The INA devices have 15 bits of
 precision, and even though the current and watt information is returned using 32-bit integers the
 precision remains the same.\n\n

 The library supports multiple INA devices and multiple INA device types. The Atmel's EEPROM is
 used to store the 96 bytes of static information per device using
 https://www.arduino.cc/en/Reference/EEPROM function calls. Although up to 16 devices could
 theoretically be present on the I2C bus the actual limit is determined by the available EEPROM -
 ATmega328 UNO has 1024k so can support up to 10 devices but the ATmega168 only has 512 bytes
 which limits it to supporting at most 5 INAs. Support has been added for the ESP32 based
 Arduinos, these use the EEPROM calls differently and need specific code.

 @section DisplayReadings_license GNU General Public License v3.0

 This program is free software : you can redistribute it and/or modify it under the terms of the
 GNU General Public License as published by the Free Software Foundation, either version 3 of the
 License, or (at your option) any later version.This program is distributed in the hope that it
 will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.You should
 have received a copy of the GNU General Public License along with this program(see
 https://github.com/Zanduino/INA/blob/master/LICENSE).  If not, see
 <http://www.gnu.org/licenses/>.

 @section DisplayReadings_author Author

 Written by Arnd <Arnd@Zanduino.Com> at https://www.github.com/SV-Zanshin

 @section DisplayReadings_versions Changelog

 | Version | Date       | Developer  | Comments                                                    |
 | ------- | ---------- | -----------| ----------------------------------------------------------- |
 | 1.0.8   | 2020-12-01 | SV-Zanshin | Issue #72. Allow dynamic RAM allocation instead of EEPROM   |
 | 1.0.7   | 2020-06-30 | SV-Zanshin | Issue #58. Changed formatting to use clang-format           |
 | 1.0.6   | 2020-06-29 | SV-Zanshin | Issue #57. Changed case of functions "Alert..."             |
 | 1.0.5   | 2020-05-03 | SV-Zanshin | Moved setting of maxAmps and shunt to constants             |
 | 1.0.4   | 2019-02-16 | SV-Zanshin | Reformatted and refactored for legibility and clarity       |
 | 1.0.3   | 2019-02-10 | SV-Zanshin | Issue #38. Made pretty-print columns line up                |
 | 1.0.3   | 2019-02-09 | SV-Zanshin | Issue #38. Added device number to display                   |
 | 1.0.2   | 2018-12-29 | SV-Zanshin | Converted comments to doxygen format                        |
 | 1.0.1   | 2018-09-22 | SV-Zanshin | Comments corrected, add INA wait loop, removed F("") calls  |
 | 1.0.0   | 2018-06-22 | SV-Zanshin | Initial release                                             |
 | 1.0.0b  | 2018-06-17 | SV-Zanshin | INA219 and INA226 completed, including testing              |
 | 1.0.0a  | 2018-06-10 | SV-Zanshin | Initial coding                                              |
*/

#if ARDUINO >= 100 // Arduino IDE versions before 100 need to use the older library
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include <INA.h> // Zanshin INA Library
#include <DS3231Time.h>
#include <Wire.h>

#if defined(_SAM3XA_) || defined(ARDUINO_ARCH_SAMD)
// The SAM3XA architecture needs to include this library, it is already included automatically on
// other platforms //
#include <avr/dtostrf.h> // Needed for the SAM3XA (Arduino Zero)
#endif

/**************************************************************************************************
** Declare program constants, global variables and instantiate INA class                         **
**************************************************************************************************/
const uint32_t SERIAL_SPEED{115200}; ///< Use fast serial speed

// // 1 Ohm
// const uint32_t SHUNT_MICRO_OHM{1000000};  ///< Shunt resistance in Micro-Ohm, e.g. 100000 is 0.1 Ohm
// 0.0001 Ohm
const uint32_t SHUNT_MICRO_OHM{100}; ///< Shunt resistance in Micro-Ohm, e.g. 100000 is 0.1 Ohm

const uint16_t MAXIMUM_AMPS{1}; ///< Max expected amps, clamped from 1A to a max of 1022A
uint8_t devicesFound{0};        ///< Number of INAs found
INA_Class INA;                  ///< INA class instantiation to use EEPROM
// INA_Class      INA(0);                ///< INA class instantiation to use EEPROM
// INA_Class      INA(5);                ///< INA class instantiation to use dynamic memory rather
//                                            than EEPROM. Allocate storage for up to (n) devices

DS3231Time rtc;

#define LED_PIN 27

// 2.5 uV per LSB
#define SHUNT_VOLTS_PER_LSB 0.0000025

// 1.25 mV per LSB
#define BUS_VOLTS_PER_LSB 0.00125

#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>

#include "functions.h"

void setup()
{
  /*!
   * @brief    Arduino method called once at startup to initialize the system
   * @details  This is an Arduino IDE method which is called first upon boot or restart. It is only
   *           called one time and then control goes to the "loop()" method, from which control
   *           never returns. The serial port is initialized and the INA.begin() method called to
   *           find all INA devices on the I2C bus and then the devices are initialized to given
   *           conversion and averaging rates.
   * @return   void
   */

  // initialize digital pin LED_PIN as an output.
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(SERIAL_SPEED);
#ifdef __AVR_ATmega32U4__ // If a 32U4 processor, then wait 2 seconds to initialize serial port
  delay(2000);
#endif
  Serial.print("\n\nDisplay INA Readings V1.0.8\n");
  Serial.print(" - Searching & Initializing INA devices\n");
  /************************************************************************************************
  ** The INA.begin call initializes the device(s) found with an expected ±1 Amps maximum current **
  ** and for a 0.1Ohm resistor, and since no specific device is given as the 3rd parameter all   **
  ** devices are initially set to these values.                                                  **
  ************************************************************************************************/
  devicesFound = INA.begin(MAXIMUM_AMPS, SHUNT_MICRO_OHM); // Expected max Amp & shunt resistance
  while (devicesFound == 0)
  {
    Serial.println(F("No INA device found, retrying in 10 seconds..."));
    delay(10000);                                            // Wait 10 seconds before retrying
    devicesFound = INA.begin(MAXIMUM_AMPS, SHUNT_MICRO_OHM); // Expected max Amp & shunt resistance
  }                                                          // while no devices detected
  Serial.print(F(" - Detected "));
  Serial.print(devicesFound);
  Serial.println(F(" INA devices on the I2C bus"));
  INA.setBusConversion(8500);            // Maximum conversion time 8.244ms
  INA.setShuntConversion(8500);          // Maximum conversion time 8.244ms
  INA.setAveraging(16);                  // Average each reading n-times
  INA.setMode(INA_MODE_CONTINUOUS_BOTH); // Bus/shunt measured continuously
  // INA.alertOnBusOverVoltage(true, 5000);  // Trigger alert if over 5V on bus

  Serial.print(F("Lp Nr Adr Type   Bus      Shunt       Bus         Bus\n"));
  Serial.print(F("== == === ====== ======== =========== =========== ===========\n"));

  // DS3231 Setup
  rtc.begin();
  rtc.set24HourMode();
  // Set the initial date and time to 2023-06-05 05:15:22
  rtc.setTimeDate(2023, 6, 5, 5, 52, 41);

} // method setup()

// void setup() {
//   Wire.begin();

//   Serial.begin(SERIAL_SPEED);
//   while (!Serial)
//      delay(10);
//   Serial.println("\nI2C Scanner");
// }

// void scan_wire() {
//   byte error, address;
//   int nDevices;

//   Serial.println("Scanning...");

//   nDevices = 0;
//   for(address = 1; address < 127; address++ )
//   {
//     // The i2c_scanner uses the return value of
//     // the Write.endTransmisstion to see if
//     // a device did acknowledge to the address.
//     Wire.beginTransmission(address);
//     error = Wire.endTransmission();

//     if (error == 0)
//     {
//       Serial.print("I2C device found at address 0x");
//       if (address<16)
//         Serial.print("0");
//       Serial.print(address,HEX);
//       Serial.println("  !");

//       nDevices++;
//     }
//     else if (error==4)
//     {
//       Serial.print("Unknown error at address 0x");
//       if (address<16)
//         Serial.print("0");
//       Serial.println(address,HEX);
//     }
//   }
//   if (nDevices == 0)
//     Serial.println("No I2C devices found\n");
//   else
//     Serial.println("done\n");

//   delay(5000);           // wait 5 seconds for next scan
// }

void showINAMeasurements(uint16_t loopCounter)
{

  static char sprintfBuffer[100];                                       // Buffer to format output
  static char busChar[20], shuntChar[20], busMAChar[20], busMWChar[20]; // Output buffers

  for (uint8_t i = 0; i < devicesFound; i++) // Loop through all devices
  {
    int32_t shuntRawVoltage = INA.getShuntRaw(i);
    uint32_t busRawVoltage = INA.getBusRaw(i);
    float shuntVolts = SHUNT_VOLTS_PER_LSB * shuntRawVoltage;
    float busVolts = BUS_VOLTS_PER_LSB * busRawVoltage;
    float busAmps = shuntVolts * 10000.0;
    float busWatts = busAmps * busVolts;
    si_format(busVolts, 10, 3, busChar);
    si_format(shuntVolts, 10, 3, shuntChar);
    si_format(busAmps, 10, 3, busMAChar);
    si_format(busWatts, 10, 4, busMWChar);
    sprintf(sprintfBuffer, "%2d, %2d %3d %s %sV %sV %sA %sW\n", loopCounter % 8, i + 1, INA.getDeviceAddress(i),
            INA.getDeviceName(i), busChar, shuntChar, busMAChar, busMWChar);
    Serial.print(sprintfBuffer);
  } // for-next each INA device loop
}

void showTime()
{
  Serial.println(rtc.getTimestamp());
}

void loop()
{
  static uint16_t loopCounter = 0; // Count the number of iterations
  static uint8_t ledPinLevel = HIGH;

  loopCounter++;
  if (loopCounter % 8 == 0)
  {
    if (ledPinLevel == HIGH)
    {
      ledPinLevel = LOW;
    }
    else
    {
      ledPinLevel = HIGH;
    }
  }
  digitalWrite(LED_PIN, ledPinLevel); // turn the LED on (HIGH is the voltage level)

  showINAMeasurements(loopCounter);
  Serial.println();
  showTime();
  delay(500); // Wait 2 seconds before next reading
} // method loop()
