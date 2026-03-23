#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#include <stdio.h>

#include <math.h>

#include "config.h"

#ifdef PH_CALIBRATION_SKETCH

namespace
{
const float TARGET_SHORT_VOLTAGE = 2.50f;
const uint16_t SAMPLE_COUNT = 60;
const uint16_t SAMPLE_DELAY_MS = 10;
const unsigned long SAMPLE_INTERVAL_MS = 800;
const float NO_SIGNAL_MIN_V = 0.03f;
const float NO_SIGNAL_MAX_V = 4.95f;
const float FLOATING_SPAN_V = 0.25f;

float lastVoltage = 0.0f;
bool lastVoltageValid = false;
unsigned long lastSampleMs = 0;

struct VoltageSampleStats
{
  float avg;
  float min;
  float max;
};

static U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

float readAnalogReferenceVoltage()
{
#if defined(__AVR__)
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);
  ADCSRA |= _BV(ADSC);
  while (ADCSRA & _BV(ADSC))
  {
  }

  uint16_t rawBandgap = ADC;
  if (rawBandgap == 0)
  {
    return 5.0f;
  }

  return 1125.3f / (float)rawBandgap;
#else
  return 5.0f;
#endif
}

VoltageSampleStats readVoltageStats()
{
  float sum = 0.0f;
  float minV = 999.0f;
  float maxV = -1.0f;
  const float adcReferenceVoltage = readAnalogReferenceVoltage();

  for (uint16_t i = 0; i < SAMPLE_COUNT; i++)
  {
    float v = analogRead(PH_SENSOR_BUS) * (adcReferenceVoltage / 1023.0f);
    sum += v;
    if (v < minV)
      minV = v;
    if (v > maxV)
      maxV = v;
    delay(SAMPLE_DELAY_MS);
  }

  VoltageSampleStats stats;
  stats.avg = sum / SAMPLE_COUNT;
  stats.min = minV;
  stats.max = maxV;
  return stats;
}

bool isVoltageReadingValid(const VoltageSampleStats &stats)
{
  if (stats.avg <= NO_SIGNAL_MIN_V || stats.avg >= NO_SIGNAL_MAX_V)
  {
    return false;
  }

  return (stats.max - stats.min) < FLOATING_SPAN_V;
}

void formatVoltage(char *buffer, size_t bufferSize, float voltage)
{
  char numberBuffer[12];

  dtostrf(voltage, 4, 2, numberBuffer);
  snprintf(buffer, bufferSize, "%sV", numberBuffer);
}

void drawVoltageScreen(float voltage, bool isValid)
{
  char voltageText[12];
  char deltaText[12];

  if (isValid)
  {
    formatVoltage(voltageText, sizeof(voltageText), voltage);
    dtostrf(voltage - TARGET_SHORT_VOLTAGE, 5, 2, deltaText);
  }
  else
  {
    snprintf(voltageText, sizeof(voltageText), "--.--V");
    snprintf(deltaText, sizeof(deltaText), " --.--");
  }

  u8g2.firstPage();
  do
  {
    u8g2.setDrawColor(1);
    u8g2.setFontPosTop();
    u8g2.setFontDirection(0);

    u8g2.setFont(u8g2_font_6x13_tf);
    u8g2.drawStr(0, 0, "Sensor voltage");
    u8g2.drawHLine(0, 14, 128);

    u8g2.setFont(u8g2_font_5x7_tf);
    if (isValid)
    {
      u8g2.drawStr(0, 17, "Current pH sensor output");
    }
    else
    {
      u8g2.drawStr(0, 17, "No valid signal (sensor off?)");
    }

    u8g2.drawRFrame(6, 26, 116, 24, 5);
    u8g2.setFont(u8g2_font_logisoso18_tf);
    u8g2.drawStr(16, 29, voltageText);

    u8g2.setFont(u8g2_font_6x13_tf);
    u8g2.drawStr(0, 54, "Delta:");
    u8g2.drawStr(42, 54, deltaText);
    u8g2.drawStr(82, 54, "V");
  } while (u8g2.nextPage());
}

void updateReadings()
{
  VoltageSampleStats stats = readVoltageStats();
  lastVoltageValid = isVoltageReadingValid(stats);
  if (lastVoltageValid)
  {
    lastVoltage = stats.avg;
  }
}
} // namespace

void setup()
{
  u8g2.begin();
  updateReadings();
  drawVoltageScreen(lastVoltage, lastVoltageValid);
}

void loop()
{
  const unsigned long now = millis();

  if ((now - lastSampleMs) >= SAMPLE_INTERVAL_MS)
  {
    lastSampleMs = now;
    updateReadings();
    drawVoltageScreen(lastVoltage, lastVoltageValid);
  }
}

#endif
