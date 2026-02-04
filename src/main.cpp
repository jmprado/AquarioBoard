#include "config.h"
#include "display.h"
#include "hardware.h"
#include "sensors.h"

unsigned long lastSensorUpdate = 0;
unsigned long lastBubbleUpdate = 0;

void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
  }

  setupHardware();
  initSensors();
  initDisplay();
  initRtc();

  // Set RTC time - UNCOMMENT AND MODIFY THIS LINE TO SET THE CORRECT TIME
  // rtc.setDateTime(__DATE__, __TIME__);
  // After setting the time once, comment it out again and re-upload

  // Initialize animations after hardware
  initAnimations();

  delay(100);
}

// Global variables for sensor values
float temp1 = 0.0;
float ph = 0.0;

void loop()
{
  unsigned long currentMillis = millis();
  bool needsRedraw = false;

  // Handle button press for override
  handleButton();

  // Get current time and control relay_1 (10:00 AM to 6:00 PM)
  RTCDateTime rtcDateTime = getRtcDateTime();
  updateRelay1(rtcDateTime);

  // Update sensor readings every 10 seconds
  if (currentMillis - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL)
  {
    lastSensorUpdate = currentMillis;

    temp1 = readTempSensor1();
    ph = readPhSensor(temp1);

    Serial.print(F("Temp: "));
    Serial.print(temp1);
    Serial.print(F(" - pH: "));
    Serial.println(ph);

    if (relay1Override)
    {
      Serial.println(F("LIGHTS ON"));
    }

    needsRedraw = true;
  }

  // Update animations every 100ms
  if (currentMillis - lastBubbleUpdate >= BUBBLE_UPDATE_INTERVAL)
  {
    lastBubbleUpdate = currentMillis;
    updateAnimations();
    needsRedraw = true;
  }

  // Only redraw screen when something changed
  if (needsRedraw)
  {
    drawScreen(temp1, ph, relay1Override);
  }
}
