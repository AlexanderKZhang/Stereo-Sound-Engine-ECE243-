// padding added to buffers in the x direction
short buffer1[240][512];
short buffer2[240][512];

#define WHITE 0xFFFF
#define BLACK 0x0000

int* vgaSetup(int* VGABaseAddress);
void vgaDriver(volatile int* VGABase, int mouseX, int mouseY);
void waitForSync(volatile int* VGABase);
void drawPixel(volatile int backBufferAddress, int x, int y, short colour);
void clearScreen(volatile int backBufferAddress);
void drawBox3(volatile int backBufferAddress, int x, int y, short colour);