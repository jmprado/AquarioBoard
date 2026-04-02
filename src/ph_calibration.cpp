#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#include "config.h"

#ifdef PH_CALIBRATION_SKETCH

namespace
{
const float TARGET_SHORT_VOLTAGE = 2.50f;
const uint16_t SAMPLE_COUNT = 60;
const uint16_t SAMPLE_DELAY_MS = 10;

float vAcid = V_ACIDO;
float vNeutral = V_NEUTRO;
float vBasic = V_BASICO;

bool streamReadings = true;
unsigned long lastStreamMs = 0;

static U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

float readAverageVoltage()
{
  float sum = 0.0f;

  for (uint16_t i = 0; i < SAMPLE_COUNT; i++)
  {
    sum += analogRead(PH_SENSOR_BUS) * (5.0f / 1023.0f);
    delay(SAMPLE_DELAY_MS);
  }

  return sum / SAMPLE_COUNT;
}

void drawVoltageScreen(float voltage)
{
  char voltageText[14];
  char deltaText[16];

  dtostrf(voltage, 4, 3, voltageText);
  dtostrf(voltage - TARGET_SHORT_VOLTAGE, 5, 3, deltaText);

  u8g2.firstPage();
  do
  {
    u8g2.setDrawColor(1);
    u8g2.setFontPosTop();
    u8g2.setFontDirection(0);

    u8g2.setFont(u8g2_font_6x13_tf);
    u8g2.drawStr(6, 0, "pH Calibration");
    u8g2.drawStr(6, 14, "Target short: 2.50V");

    u8g2.drawRFrame(4, 26, 120, 24, 5);
    u8g2.setFont(u8g2_font_logisoso16_tf);
    u8g2.drawStr(16, 30, voltageText);
    u8g2.setFont(u8g2_font_6x13_tf);
    u8g2.drawStr(90, 34, "V");

    u8g2.drawStr(6, 52, "Delta:");
    u8g2.drawStr(44, 52, deltaText);
    u8g2.drawStr(88, 52, "V");
  } while (u8g2.nextPage());
}

void printHelp()
{
  Serial.println(F("Commands:"));
  Serial.println(F("  r      -> Read averaged voltage"));
  Serial.println(F("  4      -> Capture acid buffer (pH 4.00)"));
  Serial.println(F("  7      -> Capture neutral buffer (pH 7.00)"));
  Serial.println(F("  10     -> Capture basic buffer (pH 10.00)"));
  Serial.println(F("  p      -> Toggle periodic reading stream"));
  Serial.println(F("  show   -> Show current calibration values"));
  Serial.println(F("  help   -> Show this help"));
  Serial.println();
}

void printCalibration()
{
  Serial.println(F("Current calibration voltages:"));
  Serial.print(F("  V_ACIDO  = "));
  Serial.println(vAcid, 4);
  Serial.print(F("  V_NEUTRO = "));
  Serial.println(vNeutral, 4);
  Serial.print(F("  V_BASICO = "));
  Serial.println(vBasic, 4);

  Serial.println();
  Serial.println(F("Copy to config.h:"));
  Serial.print(F("const float V_ACIDO = "));
  Serial.print(vAcid, 4);
  Serial.println(F(";"));
  Serial.print(F("const float V_NEUTRO = "));
  Serial.print(vNeutral, 4);
  Serial.println(F(";"));
  Serial.print(F("const float V_BASICO = "));
  Serial.print(vBasic, 4);
  Serial.println(F(";"));
  Serial.println();
}

void readAndPrintSample(const __FlashStringHelper *label)
{
  float voltage = readAverageVoltage();

  if (label != nullptr)
  {
    Serial.print(label);
  }

  Serial.print(F("Voltage: "));
  Serial.print(voltage, 4);
  Serial.println(F(" V"));

  drawVoltageScreen(voltage);
}

void captureBuffer(float &target)
{
  delay(800);
  target = readAverageVoltage();
}

void handleCommand(const String &command)
{
  if (command == "r")
  {
    readAndPrintSample(nullptr);
    return;
  }

  if (command == "4")
  {
    Serial.println(F("Place probe in pH 4.00 buffer and keep still..."));
    captureBuffer(vAcid);
    readAndPrintSample(F("Captured pH 4.00 -> "));
    printCalibration();
    return;
  }

  if (command == "7")
  {
    Serial.println(F("Place probe in pH 7.00 buffer and keep still..."));
    captureBuffer(vNeutral);
    readAndPrintSample(F("Captured pH 7.00 -> "));
    printCalibration();
    return;
  }

  if (command == "10")
  {
    Serial.println(F("Place probe in pH 10.00 buffer and keep still..."));
    captureBuffer(vBasic);
    readAndPrintSample(F("Captured pH 10.00 -> "));
    printCalibration();
    return;
  }

  if (command == "p")
  {
    streamReadings = !streamReadings;
    Serial.print(F("Periodic stream: "));
    Serial.println(streamReadings ? F("ON") : F("OFF"));
    return;
  }

  if (command == "show")
  {
    printCalibration();
    return;
  }

  if (command == "help")
  {
    printHelp();
    return;
  }

  Serial.println(F("Unknown command. Type 'help'"));
}
} // namespace

void setup()
{
  Serial.begin(115200);
  while (!Serial)
  {
    ;
  }

  u8g2.begin();

  Serial.println();
  Serial.println(F("=== pH Sensor Calibration Sketch ==="));
  Serial.println(F("1) For short adjustment, tune to 2.50V"));
  Serial.println(F("2) Use commands 4, 7, 10 to capture buffers"));
  Serial.println();

  printCalibration();
  printHelp();
  readAndPrintSample(F("Initial -> "));
}

void loop()
{
  if (streamReadings && (millis() - lastStreamMs) >= 1500)
  {
    lastStreamMs = millis();
    readAndPrintSample(nullptr);
  }

  if (Serial.available())
  {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();

    if (command.length() > 0)
    {
      handleCommand(command);
    }
  }
}

#endif
