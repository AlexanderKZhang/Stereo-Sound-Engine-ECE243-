#ifndef PS2_H
#define PS2_H

struct mouse {
  volatile int* PS2_ptr;
  int x[3];
  int y[3];
};

void ps2Setup();
void readPS2();
void HEX_PS2(char b1, char b2, char b3);

#endif