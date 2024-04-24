#include "utilities.h"

namespace utilities {
    unsigned long timer = 0;
    void startTimer(const char* timedEvent) {
        timer = millis();
        Serial.print("Starting timer for ");
        Serial.println(timedEvent);
    }

    void stopTimer(const char* timedEvent) {
        unsigned long elapsedTime = millis() - timer;
        Serial.print(elapsedTime);
        Serial.print(" ms - ");
        Serial.println(timedEvent);
        timer = millis();
    }
}