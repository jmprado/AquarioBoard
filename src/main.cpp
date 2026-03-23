#include "config.h"
#include "display.h"
#include "sensors.h"

#include <math.h>

unsigned long lastSensorUpdate = 0;
unsigned long lastBubbleUpdate = 0;

// Global variables for sensor values
float temp1 = NAN;

void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
  }

  initSensors();
  initDisplay();

  // Initialize animations after display
  initAnimations();

  delay(100);
}

void loop()
{
  unsigned long currentMillis = millis();
  bool needsRedraw = false;

  // Update sensor readings every 10 seconds
  if (currentMillis - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL)
  {
    lastSensorUpdate = currentMillis;

    temp1 = readTempSensor1();

    Serial.print(F("Temp: "));
    if (isnan(temp1))
    {
      Serial.println(F("--.-"));
    }
    else
    {
      Serial.println(temp1);
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

  if (needsRedraw)
  {
    drawScreen(temp1);
  }
}
