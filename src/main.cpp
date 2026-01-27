// RTC DS3231 Lib
#include <DS3231.h>
DS3231 rtc;
RTCDateTime rtcDateTime;

// Temp sensor lib
#include <OneWire.h>
#include <DallasTemperature.h>

// LCD 128x64 Lib
#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, /* clock=*/13, /* data=*/11, /* CS=*/12, /* reset=*/8);

// Temp sensor #1
#define TEMP_SENSOR_1_BUS 2
OneWire oneWire_1(TEMP_SENSOR_1_BUS);
DallasTemperature sensors(&oneWire_1);

const char labelTemp[] = "Temp:";
const char labelPh[] = "pH:";
const char degreeSymbol[] = "\xB0";

// Timing settings
const unsigned long SENSOR_UPDATE_INTERVAL = 10000; // 10 seconds for sensor readings
const unsigned long BUBBLE_UPDATE_INTERVAL = 100;   // 100ms for bubble animation

unsigned long lastSensorUpdate = 0;
unsigned long lastBubbleUpdate = 0;

// Bubble animation settings
#define MAX_BUBBLES 6
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

struct Bubble
{
  float x;
  float y;
  float speed;
  uint8_t radius;
};

Bubble bubbles[MAX_BUBBLES];

// Fish animation settings
#define MAX_FISH 2

struct Fish
{
  float x;
  float y;
  float speed;
  int8_t direction;
  float swimOffset;
};

Fish fishes[MAX_FISH];

// Pin definitions
const uint8_t relay_1_bus = 2;
const uint8_t relay_2_bus = 3;
const uint8_t button_pin = 4;

// Relay override state
bool relay1Override = false;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Function prototypes
float readTempSensor1(void);
float readPhSensor(void);
void initBubbles(void);
void updateBubbles(void);
void drawBubbles(void);
void initFish(void);
void updateFish(void);
void drawFish(void);
void drawPlants(void);
void printDate(void);
int getTime(void);
void u8g2_prepare(void);
void handleButton(void);

void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
  }

  // Initialize hardware first
  pinMode(relay_1_bus, OUTPUT);
  pinMode(relay_2_bus, OUTPUT);
  pinMode(button_pin, INPUT_PULLUP);

  digitalWrite(relay_1_bus, LOW);
  digitalWrite(relay_2_bus, LOW);

  randomSeed(analogRead(A1));

  sensors.begin();
  u8g2.begin();
  rtc.begin();

  // Initialize animations after hardware
  initBubbles();
  initFish();

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

  // Get current time and control relay_1 (10:00 to 18:00)
  rtcDateTime = rtc.getDateTime();
  int currentTime = rtcDateTime.hour * 100 + rtcDateTime.minute;

  // Apply override or time-based control
  if (relay1Override)
  {
    digitalWrite(relay_1_bus, HIGH);
  }
  else if (currentTime >= 1000 && currentTime < 1800)
  {
    digitalWrite(relay_1_bus, HIGH);
  }
  else
  {
    digitalWrite(relay_1_bus, LOW);
  }

  // Update sensor readings every 10 seconds
  if (currentMillis - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL)
  {
    lastSensorUpdate = currentMillis;

    temp1 = readTempSensor1();
    ph = readPhSensor();

    Serial.print(F("Temp: "));
    Serial.print(temp1);
    Serial.print(F(" - pH: "));
    Serial.println(ph);
    printDate();

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
    updateBubbles();
    updateFish();
    needsRedraw = true;
  }

  // Only redraw screen when something changed
  if (needsRedraw)
  {
    // Prepare display buffers
    char buffer_temp[8];
    char buffer_ph[8];
    dtostrf(temp1, 4, 1, buffer_temp);
    dtostrf(ph, 4, 1, buffer_ph);
    strcat(buffer_temp, degreeSymbol);

    // Draw everything using page mode
    u8g2_prepare();
    u8g2.firstPage();
    do
    {
      // Draw aquarium scene
      drawPlants();
      drawBubbles();
      drawFish();

      // Draw sensor data
      u8g2.drawStr(0, 0, labelTemp);
      u8g2.drawStr(32, 0, buffer_temp);
      u8g2.drawStr(0, 12, labelPh);
      u8g2.drawStr(14, 12, buffer_ph);

      // Show override indicator
      if (relay1Override)
      {
        u8g2.drawStr(90, 0, "[OVR]");
      }
    } while (u8g2.nextPage());
  }
}

// Handle button press with debouncing
void handleButton()
{
  bool reading = digitalRead(button_pin);

  if (reading != lastButtonState)
  {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (reading == LOW && lastButtonState == HIGH)
    {
      relay1Override = !relay1Override;

      Serial.print(F("Relay Override: "));
      Serial.println(relay1Override ? F("ON") : F("OFF"));
    }
  }

  lastButtonState = reading;
}

