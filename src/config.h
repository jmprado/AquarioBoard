#pragma once

#include <Arduino.h>

// Labels
extern const char labelTemp[];
extern const char labelPh[];
extern const char degreeSymbol[];

// Timing settings
const unsigned long SENSOR_UPDATE_INTERVAL = 10000; // 10 seconds for sensor readings
const unsigned long BUBBLE_UPDATE_INTERVAL = 100;   // 100ms for bubble animation

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Animation settings
#define MAX_BUBBLES 6
#define MAX_FISH 2

// Pin definitions
const uint8_t temp_sensor_1_bus = 12; // DS18B20 temperature sensor bus pin 

// pH sensor calibration and reading
#define PH_SENSOR_BUS A0

const float V_ACIDO = 3.7066;
const float V_NEUTRO = 3.3365;
const float V_BASICO = 2.9510;
