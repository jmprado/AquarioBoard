#include "display.h"

#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#include "config.h"

const char labelTemp[] = "Temp:";
const char labelPh[] = "pH:";
const char degreeSymbol[] = "\xB0";

static U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

static void u8g2_prepare();
static void drawReadings(float tempC, float ph);
static void drawCenteredValue(uint8_t x, uint8_t y, uint8_t w, const char *value);
static void drawTempIcon(uint8_t x, uint8_t y);
static void drawPhIcon(uint8_t x, uint8_t y);

void initDisplay()
{
  u8g2.begin();
}

void initAnimations()
{
}

void updateAnimations()
{
}

void drawScreen(float tempC, float ph)
{
  u8g2_prepare();
  u8g2.firstPage();
  do
  {
    drawReadings(tempC, ph);
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

static void drawReadings(float tempC, float ph)
{
  char buffer_temp[10];
  char buffer_ph[10];

  dtostrf(tempC, 4, 1, buffer_temp);
  dtostrf(ph, 4, 1, buffer_ph);
  strcat(buffer_temp, degreeSymbol);

  u8g2.drawRFrame(0, 0, 64, SCREEN_HEIGHT, 6);
  u8g2.drawRFrame(64, 0, 64, SCREEN_HEIGHT, 6);
  u8g2.drawHLine(8, 14, 48);
  u8g2.drawHLine(72, 14, 48);

  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(12, 4, labelTemp);
  u8g2.drawStr(80, 4, labelPh);

  drawTempIcon(3, 2);
  drawPhIcon(67, 2);

  u8g2.setFont(u8g2_font_logisoso20_tf);
  drawCenteredValue(0, 25, 64, buffer_temp);
  drawCenteredValue(64, 25, 64, buffer_ph);
}

static void drawCenteredValue(uint8_t x, uint8_t y, uint8_t w, const char *value)
{
  int textWidth = u8g2.getStrWidth(value);
  int textX = x + (w - textWidth) / 2;
  if (textX < x + 2)
  {
    textX = x + 2;
  }
  u8g2.drawStr(textX, y, value);
}

static void drawTempIcon(uint8_t x, uint8_t y)
{
  u8g2.drawCircle(x + 3, y + 8, 2);
  u8g2.drawBox(x + 2, y + 2, 2, 6);
  u8g2.drawFrame(x + 1, y + 1, 4, 8);
}

static void drawPhIcon(uint8_t x, uint8_t y)
{
  u8g2.drawPixel(x + 4, y + 1);
  u8g2.drawLine(x + 3, y + 2, x + 5, y + 2);
  u8g2.drawLine(x + 2, y + 3, x + 6, y + 3);
  u8g2.drawLine(x + 2, y + 4, x + 6, y + 4);
  u8g2.drawLine(x + 3, y + 5, x + 5, y + 5);
}
