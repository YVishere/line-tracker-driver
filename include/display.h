#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

#define HEIGHT 320
#define WIDTH 480

#define ANIMATION_WIDTH 220
#define ANIMATION_HEIGHT 211
#define ANIMATION_FRAMES 3
#define ANIMATION_FRAME_SIZE (211*220)


extern const char* ANIM_FILEPATH;

void initDisplay(bool SD_enable = true);
void drawSdJpeg(const char *filename, int xpos, int ypos, bool centered = false);
void jpegRender(int xpos, int ypos);
void rotateColors();

uint8_t *initFrameHeap(int frameSize);

void freeFrameHeap(uint8_t *frameHeap);

void loadFrameIntoHeap(int startIndex, int heapSize, uint8_t *frameHeap);

void HeapAnim(bool clear_old = true);
void HeapDispFrame(int frameIndex);

void convertTextToBinary(const char* textFilePath, const char* binaryFilePath);

#endif