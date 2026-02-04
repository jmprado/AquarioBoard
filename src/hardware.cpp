#include "hardware.h"

#include "config.h"

DS3231 rtc;

bool relay1Override = false;
static bool lastRelay1State = LOW;
static unsigned long lastDebounceTime = 0;
static const unsigned long debounceDelay = 50;

void setupHardware()
{
  pinMode(relay_1_bus, OUTPUT);
  pinMode(relay_2_bus, OUTPUT);
  pinMode(button_pin, INPUT_PULLUP);

  digitalWrite(relay_1_bus, LOW);
  digitalWrite(relay_2_bus, LOW);

  randomSeed(analogRead(A1));
}

void initRtc()
{
  rtc.begin();
  // rtc.setDateTime(__DATE__, __TIME__);
}

RTCDateTime getRtcDateTime()
{
  return rtc.getDateTime();
}

void updateRelay1(const RTCDateTime &rtcDateTime)
{
  int currentTime = rtcDateTime.hour * 100 + rtcDateTime.minute;

  bool desiredRelay1State;
  if (relay1Override)
  {
    desiredRelay1State = HIGH;
  }
  else
  {
    if (currentTime >= 1000 && currentTime < 1800)
    {
      desiredRelay1State = HIGH;
    }
    else
    {
      desiredRelay1State = LOW;
    }
  }

  if (desiredRelay1State != lastRelay1State)
  {
    digitalWrite(relay_1_bus, desiredRelay1State);
    lastRelay1State = desiredRelay1State;

    Serial.print(F("Relay 1: "));
    Serial.print(desiredRelay1State == HIGH ? F("ON") : F("OFF"));
    Serial.print(F(" ["));
    Serial.print(relay1Override ? F("Override") : F("Scheduled"));
    Serial.print(F("] Time: "));
    Serial.print(rtcDateTime.hour);
    Serial.print(F(":"));
    Serial.println(rtcDateTime.minute);
  }
}

void handleButton()
{
  static bool buttonState = HIGH;
  static bool lastReading = HIGH;
  static bool initialized = false;

  bool reading = digitalRead(button_pin);

  if (!initialized)
  {
    buttonState = reading;
    lastReading = reading;
    initialized = true;
    lastDebounceTime = millis();
    return;
  }

  if (reading != lastReading)
  {
    lastDebounceTime = millis();
    lastReading = reading;
  }

  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (reading != buttonState)
    {
      buttonState = reading;
      if (buttonState == LOW)
      {
        relay1Override = !relay1Override;

        Serial.println(F("=== Override Toggled ==="));
        Serial.print(F("  Mode: "));
        Serial.println(relay1Override ? F("OVERRIDE ON") : F("SCHEDULED"));
      }
    }
  }
}
