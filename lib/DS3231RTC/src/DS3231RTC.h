#ifndef DS3231RTC_h
#define DS3231RTC_h

#include <Wire.h>

class DS3231RTC {
  private:
    static const int _deviceAddress = 0x68;
    static const uint8_t DS_REG_SEC = 0x00;
    static const uint8_t DS_REG_MIN = 0x01;
    static const uint8_t DS_REG_HOUR = 0x02;
    static const uint8_t DS_REG_DOW = 0x03;
    static const uint8_t DS_REG_DATE = 0x04;
    static const uint8_t DS_REG_MON = 0x05;
    static const uint8_t DS_REG_YEAR = 0x06;
    static const uint8_t DS_REG_CON = 0x0e;
    static const uint8_t DS_REG_STATUS = 0x0f;
    static const uint8_t DS_REG_AGING = 0x10;
    static const uint8_t DS_REG_TEMPM = 0x11;
    static const uint8_t DS_REG_TEMPL = 0x12;
    
    uint8_t bcdToDec(uint8_t val);
    uint8_t decToBcd(uint8_t val);

  public:
    void begin();
    uint32_t daysThisMillennia(uint16_t year, uint8_t month, uint8_t day);
    uint8_t daysInMonth(uint8_t month, uint16_t year);
    bool isLeapYear(uint16_t year);
    uint16_t daysInYear(uint16_t year);
    uint8_t calcDayOfWeek(uint16_t year, uint8_t month, uint8_t day);
    void setTimeDate(uint16_t year, uint8_t month, uint8_t dayOfMonth, uint8_t hour, uint8_t minute, uint8_t second);
    void getTimeDate(uint16_t *year, uint8_t *month, uint8_t *dayOfMonth, uint8_t *hour, uint8_t *minute, uint8_t *second, uint8_t *dayOfWeek);
    void set24HourMode();
    void set12HourMode();
    String getTimestamp();
    String getFSSafeTimestamp();
};

#endif
