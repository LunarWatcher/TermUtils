#pragma once

#include <any>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include "TermUtils.hpp"
#include <codecvt>
#include <locale>

namespace termutil {

// Enable forwarding of std::endl
typedef std::basic_ostream<char, std::char_traits<char>> CoutType;
typedef CoutType& (*StandardEndLine)(CoutType&);

/**
 * Enum for properly mapping the ANSI features.
 */
enum class ANSIFeature {
    CLEAR = 0,
    BOLD = 1,
    ITALIC = 3, // According to Wikipedia, this is some times treated as inverse
    UNDERLINE = 4,
    BLINK = 5,
    STRIKE = 9,
    FOREGROUND = 38,
    BACKGROUND = 48
};

class ColorPrinter {
private:
    /**
     * Cache for whether or not the provided output stream supports ansi.
     * This variable is a reflection of TermUtils::supportsAnsi(stream),
     * and is calculated in the constructor.
     *
     * Note that certain variable changes involving processing256 will
     * still be computed in order to drop all ANSI-related input.
     * In the event this variable is false, the processing256 variable
     * is a sign to drop the next variable instead of using it for
     * something useful.
     */
    bool supportsAnsi;

    /**
     * Internal variable; used to tell certain operator<<
     * functions that they're about to get ANSI input or ANSI-related
     * input that requires special processing.
     *
     * If supportsAnsi = false, this doubles down as a flag to discard
     * certain parts of the input.
     */
    bool processing256 = false;
    std::ostream& outputStream;

    bool shouldReturnEarly() {
        if (!supportsAnsi && processing256) {
            processing256 = false;
            return true;
        }
        return false;
    }

public:
    ColorPrinter(std::ostream& outputStream) : outputStream(outputStream) {
        supportsAnsi = TermUtils::supportsAnsi(&outputStream);
    }
    ColorPrinter() : ColorPrinter(std::cout) {}

    template <typename T>
    friend ColorPrinter& operator<<(ColorPrinter&, T);
    friend ColorPrinter& operator<<(ColorPrinter&, ANSIFeature);
    friend ColorPrinter& operator<<(ColorPrinter&, const std::string&);
    friend ColorPrinter& operator<<(ColorPrinter&, int);
    friend ColorPrinter& operator<<(ColorPrinter&, StandardEndLine manip);
};

template <typename T>
ColorPrinter& operator<<(ColorPrinter& printer, T type) {
    printer.outputStream << type;
    return printer;
}

inline ColorPrinter& operator<<(ColorPrinter& printer, ANSIFeature feature) {
    if (!printer.supportsAnsi) {
        // While we're dropping the print itself, we can't drop this part.
        // If we do, we get a number printed out, which isn't what we want
        printer.processing256 = true;
        // Silently drop print
        return printer;
    }
    if (printer.processing256)
        throw std::runtime_error("Cannot print a new ANSI feature while attempting to print a color");

    printer.outputStream << "\033[" << int(feature);
    if (feature == ANSIFeature::FOREGROUND || feature == ANSIFeature::BACKGROUND) {
        printer.processing256 = true;
        printer.outputStream << ";5;";
    } else {
        printer.outputStream << "m";
    }

    return printer;
}

inline ColorPrinter& operator<<(ColorPrinter& printer, const std::string& str) {

    if (printer.shouldReturnEarly()) {
        return printer;
    }
    printer.outputStream << str;
    if (printer.processing256) {
        printer.processing256 = false;

        printer.outputStream << "m";
    }
    return printer;
}

inline ColorPrinter& operator<<(ColorPrinter& printer, int code) {
    if (printer.shouldReturnEarly()) {
        return printer;
    }
    printer.outputStream << code;
    if (printer.processing256) {
        printer.processing256 = false;
        printer.outputStream << "m";
    }
    return printer;
}

inline ColorPrinter& operator<<(ColorPrinter& printer, StandardEndLine manip) {
    manip(printer.outputStream);
    return printer;
}

} // namespace termutil
