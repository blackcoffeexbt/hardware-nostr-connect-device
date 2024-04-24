#ifndef UTILITIES_H
#define UTILITIES_H

#include <Arduino.h>

namespace utilities {
    extern unsigned long timer;

    void startTimer(const char* timedEvent);
    void stopTimer(const char* timedEvent);

}
#endif