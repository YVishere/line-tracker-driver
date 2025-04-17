#include "display.h"
#include <Arduino.h>

#include <SPI.h>

#include <FS.h>
#include <SD.h>

#include <TFT_eSPI.h>

// JPEG decoder library
#include <JPEGDecoder.h>

const char* ANIM_FILEPATH = "/bsr/som.bin";

volatile int old_start_x = 0;
volatile int old_start_y = 0;
volatile int old_width = 0;
volatile int old_height = 0;

TFT_eSPI tft = TFT_eSPI();

//Heap pointer for animation frames
uint8_t *frameHeap = NULL;

//Array of random colors to cycle through
uint16_t colors[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW, TFT_CYAN, TFT_MAGENTA};

// Global file handle
File animFile;
bool animFileOpen = false;

void rotateColors() {
  static uint8_t colorIndex = 0;
  tft.fillScreen(colors[colorIndex]);
  colorIndex = (colorIndex + 1) % (sizeof(colors) / sizeof(colors[0]));
}

TFT_eSPI tft = TFT_eSPI();

void initDisplay(bool SD_enable){
    // Set all chip selects high to avoid bus contention during initialisation of each peripheral
    digitalWrite(22, HIGH); // Touch controller chip select (if used)
    digitalWrite(15, HIGH); // TFT screen chip select
    digitalWrite( 5, HIGH); // SD card chips select, must use GPIO 5 (ESP32 SS)

    tft.fillScreen(TFT_BLACK);

    tft.begin();
    tft.setRotation(1);  // Somehow important to have an odd rotation number
                          //Display does not work properly if you do not set this and initialise the SD card
    tft.fillScreen(TFT_BLACK);
    
    if (!SD.begin(5, tft.getSPIinstance())) {
      Serial.println("Card Mount Failed");
      return;
    }
    uint8_t cardType = SD.cardType();
  
    if (cardType == CARD_NONE) {
      Serial.println("No SD card attached");
      return;
    }
  
    Serial.print("SD Card Type: ");
    if (cardType == CARD_MMC) {
      Serial.println("MMC");
    } else if (cardType == CARD_SD) {
      Serial.println("SDSC");
    } else if (cardType == CARD_SDHC) {
      Serial.println("SDHC");
    } else {
      Serial.println("UNKNOWN");
    }

    // Convert the text animation to binary format (only needed once)
    if (!SD.exists("/bsr/som.bin")) {
      Serial.println("Creating binary animation file...");
      convertTextToBinary("/bsr/som.txt", "/bsr/som.bin");
    }
  
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
  
    Serial.println("initialisation done.");
}

void drawSdJpeg(const char *filename, int xpos, int ypos, bool centered) {
  File jpegFile = SD.open(filename);
  if (!jpegFile) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.println("===========================");
  Serial.print("Drawing file: "); Serial.println(filename);
  Serial.println("===========================");

  // If centered, we need to get dimensions first
  if (centered) {
    bool pre_decoded = JpegDec.decodeSdFile(jpegFile);
    if (pre_decoded) {
      // Get image dimensions
      uint32_t image_width = JpegDec.width;
      uint32_t image_height = JpegDec.height;
      
      // Calculate centered position
      xpos = (TFT_WIDTH - image_width) / 2;
      ypos = (TFT_HEIGHT - image_height) / 2;
      
      // Ensure position isn't negative
      xpos = max(0, xpos);
      ypos = max(0, ypos);
      
      // Close and reopen for actual rendering
      jpegFile.close();
      jpegFile = SD.open(filename);
    }
  }

  // Use one of the following methods to initialise the decoder:
  bool decoded = JpegDec.decodeSdFile(jpegFile);

  if (decoded) {
    // render the image onto the screen at given coordinates
    jpegRender(xpos, ypos);
  }
  else {
    Serial.println("Jpeg file format not supported!");
  }
}

