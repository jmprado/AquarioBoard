#include "sensors.h"

#include <Arduino.h>
#include <DallasTemperature.h>
#include <math.h>
#include <OneWire.h>

#include "config.h"

static OneWire oneWire_1(temp_sensor_1_bus);
static DallasTemperature sensors(&oneWire_1);

static bool hasLastValidTemp = false;
static float lastValidTemp = NAN;
static bool hasLastValidPh = false;
static float lastValidPh = NAN;
static float lastPhVoltage = NAN;
static bool lastPhOutOfBounds = false;

static float phSensorAvgVoltage();
static float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
static float calculatePhFromVoltage(float voltage);
static float readAnalogReferenceVoltage();
static bool isInvalidDs18b20Reading(float tempC);

void initSensors()
{
  sensors.begin();
  sensors.setWaitForConversion(true);
  sensors.requestTemperatures();
}

float readTempSensor1()
{
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  // DS18B20 may return 85.0 at startup and -127.0 when disconnected/not ready.
  if (isInvalidDs18b20Reading(tempC))
  {
    sensors.requestTemperatures();
    tempC = sensors.getTempCByIndex(0);
  }

  if (isInvalidDs18b20Reading(tempC))
  {
    return hasLastValidTemp ? lastValidTemp : NAN;
  }

  hasLastValidTemp = true;
  lastValidTemp = tempC;

  return tempC;
}

float readPhSensor(float tempC)
{
  float vMedio = phSensorAvgVoltage();
  lastPhVoltage = vMedio;
  Serial.print(F("phV: "));
  Serial.print(vMedio);
  Serial.print(F(" - "));

  float minCalV = V_ACIDO;
  float maxCalV = V_ACIDO;
  if (V_NEUTRO < minCalV)
    minCalV = V_NEUTRO;
  if (V_BASICO < minCalV)
    minCalV = V_BASICO;
  if (V_NEUTRO > maxCalV)
    maxCalV = V_NEUTRO;
  if (V_BASICO > maxCalV)
    maxCalV = V_BASICO;

  const float validMinV = minCalV - 0.25;
  const float validMaxV = maxCalV + 0.25;

  lastPhOutOfBounds = (vMedio < validMinV || vMedio > validMaxV);

  if (lastPhOutOfBounds)
  {
    Serial.print(F("phV fora da faixa ("));
    Serial.print(validMinV, 2);
    Serial.print(F(".."));
    Serial.print(validMaxV, 2);
    Serial.print(F(") - mantendo ultimo valor valido"));
    return hasLastValidPh ? lastValidPh : NAN;
  }

  float phCalculado = calculatePhFromVoltage(vMedio);

  if (phCalculado < 0.0)
    phCalculado = 0.0;
  if (phCalculado > 14.0)
    phCalculado = 14.0;

  Serial.print(F("phBase: "));
  Serial.print(phCalculado, 2);
  if (!isnan(tempC))
  {
    Serial.print(F(" - temp: "));
    Serial.print(tempC, 1);
  }

  hasLastValidPh = true;
  lastValidPh = phCalculado;

  return phCalculado;
}

float getLastPhVoltage()
{
  return lastPhVoltage;
}

bool isPhOutOfBounds()
{
  return lastPhOutOfBounds;
}

static float phSensorAvgVoltage()
{
  float somaTensao = 0;
  const uint8_t numAmostras = 50;
  const float adcReferenceVoltage = readAnalogReferenceVoltage();

  for (uint8_t i = 0; i < numAmostras; i++)
  {
    somaTensao += analogRead(PH_SENSOR_BUS) * (adcReferenceVoltage / 1023.0f);
    delay(5);
  }

  return somaTensao / (float)numAmostras;
}

static float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  if (in_max == in_min)
  {
    return out_min;
  }

  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static float calculatePhFromVoltage(float voltage)
{
  if (voltage >= V_NEUTRO)
  {
    return mapFloat(voltage, V_ACIDO, V_NEUTRO, 4.0, 7.0);
  }

  return mapFloat(voltage, V_NEUTRO, V_BASICO, 7.0, 10.0);
}

static float readAnalogReferenceVoltage()
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

static bool isInvalidDs18b20Reading(float tempC)
{
  return tempC <= -126.5f || (tempC >= 84.5f && tempC <= 85.5f);
}
