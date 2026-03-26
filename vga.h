#ifndef VGA_H
#define VGA_H

#define WHITE 0xFFFF
#define BLACK 0x0000

int cursorX, cursorY;

int* vgaSetup(unsigned int VGABaseAddress);
void waitForSync(volatile int* VGABase);
void drawPixel(volatile int backBufferAddress, int x, int y, short colour);
void clearScreen(volatile int backBufferAddress);
void drawBall(volatile int backBufferAddress, int x, int y, short colour);
void undrawBall(volatile int backBufferAddress, int x, int y);

#endif