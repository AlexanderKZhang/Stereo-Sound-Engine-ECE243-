#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "address_map.h"
// #include "ps2.c"
#include "ps2.h"
// #include "vga.c"
#include "vga.h"

int main(void) {
  /* Declare volatile pointers to I/O registers (volatile means that IO load
  and store instructions will be used to access these pointer locations,
  instead of regular memory loads and stores) */

  struct mouse Mouse;

  Mouse.PS2_ptr = (int*)PS2_BASE;
  Mouse.x = 160;
  Mouse.y = 120;

  volatile int* VGABase = vgaSetup((unsigned int)PIXEL_BUF_CTRL_BASE);
  ps2Setup(&Mouse);

  while (1) {
    vgaDriver(VGABase, Mouse.x, Mouse.y);
    readPS2(&Mouse);
  }
}
