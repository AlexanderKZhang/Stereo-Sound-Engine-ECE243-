#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "address_map.h"
#include "p2s.h"
#include "vga.h"

int main(void) {
  /* Declare volatile pointers to I/O registers (volatile means that IO load
  and store instructions will be used to access these pointer locations,
  instead of regular memory loads and stores) */
  volatile int* VGA_Base = (int*)PIXEL_BUF_CTRL_BASE;

  struct mouse {
    volatile int* PS2_ptr;
    int x;
    int y;
  } Mouse;

  Mouse.PS2_ptr = (int*)PS2_BASE;
  Mouse.x = 160;
  Mouse.y = 120;

  int* VGABase = vgaSetup(PIXEL_BUF_CTRL_BASE);
  vgaDriver(VGABase, Mouse.x, Mouse.y);
}
