#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>

void si_format(float number, int min_width, int precision, char* buffer) {
    std::string suffix = "";
    bool isNegative = false;

    // Check if the number is negative
    if (number < 0.0f) {
        isNegative = true;
        number = -number;  // Work with the absolute value and add the sign at the end.
    }

    // Adjust the number and select the appropriate suffix.
    if (number >= 1.0e15f) {
        number /= 1.0e15f;
        suffix = "P";
    } else if (number >= 1.0e12f) {
        number /= 1.0e12f;
        suffix = "T";
    } else if (number >= 1.0e9f) {
        number /= 1.0e9f;
        suffix = "G";
    } else if (number >= 1.0e6f) {
        number /= 1.0e6f;
        suffix = "M";
    } else if (number >= 1.0e3f) {
        number /= 1.0e3f;
        suffix = "k";
    } else if (number >= 1.0f || number == 0.0) {
        // Do nothing.
    } else if (number >= 0.001f) {
        number *= 1.0e3f;
        suffix = "m";
    } else if (number >= 0.000001f) {
        number *= 1.0e6f;
        suffix = "u";
    } else if (number >= 0.000000001f) {
        number *= 1.0e9f;
        suffix = "n";
    } else {
        number *= 1.0e12f;
        suffix = "p";
    }

    // Create a string stream for the formatted output.
    if (isNegative) {
      number = -number;
    }

    std::ostringstream formattedNumber;
    formattedNumber << std::fixed << std::setprecision(precision) << number << suffix;
    std::string formatted = formattedNumber.str();

    int paddingSize = min_width - formatted.length();
    if (paddingSize > 0) {
        formatted = std::string(paddingSize, ' ') + formatted;
    }

    // Copy the formatted string into the buffer.
    strncpy(buffer, formatted.c_str(), formatted.length());
    buffer[formatted.length()] = '\0';  // Null-terminate the string in the buffer.
}
