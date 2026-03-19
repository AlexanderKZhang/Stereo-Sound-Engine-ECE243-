#include "vga.h"

#include "address_map.h"

// padding added to buffers in the x direction
short buffer1[240][512];
short buffer2[240][512];

int* vgaSetup(unsigned int VGABaseAddress) {
  // set the address to the buffer base
  int* vgaBase = (int*)VGABaseAddress;
  volatile int backBufferAddress;

  // set buffer1 to be front buffer
  vgaBase[1] = (int)&buffer1;
  backBufferAddress = vgaBase[1];
  clearScreen(backBufferAddress);
  waitForSync(vgaBase);

  // set buffer2 to be back buffer
  vgaBase[1] = (int)&buffer2;
  backBufferAddress = vgaBase[1];
  clearScreen(backBufferAddress);
  waitForSync(vgaBase);

  return vgaBase;
}

void vgaDriver(volatile int* VGABase, int mouseX, int mouseY) {
  volatile int backBufferAddress = VGABase[1];
  drawBall(backBufferAddress, mouseX, mouseY, (short)WHITE);
  waitForSync(VGABase);
  backBufferAddress = VGABase[1];
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
  volatile short* pixelAddress =
      (volatile short*)(backBufferAddress + (y << 10) + (x << 1));
  *(pixelAddress) = colour;
}

// clear the back buffer by drawing black into each pixel
void clearScreen(volatile int backBufferAddress) {
  for (int x = 0; x < 320; x++) {
    for (int y = 0; y < 240; y++) {
      drawPixel(backBufferAddress, x, y, (short)BLACK);
    }
  }
}

void drawBall(volatile int backBufferAddress, int x, int y, short colour) {
  for (int i = x - 1; i < x + 2; i++) {
    for (int j = y - 1; j < y + 2; j++) {
      if (i >= 0 && i < 320 && j >= 0 && j < 240) {
        drawPixel(backBufferAddress, i, j, colour);
      }
    }
  }
}