void jpegRender(int xpos, int ypos) {

    //jpegInfo(); // Print information from the JPEG file (could comment this line out)
  
    uint16_t *pImg;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t max_x = JpegDec.width;
    uint32_t max_y = JpegDec.height;
  
    bool swapBytes = tft.getSwapBytes();
    tft.setSwapBytes(true);
    
    // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
    // Typically these MCUs are 16x16 pixel blocks
    // Determine the width and height of the right and bottom edge image blocks
    uint32_t min_w = jpg_min(mcu_w, max_x % mcu_w);
    uint32_t min_h = jpg_min(mcu_h, max_y % mcu_h);
  
    // save the current image block size
    uint32_t win_w = mcu_w;
    uint32_t win_h = mcu_h;
  
    // record the current time so we can measure how long it takes to draw an image
    uint32_t drawTime = millis();
  
    // save the coordinate of the right and bottom edges to assist image cropping
    // to the screen size
    max_x += xpos;
    max_y += ypos;
  
    // Fetch data from the file, decode and display
    while (JpegDec.read()) {    // While there is more data in the file
      pImg = JpegDec.pImage ;   // Decode a MCU (Minimum Coding Unit, typically a 8x8 or 16x16 pixel block)
  
      // Calculate coordinates of top left corner of current MCU
      int mcu_x = JpegDec.MCUx * mcu_w + xpos;
      int mcu_y = JpegDec.MCUy * mcu_h + ypos;
  
      // check if the image block size needs to be changed for the right edge
      if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
      else win_w = min_w;
  
      // check if the image block size needs to be changed for the bottom edge
      if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
      else win_h = min_h;
  
      // copy pixels into a contiguous block
      if (win_w != mcu_w)
      {
        uint16_t *cImg;
        int p = 0;
        cImg = pImg + win_w;
        for (int h = 1; h < win_h; h++)
        {
          p += mcu_w;
          for (int w = 0; w < win_w; w++)
          {
            *cImg = *(pImg + w + p);
            cImg++;
          }
        }
      }
  
      // calculate how many pixels must be drawn
      uint32_t mcu_pixels = win_w * win_h;
  
      // draw image MCU block only if it will fit on the screen
      if (( mcu_x + win_w ) <= tft.width() && ( mcu_y + win_h ) <= tft.height())
        tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
      else if ( (mcu_y + win_h) >= tft.height())
        JpegDec.abort(); // Image has run off bottom of screen so abort decoding
    }
  
    tft.setSwapBytes(swapBytes);
  }


uint8_t *initFrameHeap(int frameSize) {
    // Allocate memory for the frame heap
    uint8_t *frameHeap = (uint8_t *)malloc(frameSize * sizeof(uint8_t));
    if (frameHeap == NULL) {
        Serial.println("Failed to allocate memory for frame heap");
        return NULL;
    }
    return frameHeap;
}  

void freeFrameHeap(uint8_t *frameHeap) {
    // Free the allocated memory for the frame heap
    if (frameHeap != NULL) {
        free(frameHeap);
        Serial.println("Frame heap memory freed successfully");
    } else {
        Serial.println("Frame heap is already NULL, no need to free memory");
    }
}

// Improved animation loading with block reading for RGB332 format
void HeapAnim(bool clear_old) {
  if (clear_old) {
    tft.fillRect(old_start_x, old_start_y, old_width, old_height, TFT_BLACK);
  }

  // Initialize heap only once - Use 1 byte per pixel for RGB332 format
  if (frameHeap == NULL) {
    frameHeap = initFrameHeap(ANIMATION_WIDTH * ANIMATION_HEIGHT); // RGB332 is 1 byte per pixel
    if (frameHeap == NULL) {
      Serial.println("Failed to allocate memory for frame heap");
      return;
    }
  }

  // Open file once for the entire animation
  if (!animFileOpen) {
    animFile = SD.open(ANIM_FILEPATH, FILE_READ);
    if (!animFile) {
      Serial.println("Animation file does not exist!");
      // Try to convert if binary doesn't exist
      if (SD.exists("/bsr/som.txt") && !SD.exists("/bsr/som.bin")) {
        Serial.println("Trying to convert text file to binary...");
        convertTextToBinary("/bsr/som.txt", "/bsr/som.bin");
        animFile = SD.open(ANIM_FILEPATH, FILE_READ);
        if (!animFile) {
          Serial.println("Still couldn't open animation file after conversion");
          return;
        }
      } else {
        return;
      }
    }
    animFileOpen = true;
    Serial.println("Animation file opened successfully");
    Serial.printf("Animation file size: %d bytes\n", animFile.size());
  }

  uint32_t startTime = millis();
  
  for (int i = 0; i < 14; i++) {
    uint32_t frameStart = millis();
    HeapDispFrame(i);
    Serial.printf("Frame %d timing: %d ms\n", i, millis() - frameStart);
    delay(42);  // Target ~24fps
  }
  
  Serial.printf("Total animation time: %d ms\n", millis() - startTime);
  
  // Close file when animation completes
  animFile.close();
  animFileOpen = false;
  Serial.println("Animation file closed");
}

