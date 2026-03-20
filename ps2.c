/*
Byte 1: The Status Byte
This is the most important byte. It acts as a dashboard for the mouse's current
state and tells you how to read the next two bytes.

Bit 0 (Rightmost): Left Button (1 = pressed, 0 = released)

Bit 1: Right Button

Bit 2: Middle Button

Bit 3: Always 1

Bit 4: X-Axis Sign Bit (1 = moving left/negative, 0 = moving right/positive)

Bit 5: Y-Axis Sign Bit (1 = moving down/negative, 0 = moving up/positive)

Bit 6: X-Axis Overflow (moved too fast for the 8-bit counter)

Bit 7 (Leftmost): Y-Axis Overflow

Byte 2: X-Axis Movement
This is an 8-bit number representing how far the mouse moved horizontally.

Because it is 8 bits, the raw values range from 0 to 255 (0x00 to 0xFF).

However, this is actually a Two's Complement signed integer (-128 to +127).

Byte 3: Y-Axis Movement
Exactly like Byte 2, but for the vertical axis.
*/

#include "ps2.h"

#include "address_map.h"

extern struct mouse Mouse;

void ps2Setup() {
  // PS/2 mouse needs to be reset (must be already plugged in)
  *Mouse.PS2_ptr = 0xFF;  // reset

  int PS2Data;

  // wait for the PS2 to finish internal setup before proceeding
  // Wait for 0xAA
  while (1) {
    PS2Data = *Mouse.PS2_ptr;
    // read data when it is available
    // data is available when RVALID (bit 15 of PS2Data) is a 1
    if (PS2Data & (1 << 15)) {
      if ((char)(PS2Data & 0xFF) == (char)0xAA) break;
    }
  }

  // Wait for 0x00 (Mouse ID) which always follows 0xAA
  while (1) {
    PS2Data = *Mouse.PS2_ptr;
    // read data when it is available
    // data is available when RVALID (bit 15 of PS2Data) is a 1
    if (PS2Data & (1 << 15)) {
      if ((char)(PS2Data & 0xFF) == (char)0x00) break;
    }
  }

  // mouse inserted; initialize sending of data
  *Mouse.PS2_ptr = 0xF4;

  // setup interrupts for the PS2 port
  Mouse.PS2_ptr[1] = Mouse.PS2_ptr[1] | 0b1;
}

void readPS2() {
  static int PS2_data, RVALID;
  PS2_data = *Mouse.PS2_ptr;  // read the Data register in the PS/2 port
  static unsigned char bytes[3] = {0, 0, 0};
  static int byteCounter = 0;
  RVALID = PS2_data & 0x8000;  // extract the RVALID field
  if (RVALID) {
    unsigned char newData = (char)(PS2_data & 0xFF);

    // check for byte misalignment and skip the byte if misaligned
    if (byteCounter == 0 && !(newData & (1 << 3))) return;

    /* shift the next data byte into the display */
    bytes[0] = bytes[1];
    bytes[1] = bytes[2];
    bytes[2] = newData;
    byteCounter++;

    if (byteCounter >= 3) {
      byteCounter = 0;
      HEX_PS2(bytes[0], bytes[1], bytes[2]);

      // decipher the PS2 data bytes
      int xData = (bytes[0] & (1 << 4)) ? (0xFFFFFF00 | bytes[1]) : bytes[1];
      int yData = (bytes[0] & (1 << 5)) ? (0xFFFFFF00 | bytes[2]) : bytes[2];

      if (xData) {
        Mouse.x[0] = Mouse.x[1];
        Mouse.x[1] = Mouse.x[2];
        Mouse.x[2] += xData;
        if (Mouse.x < 0)
          Mouse.x[2] = 0;
        else if (Mouse.x >= 320)
          Mouse.x[2] = 319;
      }

      if (yData) {
        Mouse.y[0] = Mouse.y[1];
        Mouse.y[1] = Mouse.y[2];
        Mouse.y[2] -= yData;
        if (Mouse.y[2] < 0)
          Mouse.y[2] = 0;
        else if (Mouse.y[2] >= 240)
          Mouse.y[2] = 239;
      }
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
  shift_buffer = (unsigned int)((b1 << 16) | (b2 << 8) | b3);
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