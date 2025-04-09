#ifndef DISPLAY_H
#define DISPLAY_H


void initDisplay(bool SD_enable = true);
void drawSdJpeg(const char *filename, int xpos, int ypos);
void jpegRender(int xpos, int ypos);
void rotateColors();

#endif