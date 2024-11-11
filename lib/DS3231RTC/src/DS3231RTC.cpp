#include <Wire.h>
#include <DS3231RTC.h>

uint8_t DS3231RTC::bcdToDec(uint8_t val)  {
  return ((val / 16 * 10) + (val % 16));
}

uint8_t DS3231RTC::decToBcd(uint8_t val) {
  return ((val / 10 * 16) + (val % 10));
}

void DS3231RTC::begin() {
  Wire.begin();
}

uint32_t DS3231RTC::daysThisMillennia(uint16_t year, uint8_t month, uint8_t day) {
  uint32_t n = day;
  for (uint8_t m = month - 1; m > 0; --m) // count complete months
    n += daysInMonth(m, year);
  for (uint16_t y = year; y > 2000; --y) // count complete years
    n += daysInYear(y - 1);
  return n;
}

uint8_t DS3231RTC::daysInMonth(uint8_t month, uint16_t year) {
  uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  uint8_t days = daysInMonth[month - 1];
  if (month == 2 && isLeapYear(year)) // adjust for leap years
    ++days;
  return days;
}

bool DS3231RTC::isLeapYear(uint16_t year) {
  return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

uint16_t DS3231RTC::daysInYear(uint16_t year) {
  return isLeapYear(year) ? 366 : 365;
}

uint8_t DS3231RTC::calcDayOfWeek(uint16_t year, uint8_t month, uint8_t day) {
  // 2000-01-01 was a Saturday (6)
  return (daysThisMillennia(year, month, day) + 5) % 7 + 1;
}

void DS3231RTC::setTimeDate(uint16_t year, uint8_t month, uint8_t dayOfMonth, uint8_t hour, uint8_t minute, uint8_t second) {
  uint8_t dayOfWeek = calcDayOfWeek(year, month, dayOfMonth);
  uint8_t byteYear = year - 2000;
  
  Wire.beginTransmission(_deviceAddress);
  Wire.write(DS_REG_SEC); 
  Wire.write(decToBcd(second)); 
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(dayOfWeek)); // dayOfWeek (1 = Sunday, 7 = Saturday)
  Wire.write(decToBcd(dayOfMonth));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(byteYear)); // year from 00 to 99
  Wire.endTransmission();
}

void DS3231RTC::getTimeDate(uint16_t *year, uint8_t *month, uint8_t *dayOfMonth, uint8_t *hour, uint8_t *minute, uint8_t *second, uint8_t *dayOfWeek) {
  Wire.beginTransmission(_deviceAddress);
  Wire.write(DS_REG_SEC);
  Wire.endTransmission();
  
  Wire.requestFrom(_deviceAddress, 7);
  
  *second = bcdToDec(Wire.read() & 0x7F);
  *minute = bcdToDec(Wire.read());
  
  uint8_t rawHour = Wire.read();
  
  bool is12HourMode = rawHour & 0x40; // Check bit 6 (12/24-hour mode)
  bool isPM = rawHour & 0x20; // Check bit 5 (AM/PM in 12h, 20h in 24h)
  
  if (is12HourMode) {
    *hour = bcdToDec(rawHour & 0x1F); // If in 12h mode, mask the mode bits and convert
    if (isPM) {
      *hour += 12; // If PM, add 12 hours
    }
  } else {
    *hour = bcdToDec(rawHour & 0x3F); // If in 24h mode, mask the mode bits and convert
  }
  
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  uint8_t byteYear = bcdToDec(Wire.read());
  *year = byteYear + 2000;
}


void DS3231RTC::set24HourMode() {
  Wire.beginTransmission(_deviceAddress);
  Wire.write(DS_REG_HOUR); // Go to REG_HOUR
  Wire.endTransmission();

  Wire.requestFrom(_deviceAddress, 1);
  uint8_t hour = Wire.read();
  hour &= ~(1 << 6); // Clear bit 6 (24-hour mode)

  Wire.beginTransmission(_deviceAddress);
  Wire.write(DS_REG_HOUR); // Go back to REG_HOUR
  Wire.write(hour); // Write new hour
  Wire.endTransmission();
}

void DS3231RTC::set12HourMode() {
  Wire.beginTransmission(_deviceAddress);
  Wire.write(DS_REG_HOUR); // Go to REG_HOUR
  Wire.endTransmission();

  Wire.requestFrom(_deviceAddress, 1);
  uint8_t hour = Wire.read();
  hour |= (1 << 6); // Set bit 6 (12-hour mode)

  Wire.beginTransmission(_deviceAddress);
  Wire.write(DS_REG_HOUR); // Go back to REG_HOUR
  Wire.write(hour); // Write new hour
  Wire.endTransmission();
}

String DS3231RTC::getTimestamp() {
  uint8_t second, minute, hour, dayOfWeek, dayOfMonth, month;
  uint16_t year;
  getTimeDate(&year,&month, &dayOfMonth, &hour, &minute, &second, &dayOfWeek);

  char timestamp[20];
  sprintf(timestamp, "%04d-%02d-%02dT%02d:%02d:%02d", year, month, dayOfMonth, hour, minute, second);
  
  return String(timestamp);
}

String DS3231RTC::getFSSafeTimestamp() {
  uint8_t second, minute, hour, dayOfWeek, dayOfMonth, month;
  uint16_t year;
  getTimeDate(&year,&month, &dayOfMonth, &hour, &minute, &second, &dayOfWeek);

  char timestamp[20];
  sprintf(timestamp, "%04d-%02d-%02dT%02d-%02d-%02d", year, month, dayOfMonth, hour, minute, second);
  
  return String(timestamp);
}
