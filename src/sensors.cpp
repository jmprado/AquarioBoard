#include "sensors.h"

#include <Arduino.h>
#include <DallasTemperature.h>
#include <math.h>
#include <OneWire.h>

#include "config.h"

static OneWire oneWire_1(temp_sensor_1_bus);
static DallasTemperature sensors(&oneWire_1);

static bool hasLastValidPh = false;
static float lastValidPh = 7.0;

static float phSensorAvgVoltage();

void initSensors()
{
  sensors.begin();

  const uint8_t count = sensors.getDeviceCount();
  Serial.print(F("DS18B20 devices: "));
  Serial.println(count);

  if (count == 0)
  {
    Serial.println(F("DS18B20 not found on bus"));
    return;
  }

  DeviceAddress addr;
  if (!sensors.getAddress(addr, 0))
  {
    Serial.println(F("DS18B20 found but failed to read address"));
    return;
  }

  Serial.print(F("DS18B20 resolution: "));
  Serial.print(sensors.getResolution(addr));
  Serial.println(F(" bits"));
}

float readTempSensor1()
{
  sensors.requestTemperatures();
  const float tempC = sensors.getTempCByIndex(0);

  if (tempC == DEVICE_DISCONNECTED_C || tempC <= -126.0f)
  {
    Serial.println(F("DS18B20 read failed (disconnected or wiring issue)"));
    return NAN;
  }

  return tempC;
}

float readPhSensor(float tempC)
{
  float vMedio = phSensorAvgVoltage();
  Serial.print(F("phV: "));
  Serial.print(vMedio);
  Serial.print(F(" - "));

  const float minCalV = (V_BASICO < V_ACIDO) ? V_BASICO : V_ACIDO;
  const float maxCalV = (V_BASICO > V_ACIDO) ? V_BASICO : V_ACIDO;
  const float validMinV = minCalV - 0.25;
  const float validMaxV = maxCalV + 0.25;

  if (vMedio < validMinV || vMedio > validMaxV)
  {
    Serial.print(F("phV fora da faixa ("));
    Serial.print(validMinV, 2);
    Serial.print(F(".."));
    Serial.print(validMaxV, 2);
    Serial.print(F(") - "));
    return hasLastValidPh ? lastValidPh : 7.0;
  }

  float phCalculado;

  if (vMedio >= V_NEUTRO)
  {
    // Acid side: interpolate between pH 7 and pH 4 calibration points.
    const float slope74 = (4.0f - 7.0f) / (V_ACIDO - V_NEUTRO);
    const float offset74 = 7.0f - (slope74 * V_NEUTRO);
    phCalculado = (slope74 * vMedio) + offset74;
  }
  else
  {
    // Basic side: interpolate between pH 7 and pH 10 calibration points.
    const float slope710 = (10.0f - 7.0f) / (V_BASICO - V_NEUTRO);
    const float offset710 = 7.0f - (slope710 * V_NEUTRO);
    phCalculado = (slope710 * vMedio) + offset710;
  }

  if (!isnan(tempC) && tempC > -20.0 && tempC < 80.0)
  {
    phCalculado = phCalculado + ((25.0 - tempC) * 0.03);
  }

  if (phCalculado < 0.0)
    phCalculado = 0.0;
  if (phCalculado > 14.0)
    phCalculado = 14.0;

  hasLastValidPh = true;
  lastValidPh = phCalculado;

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
