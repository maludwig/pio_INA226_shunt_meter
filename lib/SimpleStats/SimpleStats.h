#ifndef SIMPLESTATS_h
#define SIMPLESTATS_h

#include <stdint.h>

class SimpleStats {
public:

    int64_t sum;
    int64_t min;
    int64_t max;
    uint32_t count;

    // Constructor
    SimpleStats() : sum(0), min(INT32_MAX), max(INT32_MIN), count(0) {}

    // Method to add a measurement
    void add_measurement(int32_t measurement) {
        sum += measurement;
        count++;
        if(measurement < min) min = measurement;
        if(measurement > max) max = measurement;
    }
    
    void add_measurement(uint32_t measurement) {
        sum += measurement;
        count++;
        if(measurement < min) min = measurement;
        if(measurement > max) max = measurement;
    }

    int32_t get_mean() const {
        if (count == 0) return 0;
        // Compute the average in 64 bit
        int64_t mean64 = sum / count;
        // But return the result in 32 bit (the average of a set of 32 bit numbers should be 32 bit)
        return static_cast<int32_t>(mean64);
    }

    // Method to reset the statistics
    void reset() {
        sum = 0;
        min = INT32_MAX;
        max = INT32_MIN;
        count = 0;
    }
};

#endif
