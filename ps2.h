#ifndef PS2_H
#define PS2_H

struct mouse {
  volatile int* PS2_ptr;
  int x;
  int y;
};

void ps2Setup(struct mouse* Mouse);
void readPS2(struct mouse* Mouse);
void HEX_PS2(char b1, char b2, char b3);

#endif