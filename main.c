#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "address_map.h"
// #include "ps2.c"
#include "ps2.h"
// #include "vga.c"
#include "vga.h"

void interruptSetup();
void interruptHandler() __attribute__((interrupt("machine")));

struct mouse Mouse;

extern short buffer1[240][512];
extern short buffer2[240][512];

int main(void) {
  /* Declare volatile pointers to I/O registers (volatile means that IO load
  and store instructions will be used to access these pointer locations,
  instead of regular memory loads and stores) */

  Mouse.PS2_ptr = (int*)PS2_BASE;
  Mouse.x = Mouse.buffer1X = Mouse.buffer2X = 160;
  Mouse.y = Mouse.buffer1Y = Mouse.buffer2Y = 120;

  volatile int* VGABase = vgaSetup((unsigned int)PIXEL_BUF_CTRL_BASE);
  interruptSetup();

  while (1) {
    volatile int backBufferAddress = VGABase[1];
    if (backBufferAddress == (int)buffer1) {
      drawBall(backBufferAddress, Mouse.buffer1X, Mouse.buffer1Y, (short)BLACK);
      Mouse.buffer1X = Mouse.x;
      Mouse.buffer1Y = Mouse.y;
    } else {
      drawBall(backBufferAddress, Mouse.buffer2X, Mouse.buffer2Y, (short)BLACK);
      Mouse.buffer2X = Mouse.x;
      Mouse.buffer2Y = Mouse.y;
    }
    drawBall(backBufferAddress, Mouse.x, Mouse.y, (short)WHITE);
    waitForSync(VGABase);
  }
}

void interruptSetup() {
  int mstatusValue, mtvecValue, mieValue;
  // temporarily disable interrupts during setup
  mstatusValue = 0b1000;
  __asm__ volatile("csrc mstatus, %0" ::"r"(mstatusValue));

  // turn on interrupts on the PS2 side
  ps2Setup();

  // enable interrupts within the processor for PS2(IRQ22)
  mieValue = (0b1 << 22);
  __asm__ volatile("csrs mie, %0" ::"r"(mieValue));

  // store the interruptHandler address into mtvec register
  mtvecValue = (int)&interruptHandler;
  __asm__ volatile("csrw mtvec, %0" ::"r"(mtvecValue));

  // re-enable interrupts
  __asm__ volatile("csrs mstatus, %0" ::"r"(mstatusValue));

  // setup done!
}

void interruptHandler() {
  // read machine interrupt pending (mip) register value to check which device
  // caused the interrupt exception
  int mipValue;
  __asm__ volatile("csrr %0, mip" : "=r"(mipValue));

  if (mipValue & (1 << 22)) {
    // PS2 interrupt
    readPS2(Mouse);
  }
}