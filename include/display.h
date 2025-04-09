#ifndef DISPLAY_H
#define DISPLAY_H

#include <SPI.h>

#include <FS.h>
#include <SD.h>

#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

// JPEG decoder library
#include <JPEGDecoder.h>

#define TFT_LENGTH      480
#define TFT_WIDTH       320

void initDisplay(bool SD_enable = true);
void drawSdJpeg(const char *filename, int xpos, int ypos);
void jpegRender(int xpos, int ypos);

#endif