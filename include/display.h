#ifndef DISPLAY_H
#define DISPLAY_H

#include <SPI.h>

#include <FS.h>
#include <SD.h>

#include <TFT_eSPI.h>

// JPEG decoder library
#include <JPEGDecoder.h>

void initDisplay(bool SD_enable = true);
void drawSdJpeg(const char *filename, int xpos, int ypos);
void jpegRender(int xpos, int ypos);
void rotateColors();

#endif