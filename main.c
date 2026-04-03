#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "address_map.h"
#include "audio.h"
#include "greyCircle.h"
#include "ps2.h"
#include "vga.h"


void interruptSetup();
void interruptHandler() __attribute__((interrupt("machine")));

int cursorX, cursorY, angle, elevation;

struct mouse Mouse;

int main(void) {
  /* Declare volatile pointers to I/O registers (volatile means that IO load
  and store instructions will be used to access these pointer locations,
  instead of regular memory loads and stores) */

  Mouse.PS2_ptr = (int*)PS2_BASE;
  Mouse.x = Mouse.buffer1X = Mouse.buffer2X = 160;
  Mouse.y = Mouse.buffer1Y = Mouse.buffer2Y = 120;

  volatile int* VGABase = vgaSetup((unsigned int)PIXEL_BUF_CTRL_BASE);
  interruptSetup();

  //some fix text that will be drawn to the screen
  char degree_fix_text[10] = "Azimuths: ";
  char elevation_fix_text[12] = "Elevation: ";

  bool drawingBuffer1 = true;
  while (1) {
    cursorX = Mouse.x;
    cursorY = Mouse.y;
    // extract mouse position relative to centre of screen
    int xFromCentre = (cursorX) - (GREYCIRCLE_WIDTH >> 1);
    int yFromCentre = (cursorY) - (GREYCIRCLE_HEIGHT >> 1);
    
    // compute the angle from the centre point
    // we want angle relative to y axis, negative angles are mapped to quadrant 2, 3, positive angles are mapped to quadrant 1, 4
    // this is achieve by treating x as y, y as x
    angle = calculateAngle(-yFromCentre, xFromCentre);
    char angleStr[4];
    intToStr(abs(angle), angleStr);
    
    char elevationStr[4];
    intToStr(elevation, elevationStr);
  
    volatile int backBufferAddress = VGABase[1];
    if (drawingBuffer1) {
      undrawBall(backBufferAddress, Mouse.buffer2X, Mouse.buffer2Y);
      Mouse.buffer2X = cursorX;
      Mouse.buffer2Y = cursorY;
    } else {
      undrawBall(backBufferAddress, Mouse.buffer1X, Mouse.buffer1Y);
      Mouse.buffer1X = cursorX;
      Mouse.buffer1Y = cursorY;
    }
    drawBall(backBufferAddress, cursorX, cursorY, (short)WHITE);
    video_text(8, 8, angleStr);
    video_text(16, 8, degree_fix_text);
    video_text(304, 8, elevation_fix_text);
    video_text(312, 8, elevationStr);
    waitForSync(VGABase);
    drawingBuffer1 = !drawingBuffer1;
  }
}

void interruptSetup() {
  int mstatusValue, mtvecValue, mieValue;
  // temporarily disable interrupts during setup
  mstatusValue = 0b1000;
  __asm__ volatile("csrc mstatus, %0" ::"r"(mstatusValue));

  // turn on interrupts on the PS2 side
  ps2Setup();
  audio_setup();

  // enable interrupts within the processor for PS2(IRQ22)
  mieValue = (0b1 << 22);
  __asm__ volatile("csrs mie, %0" ::"r"(mieValue));

  // enable interrupts for Audio port (IRQ21)
  mieValue = (0b1 << 21);
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
  int mcause_value;
  __asm__ volatile("csrr %0, mcause" : "=r"(mcause_value));

  // look at the lower 31 bits (remove bit 32) and see if ISR22 causes the
  // interrupt
  if ((mcause_value & 0x7FFFFFFF) == (22)) {
    // PS2 interrupt
    readPS2(Mouse);
  }

  // look at the lower 31 bits (remove bit 32) and see if ISR21 causes the
  // interrupt
  if ((mcause_value & 0x7FFFFFFF) == (21)) {
    // audio interrupt
    handle_audio();
  }
}