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
// Below two globals are used in vga display
int current_left_sample = 0;
int current_right_sample = 0;

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

  // Peak hold state variables
  int left_peak = 0, right_peak = 0;             // The current master peak values
  int left_peak_timer = 0, right_peak_timer = 0; // Timers to track how long they've been held
  const int PEAK_HOLD_FRAMES = 30;               // Wait ~0.5 seconds (at 60fps) before dropping
  // Track previous peak heights to undraw them in the correct buffer
  int left_p_buffer1 = 0, left_p_buffer2 = 0;
  int right_p_buffer1 = 0, right_p_buffer2 = 0;

  // Track the previous heights of the bars for both buffers
  int left_h_buffer1 = 0, left_h_buffer2 = 0;
  int right_h_buffer1 = 0, right_h_buffer2 = 0;
  // Coordinates for the bottom-left corner of the volume bars
  int leftBarX = 10, leftBarY = 200;
  int rightBarX = 310, rightBarY = 200;
  int maxBarHeight = 200; // Cap the bar height to 200 pixels


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
    
    // Calculate the new bar heights based on the latest audio samples
    int current_left_h = abs(current_left_sample) >> 17; 
    if (current_left_h > maxBarHeight) current_left_h = maxBarHeight;

    int current_right_h = abs(current_right_sample) >> 17;
    if (current_right_h > maxBarHeight) current_right_h = maxBarHeight;

    // Update Left Peak
    if (current_left_h >= left_peak) {
      left_peak = current_left_h; // New high! Update peak.
      left_peak_timer = 0;        // Reset the timer.
    } else {
      left_peak_timer++;
      if (left_peak_timer > PEAK_HOLD_FRAMES) {
        left_peak -= 2; // Decay speed (drops 2 pixels per frame)
        if (left_peak < current_left_h) left_peak = current_left_h; // Don't drop below current bar
      }
    }

    // Update Right Peak
    if (current_right_h >= right_peak) {
      right_peak = current_right_h;
      right_peak_timer = 0;
    } else {
      right_peak_timer++;
      if (right_peak_timer > PEAK_HOLD_FRAMES) {
        right_peak -= 2; 
        if (right_peak < current_right_h) right_peak = current_right_h; 
      }
    }

    volatile int backBufferAddress = VGABase[1];
    
    // Undraw old elements and update states
    if (drawingBuffer1) {
      undrawBall(backBufferAddress, Mouse.buffer2X, Mouse.buffer2Y);
      undrawVolumeBar(backBufferAddress, leftBarX, leftBarY, left_h_buffer2);
      undrawVolumeBar(backBufferAddress, rightBarX, rightBarY, right_h_buffer2);
      
      undrawPeakLine(backBufferAddress, leftBarX, leftBarY, left_p_buffer2);
      undrawPeakLine(backBufferAddress, rightBarX, rightBarY, right_p_buffer2);
      
      // Save states for buffer 2
      Mouse.buffer2X = cursorX;
      Mouse.buffer2Y = cursorY;
      left_h_buffer2 = current_left_h;
      right_h_buffer2 = current_right_h;
      left_p_buffer2 = left_peak;
      right_p_buffer2 = right_peak;

    } else {
      undrawBall(backBufferAddress, Mouse.buffer1X, Mouse.buffer1Y);
      undrawVolumeBar(backBufferAddress, leftBarX, leftBarY, left_h_buffer1);
      undrawVolumeBar(backBufferAddress, rightBarX, rightBarY, right_h_buffer1);
      
      undrawPeakLine(backBufferAddress, leftBarX, leftBarY, left_p_buffer1);
      undrawPeakLine(backBufferAddress, rightBarX, rightBarY, right_p_buffer1);
      
      // Save states for buffer 1
      Mouse.buffer1X = cursorX;
      Mouse.buffer1Y = cursorY;
      left_h_buffer1 = current_left_h;
      right_h_buffer1 = current_right_h;
      left_p_buffer1 = left_peak;
      right_p_buffer1 = right_peak;
    }
    
    // Draw the new elements
    drawBall(backBufferAddress, cursorX, cursorY, (short)WHITE);
    
    // Draw the main volume bars
    drawVolumeBar(backBufferAddress, leftBarX, leftBarY, current_left_h, 0x7E0B); // Green
    drawVolumeBar(backBufferAddress, rightBarX, rightBarY, current_right_h, 0x7E0B); 

    // Draw the peak lines slightly above the bars (Yellow: 0xFFE0)
    drawPeakLine(backBufferAddress, leftBarX, leftBarY, left_peak, 0xFFE0); 
    drawPeakLine(backBufferAddress, rightBarX, rightBarY, right_peak, 0xFFE0);
    
    video_text(8, 8, degree_fix_text);
    video_text(18, 8, angleStr);
    video_text(8, 10, elevation_fix_text);
    video_text(18, 10, elevationStr);
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