// LCD start up
void u8g2_prepare(void)
{
  u8g2.setFont(u8g2_font_spleen6x12_mf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

// Temperature reading #1
float readTempSensor1(void)
{
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

// pH sensor calibration and reading
#define PH_SENSOR_BUS A0

const float V_ACIDO = 3.75;
const float V_NEUTRO = 3.25;
const float V_BASICO = 2.80;

float phSensorAvgVoltage(void);
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);

float readPhSensor()
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

  phCalculado = phCalculado + ((25.0 - temp1) * 0.03);

  return phCalculado;
}

float phSensorAvgVoltage(void)
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

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Initialize bubbles
void initBubbles()
{
  for (uint8_t i = 0; i < MAX_BUBBLES; i++)
  {
    bubbles[i].x = random(100, 120);
    bubbles[i].y = SCREEN_HEIGHT + random(0, 20);
    bubbles[i].speed = random(5, 15) / 10.0;
    bubbles[i].radius = random(1, 4);
  }
}

// Update bubble positions
void updateBubbles()
{
  for (uint8_t i = 0; i < MAX_BUBBLES; i++)
  {
    bubbles[i].y -= bubbles[i].speed;
    bubbles[i].x += sin(bubbles[i].y / 10.0) * 0.3;

    if (bubbles[i].y < -bubbles[i].radius)
    {
      bubbles[i].y = SCREEN_HEIGHT + random(0, 20);
      bubbles[i].x = random(100, 120);
      bubbles[i].speed = random(5, 15) / 10.0;
      bubbles[i].radius = random(1, 4);
    }

    if (bubbles[i].x < 0)
      bubbles[i].x = 0;
    if (bubbles[i].x > SCREEN_WIDTH)
      bubbles[i].x = SCREEN_WIDTH;
  }
}

// Draw bubbles on screen
void drawBubbles()
{
  for (uint8_t i = 0; i < MAX_BUBBLES; i++)
  {
    if (bubbles[i].y >= 0 && bubbles[i].y < SCREEN_HEIGHT)
    {
      u8g2.drawCircle((int)bubbles[i].x, (int)bubbles[i].y, bubbles[i].radius);

      if (bubbles[i].radius > 1)
      {
        u8g2.drawPixel((int)bubbles[i].x - 1, (int)bubbles[i].y - 1);
      }
    }
  }
}

// Draw aquarium plants at the bottom
void drawPlants()
{
  static unsigned long lastMillis = 0;
  static float timeOffset = 0;

  unsigned long currentMillis = millis();
  if (currentMillis != lastMillis)
  {
    lastMillis = currentMillis;
    timeOffset = currentMillis / 1000.0;
  }

  int sway1 = (int)(sin(timeOffset * 1.5) * 2);
  int sway2 = (int)(sin(timeOffset * 2.0) * 3);
  int sway3 = (int)(sin(timeOffset * 1.8 + 1) * 2);
  int sway4 = (int)(sin(timeOffset * 2.5) * 1);

  // Plant 1 - Left side
  const int plant1X = 10;
  const int plant1BaseY = SCREEN_HEIGHT - 1;
  for (uint8_t i = 0; i < 3; i++)
  {
    int offsetX = plant1X + (i * 3);
    u8g2.drawLine(offsetX, plant1BaseY, offsetX - 2 + sway1, plant1BaseY - 15);
    u8g2.drawLine(offsetX, plant1BaseY, offsetX + 2 + sway1, plant1BaseY - 12);
  }

  // Plant 2 - Center-left
  const int plant2X = 35;
  const int plant2BaseY = SCREEN_HEIGHT - 1;
  u8g2.drawLine(plant2X, plant2BaseY, plant2X + (sway2 / 2), plant2BaseY - 18);
  u8g2.drawLine(plant2X, plant2BaseY - 10, plant2X - 5 + sway2, plant2BaseY - 15);
  u8g2.drawLine(plant2X, plant2BaseY - 10, plant2X + 5 + sway2, plant2BaseY - 15);
  u8g2.drawLine(plant2X, plant2BaseY - 5, plant2X - 4 + (sway2 / 2), plant2BaseY - 8);
  u8g2.drawLine(plant2X, plant2BaseY - 5, plant2X + 4 + (sway2 / 2), plant2BaseY - 8);

  // Plant 3 - Center-right
  const int plant3X = 75;
  const int plant3BaseY = SCREEN_HEIGHT - 1;
  for (uint8_t i = 0; i < 2; i++)
  {
    int offsetX = plant3X + (i * 4);
    u8g2.drawLine(offsetX, plant3BaseY, offsetX - 1 + sway3, plant3BaseY - 8);
    u8g2.drawLine(offsetX - 1 + sway3, plant3BaseY - 8, offsetX + 1 + sway3 * 2, plant3BaseY - 14);
    u8g2.drawLine(offsetX + 1 + sway3 * 2, plant3BaseY - 14, offsetX + sway3 * 2, plant3BaseY - 20);
  }

  // Plant 4 - Right side
  const int plant4X = 105;
  const int plant4BaseY = SCREEN_HEIGHT - 1;
  for (uint8_t i = 0; i < 4; i++)
  {
    int offsetX = plant4X + (i * 2);
    u8g2.drawLine(offsetX, plant4BaseY, offsetX + (i % 2 ? -1 : 1) + sway4, plant4BaseY - 10);
  }

  // Rocks
  u8g2.drawCircle(25, SCREEN_HEIGHT - 2, 2);
  u8g2.drawCircle(55, SCREEN_HEIGHT - 3, 3);
  u8g2.drawCircle(95, SCREEN_HEIGHT - 2, 2);
  u8g2.drawCircle(118, SCREEN_HEIGHT - 3, 2);
}

// Initialize fish
void initFish()
{
  fishes[0].x = random(-20, 0);
  fishes[0].y = 28;
  fishes[0].speed = 0.4;
  fishes[0].direction = 1;
  fishes[0].swimOffset = 0;

  fishes[1].x = random(SCREEN_WIDTH, SCREEN_WIDTH + 20);
  fishes[1].y = 42;
  fishes[1].speed = 0.3;
  fishes[1].direction = -1;
  fishes[1].swimOffset = 0;
}

// Update fish positions
void updateFish()
{
  for (uint8_t i = 0; i < MAX_FISH; i++)
  {
    fishes[i].x += fishes[i].speed * fishes[i].direction;
    fishes[i].y += sin(fishes[i].x / 15.0) * 0.15;
    fishes[i].swimOffset += 0.3;

    if (fishes[i].direction > 0 && fishes[i].x > SCREEN_WIDTH + 20)
    {
      fishes[i].x = -20;
      fishes[i].y = random(20, 50);
      fishes[i].speed = random(2, 6) / 10.0;
    }
    else if (fishes[i].direction < 0 && fishes[i].x < -20)
    {
      fishes[i].x = SCREEN_WIDTH + 20;
      fishes[i].y = random(20, 50);
      fishes[i].speed = random(2, 6) / 10.0;
    }

    if (fishes[i].y < 20)
      fishes[i].y = 20;
    if (fishes[i].y > 50)
      fishes[i].y = 50;
  }
}

// Draw fish
void drawFish()
{
  for (uint8_t i = 0; i < MAX_FISH; i++)
  {
    int fishX = (int)fishes[i].x;
    int fishY = (int)fishes[i].y;
    int8_t dir = fishes[i].direction;

    int tailBaseX = fishX - (3 * dir);
    int tailEndX = fishX - (6 * dir);
    int tailWag = (int)(sin(fishes[i].swimOffset) * 2);
    int tailTop = fishY - 2 + tailWag;
    int tailBottom = fishY + 2 + tailWag;

    int leftmost = (dir > 0) ? tailEndX : fishX - 3;
    int rightmost = (dir > 0) ? fishX + 4 : tailEndX;

    if (leftmost >= 0 && rightmost < SCREEN_WIDTH &&
        fishY >= 3 && fishY < SCREEN_HEIGHT - 3 &&
        tailTop >= 0 && tailTop < SCREEN_HEIGHT &&
        tailBottom >= 0 && tailBottom < SCREEN_HEIGHT)
    {
      u8g2.drawCircle(fishX, fishY, 3);
      u8g2.drawCircle(fishX + dir, fishY, 3);
      u8g2.drawPixel(fishX, fishY);
      u8g2.drawLine(tailBaseX, fishY, tailEndX, tailTop);
      u8g2.drawLine(tailBaseX, fishY, tailEndX, tailBottom);
      u8g2.drawPixel(fishX + (2 * dir), fishY - 1);
    }
  }
}

void printDate(void)
{
  rtcDateTime = rtc.getDateTime();
  Serial.print(rtcDateTime.year);
  Serial.print(F("-"));
  Serial.print(rtcDateTime.month);
  Serial.print(F("-"));
  Serial.print(rtcDateTime.day);
  Serial.print(F(" "));
  Serial.print(rtcDateTime.hour);
  Serial.print(F(":"));
  Serial.print(rtcDateTime.minute);
  Serial.print(F(":"));
  Serial.print(rtcDateTime.second);
  Serial.println();
}

int getTime(void)
{
  rtcDateTime = rtc.getDateTime();
  return rtcDateTime.hour * 100 + rtcDateTime.minute;
}