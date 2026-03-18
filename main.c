#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "address_map.h"

/* function prototypes */
void HEX_PS2(char, char, char);
/*******************************************************************************
 * This program demonstrates use of the PS/2 port by displaying the last three
 * bytes of data received from the PS/2 port on the HEX displays.
 ******************************************************************************/
int main(void) {
  /* Declare volatile pointers to I/O registers (volatile means that IO load
  and store instructions will be used to access these pointer locations,
  instead of regular memory loads and stores) */
  volatile int* PS2_ptr = (int*)PS2_BASE;
  int PS2_data, RVALID;
  char byte1 = 0, byte2 = 0, byte3 = 0;
  // PS/2 mouse needs to be reset (must be already plugged in)
  *(PS2_ptr) = 0xFF;  // reset
  while (1) {
    PS2_data = *(PS2_ptr);       // read the Data register in the PS/2 port
    RVALID = PS2_data & 0x8000;  // extract the RVALID field
    if (RVALID) {
      /* shift the next data byte into the display */
      byte1 = byte2;
      byte2 = byte3;
      byte3 = (PS2_data & 0xFF);
      HEX_PS2(byte1, byte2, byte3);
      //   if ((byte2 == (char)0xAA) && (byte3 == (char)0x00))
      //     // mouse inserted; initialize sending of data
      //     *(PS2_ptr) = 0xF4;
    }
  }
}