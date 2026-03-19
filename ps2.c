// mouse data
#define moveUP 0x080001
#define moveDOWN 0x2800FF
#define moveLEFT 0x18FF00
#define moveRIGHT 0x080100
#define leftClick 0x090000
#define rightClick 0x0A0000
#define middleClick 0x0C0000
#define buttonRelease 0x080000

#include "address_map.h"
#include "p2s.h"

void readPS2(volatile int* PS2_ptr) {
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

/****************************************************************************************
 * Subroutine to show a string of HEX data on the HEX displays
 ****************************************************************************************/
void HEX_PS2(char b1, char b2, char b3) {
  volatile int* HEX3_HEX0_ptr = (int*)HEX3_HEX0_BASE;
  volatile int* HEX5_HEX4_ptr = (int*)HEX5_HEX4_BASE;
  /* SEVEN_SEGMENT_DECODE_TABLE gives the on/off settings for all segments in
   * a single 7-seg display in the DE1-SoC Computer, for the hex digits 0 - F
   */
  unsigned char seven_seg_decode_table[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D,
                                            0x7D, 0x07, 0x7F, 0x67, 0x77, 0x7C,
                                            0x39, 0x5E, 0x79, 0x71};
  unsigned char hex_segs[] = {0, 0, 0, 0, 0, 0, 0, 0};
  unsigned int shift_buffer, nibble;
  unsigned char code;
  int i;
  shift_buffer = (b1 << 16) | (b2 << 8) | b3;
  for (i = 0; i < 6; ++i) {
    nibble = shift_buffer & 0x0000000F;  // character is in rightmost nibble
    code = seven_seg_decode_table[nibble];
    hex_segs[i] = code;
    shift_buffer = shift_buffer >> 4;
  }
  /* drive the hex displays */
  *(HEX3_HEX0_ptr) = *(int*)(hex_segs);
  *(HEX5_HEX4_ptr) = *(int*)(hex_segs + 4);
}