void HeapDispFrame(int frameIndex) {
  // Seek to the correct position instead of re-opening
  // For RGB332 format (1 byte per pixel)
  uint32_t frameOffset = frameIndex * ANIMATION_WIDTH * ANIMATION_HEIGHT;
  
  if (animFileOpen) {
    animFile.seek(frameOffset);
    
    // Block reading - now for RGB332 (1 byte per pixel)
    uint32_t bytesRead = animFile.read(frameHeap, ANIMATION_WIDTH * ANIMATION_HEIGHT);
    
    if (bytesRead != ANIMATION_WIDTH * ANIMATION_HEIGHT) {
      Serial.printf("Error reading frame: read %d bytes\n", bytesRead);
    }
    
    // Calculate position to center
    int start_x = (WIDTH - ANIMATION_WIDTH) / 2;
    int start_y = (HEIGHT - ANIMATION_HEIGHT) / 2;
    
    // Display the frame - RGB332 format
    tft.pushImage(start_x, start_y, ANIMATION_WIDTH, ANIMATION_HEIGHT, frameHeap, true); 
    // The last parameter 'true' tells the library this is RGB332 format
    
    // Store coordinates for clearing in next frame
    old_start_x = start_x;
    old_start_y = start_y;
    old_width = ANIMATION_WIDTH;
    old_height = ANIMATION_HEIGHT;
  }
}

// You can keep your loadFrameIntoHeap function as a backward compatibility option
// if you still need to support text files, but it will be much slower
void loadFrameIntoHeap(int startIndex, int heapSize, uint8_t *frameHeap) {
    // Open the SD card file for reading
    File file = SD.open("/bsr/som.txt", FILE_READ);
    if (SD.exists(ANIM_FILEPATH) == false) {
        Serial.println("Animation file does not exist!");
        return;
    }
    if (!file) {
        Serial.println("Failed to open animation file");
        return;
    }

    // Read the file line by line and store the values in the frame heap
    int index = 0;
    while (file.available()) {
        String line = file.readStringUntil('\n');
        if (index >= startIndex && index < startIndex + heapSize) {
            frameHeap[index - startIndex] = (uint8_t)line.toInt();
        }
        if (index >= startIndex + heapSize) {
            break; // Stop reading if we have filled the heap
        }
        index++;
    }

    // Close the file
    file.close();
}

// Updated converter for RGB332 format (1 byte per pixel)
void convertTextToBinary(const char* textFilePath, const char* binaryFilePath) {
  File textFile = SD.open(textFilePath, FILE_READ);
  if (!textFile) {
    Serial.println("Failed to open text file");
    return;
  }
  
  File binaryFile = SD.open(binaryFilePath, FILE_WRITE);
  if (!binaryFile) {
    Serial.println("Failed to create binary file");
    textFile.close();
    return;
  }
  
  uint8_t pixel; // Use 8-bit value for RGB332
  int count = 0;
  
  while (textFile.available()) {
    String line = textFile.readStringUntil('\n');
    pixel = (uint8_t)line.toInt(); // Only need a single byte
    binaryFile.write(pixel);
    count++;
  }
  
  textFile.close();
  binaryFile.close();
  
  Serial.printf("Converted %d pixels to RGB332 binary format\n", count);
}
