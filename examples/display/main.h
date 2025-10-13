#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>


void testlines(uint16_t color);

void testdrawtext(char *text, uint16_t color);

void testfastlines(uint16_t color1, uint16_t color2);

void testdrawrects(uint16_t color);

void testfillrects(uint16_t color1, uint16_t color2);

void testfillcircles(uint8_t radius, uint16_t color);

void testdrawcircles(uint8_t radius, uint16_t color);


void testtriangles();

void testroundrects();

void tftPrintTest();

void mediabuttons();