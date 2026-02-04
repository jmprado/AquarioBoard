#pragma once

#include <Arduino.h>
#include <DS3231.h>

void setupHardware();
void initRtc();
RTCDateTime getRtcDateTime();
void updateRelay1(const RTCDateTime &rtcDateTime);
void handleButton();

extern bool relay1Override;
