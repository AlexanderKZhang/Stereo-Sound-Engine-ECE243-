#include "vga.h"

#include "address_map.h"
#include "greyCircle.h"

// padding added to buffers in the x direction
#define buffer1 0x08000000
#define buffer2 0x08040000

// short Buffer1[240][320] __attribute__((section(".vga_front_buffer")));
// short Buffer2[240][320] __attribute__((section(".vga_back_buffer")));

int* vgaSetup(unsigned int VGABaseAddress) {
  // set the address to the buffer base
  int* vgaBase = (int*)VGABaseAddress;

  unsigned int resolution = (GREYCIRCLE_HEIGHT << 16) + GREYCIRCLE_WIDTH;
  vgaBase[2] = resolution;

  // set buffer1 to be front buffer
  vgaBase[1] = buffer1;
  clearScreen(buffer1);
  waitForSync(vgaBase);

  // set buffer2 to be back buffer
  vgaBase[1] = buffer2;
  clearScreen(buffer2);
  waitForSync(vgaBase);

  return vgaBase;
}

// initiate buffer swap process and wait for the buffer to finish
void waitForSync(volatile int* VGABase) {
  // write 1 into front buffer reg to initiate the swap
  VGABase[0] = 1;

  int status = VGABase[3];
  while ((status & 1) != 0) {  // poll the status bit until switch is completed
    status = VGABase[3];
  }
}

// draws a specific colour into a pixel in the back buffer at the specified x
// and y coordinates provided
void drawPixel(volatile int backBufferAddress, int x, int y, short colour) {
  // y byte coordinates stored starting from 10, x coordinates byte stored starting from bit 1
  volatile short* pixelAddress = (volatile short*)(backBufferAddress + (y << 10) + (x << 1));
  *(pixelAddress) = colour;
}

// draws the grey circle background into the back buffer
void clearScreen(volatile int backBufferAddress) {
  for (int x = 0; x < GREYCIRCLE_WIDTH; x++) {
    for (int y = 0; y < GREYCIRCLE_HEIGHT; y++) {
      drawPixel(backBufferAddress, x, y, greyCircle[GREYCIRCLE_WIDTH * y + x]);
    }
  }
}

// draws a ball of radius 2 at at the specified x,y coordinate
void drawBall(volatile int backBufferAddress, int x, int y, short colour) {
  for (int i = x - 2; i < x + 3; i++) {
    if (i == x - 2 || i == x + 2) {
      for (int j = y - 1; j < y + 2; j++) {
        if (i >= 0 && i < GREYCIRCLE_WIDTH && j >= 0 && j < GREYCIRCLE_HEIGHT) {
          drawPixel(backBufferAddress, i, j, colour);
        }
      }
    } else {
      for (int j = y - 2; j < y + 3; j++) {
        if (i >= 0 && i < GREYCIRCLE_WIDTH && j >= 0 && j < GREYCIRCLE_HEIGHT) {
          drawPixel(backBufferAddress, i, j, colour);
        }
      }
    }
  }
}

// undraws the ball by drawing the background over the specified x,y coordinates
void undrawBall(volatile int backBufferAddress, int x, int y) {
  for (int i = x - 2; i < x + 3; i++) {
    if (i == x - 2 || i == x + 2) {
      for (int j = y - 1; j < y + 2; j++) {
        if (i >= 0 && i < GREYCIRCLE_WIDTH && j >= 0 && j < GREYCIRCLE_HEIGHT) {
          drawPixel(backBufferAddress, i, j, greyCircle[GREYCIRCLE_WIDTH * j + i]);
        }
      }
    } else {
      for (int j = y - 2; j < y + 3; j++) {
        if (i >= 0 && i < GREYCIRCLE_WIDTH && j >= 0 && j < GREYCIRCLE_HEIGHT) {
          drawPixel(backBufferAddress, i, j, greyCircle[GREYCIRCLE_WIDTH * j + i]);
        }
      }
    }
  }
}

// Draws a rectangle 10 pixels wide, growing UPWARDS from bottomY
void drawVolumeBar(volatile int backBufferAddress, int startX, int bottomY, int height, short colour) {
  for (int x = startX; x < startX + 10; x++) { // Bar is 10 pixels wide
    for (int y = bottomY; y > bottomY - height; y--) { // Drawing upwards
      if (x >= 0 && x < GREYCIRCLE_WIDTH && y >= 0 && y < GREYCIRCLE_HEIGHT) {
        drawPixel(backBufferAddress, x, y, colour);
      }
    }
  }
}

// Restores the background where the bar used to be
void undrawVolumeBar(volatile int backBufferAddress, int startX, int bottomY, int height) {
  for (int x = startX; x < startX + 10; x++) {
    for (int y = bottomY; y > bottomY - height; y--) {
      if (x >= 0 && x < GREYCIRCLE_WIDTH && y >= 0 && y < GREYCIRCLE_HEIGHT) {
        drawPixel(backBufferAddress, x, y, greyCircle[GREYCIRCLE_WIDTH * y + x]);
      }
    }
  }
}

// Draws a 2-pixel thick horizontal line at the peak height
void drawPeakLine(volatile int backBufferAddress, int startX, int bottomY, int peakHeight, short colour) {
  int y = bottomY - peakHeight;
  
  for (int x = startX; x < startX + 10; x++) {
    for (int dy = 0; dy < 2; dy++) { // 2 pixels thick
      int drawY = y - dy;
      if (x >= 0 && x < GREYCIRCLE_WIDTH && drawY >= 0 && drawY < GREYCIRCLE_HEIGHT) {
        drawPixel(backBufferAddress, x, drawY, colour);
      }
    }
  }
}

// Restores the background where the peak line used to be
void undrawPeakLine(volatile int backBufferAddress, int startX, int bottomY, int peakHeight) {
  int y = bottomY - peakHeight;
  
  for (int x = startX; x < startX + 10; x++) {
    for (int dy = 0; dy < 2; dy++) {
      int drawY = y - dy;
      if (x >= 0 && x < GREYCIRCLE_WIDTH && drawY >= 0 && drawY < GREYCIRCLE_HEIGHT) {
        drawPixel(backBufferAddress, x, drawY, greyCircle[GREYCIRCLE_WIDTH * drawY + x]);
      }
    }
  }
}

// provided function that writes the character displayed in the character buffer
void video_text(int x, int y, char * text_ptr) {
  int offset;
  volatile char * character_buffer =
  (char *)FPGA_CHAR_BASE; // video character buffer
  /* assume that the text string fits on one line */
  offset = (y << 7) + x;
  while (*(text_ptr)) {
    *(character_buffer + offset) =
    *(text_ptr); // write to the character buffer
    ++text_ptr;
    ++offset;
  }
}

// Note: 'buffer' must be an array of at least 4 characters to hold the 3 digits + null terminator
void intToStr(int num, char* string) {
  // Cap the number to prevent overflowing the 3 digits
  if (num > 999) num = 999;

  // Extract each digit mathematically and add '0' (0x30) to convert it to ASCII
  string[0] = abs((num / 100)) + '0';           // The hundreds place
  string[1] = abs(((num / 10) % 10)) + '0';     // The tens place
  string[2] = abs((num % 10)) + '0';            // The ones place
  
  // Always end C-strings with a null terminator!
  string[3] = '\0'; 
}

int abs(int num) {
  if (num < 0) return -num;
  return num;
}