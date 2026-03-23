#include "display.h"

#include <Arduino.h>
#include <U8g2lib.h>
#include <math.h>
#include <string.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#include "config.h"

const char degreeSymbol[] = "\xB0";

static U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);
static uint8_t shimmerOffset = 0;

static void u8g2_prepare();
static void drawTemperatureScreen(float tempC);
static void drawCenteredText(uint8_t x, uint8_t y, uint8_t w, const char *value);
static void drawThermometerIcon(uint8_t x, uint8_t y);
static void drawDecorativeWave();

void initDisplay()
{
  u8g2.begin();
}

void initAnimations()
{
  shimmerOffset = 0;
}

void updateAnimations()
{
  shimmerOffset = (uint8_t)((shimmerOffset + 1) % 16);
}

void drawScreen(float tempC)
{
  u8g2_prepare();
  u8g2.firstPage();
  do
  {
    drawTemperatureScreen(tempC);
  } while (u8g2.nextPage());
}

static void u8g2_prepare()
{
  u8g2.setFont(u8g2_font_spleen6x12_mf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
}

static void drawTemperatureScreen(float tempC)
{
  char tempText[12];
  bool hasValidTemp = !(isnan(tempC) || tempC <= -126.5f);

  if (hasValidTemp)
  {
    dtostrf(tempC, 4, 1, tempText);
    strcat(tempText, degreeSymbol);
  }
  else
  {
    strcpy(tempText, "--.-");
  }

  u8g2.drawRFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 8);
  u8g2.drawRFrame(4, 4, SCREEN_WIDTH - 8, SCREEN_HEIGHT - 8, 6);

  u8g2.setFont(u8g2_font_6x13_tf);
  u8g2.drawStr(36, 8, "AQUARIO");
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(40, 21, "TEMPERATURA");
  u8g2.drawHLine(30, 31, 68);

  drawThermometerIcon(14, 18);

  u8g2.setFont(u8g2_font_logisoso24_tf);
  drawCenteredText(36, 33, 86, tempText);

  u8g2.setFont(u8g2_font_5x7_tf);
  if (hasValidTemp)
  {
    u8g2.drawStr(38, 56, "Sensor OK");
  }
  else
  {
    u8g2.drawStr(34, 56, "Aguardando sensor");
  }

  drawDecorativeWave();
}

static void drawCenteredText(uint8_t x, uint8_t y, uint8_t w, const char *value)
{
  int textWidth = u8g2.getStrWidth(value);
  int textX = x + (w - textWidth) / 2;
  if (textX < x + 2)
  {
    textX = x + 2;
  }
  u8g2.drawStr(textX, y, value);
}

static void drawThermometerIcon(uint8_t x, uint8_t y)
{
  u8g2.drawCircle(x + 7, y + 26, 7);
  u8g2.drawCircle(x + 7, y + 26, 6);
  u8g2.drawRFrame(x + 4, y + 2, 6, 23, 3);
  u8g2.drawBox(x + 6, y + 10, 2, 15);
  u8g2.drawBox(x + 5, y + 21, 4, 4);
}

static void drawDecorativeWave()
{
  for (uint8_t x = 10; x < 118; x += 6)
  {
    uint8_t y = 61;
    if (((x + shimmerOffset) % 12) < 6)
    {
      y = 60;
    }
    u8g2.drawPixel(x, y);
  }
}
