#include "sensors.h"

#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#include "config.h"

static OneWire oneWire_1(temp_sensor_1_bus);
static DallasTemperature sensors(&oneWire_1);

static float phSensorAvgVoltage();
static float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);

void initSensors()
{
  sensors.begin();
}

float readTempSensor1()
{
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

float readPhSensor(float tempC)
{
  float vMedio = phSensorAvgVoltage();
  Serial.print(F("phV: "));
  Serial.print(vMedio);
  Serial.print(F(" - "));

  float phCalculado;

  if (vMedio >= V_NEUTRO)
  {
    phCalculado = mapFloat(vMedio, V_ACIDO, V_NEUTRO, 2.6, 7.0);
  }
  else
  {
    phCalculado = mapFloat(vMedio, V_NEUTRO, V_BASICO, 7.0, 8.2);
  }

  phCalculado = phCalculado + ((25.0 - tempC) * 0.03);

  return phCalculado;
}

static float phSensorAvgVoltage()
{
  float somaTensao = 0;
  const uint8_t numAmostras = 50;

  for (uint8_t i = 0; i < numAmostras; i++)
  {
    somaTensao += analogRead(PH_SENSOR_BUS) * (5.0 / 1023.0);
    delay(5);
  }

  return somaTensao / (float)numAmostras;
}

static float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
