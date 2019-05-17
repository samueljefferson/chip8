#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ncurses.h>
#include <pthread.h>
#include <linux/input.h>

#include "chip8.h"

int testing = 1;

unsigned char memory[4096];    // 4096 bytes of memory
unsigned char V[16];           // 16 8 bit registers
unsigned short I;              // 16 bit register
unsigned short PC = 0x200;     // program counter is 16 bit
char SP;                       // SP is 8 bit
unsigned short stack[16];      // stack in 16 16 bit values

int keyboard = 0;              // contains flags for all keys from 0-F

//char key =

void *kb_input(void *ptr) {
  // printw("test\n");
  // refresh();
  // setup keyboard stuff
  int kb_convert[256];  // array used to convert ev.code into correct values
  memset(kb_convert, -1, 256);
  kb_convert[2] = 2;
  kb_convert[3] = 4;
  kb_convert[4] = 8;
  kb_convert[5] = 4096;

  kb_convert[16] = 16;
  kb_convert[17] = 32;
  kb_convert[18] = 64;
  kb_convert[19] = 8192;

  kb_convert[30] = 128;
  kb_convert[31] = 256;
  kb_convert[32] = 512;
  kb_convert[33] = 16384;

  kb_convert[44] = 1024;
  kb_convert[45] = 1;
  kb_convert[46] = 2048;
  kb_convert[47] = 32768;

  char *dev = "/dev/input/by-path/platform-i8042-serio-0-event-kbd";
  struct input_event ev;
  ssize_t n;
  int fd;
  fd = open(dev, O_RDONLY);

  while (1) {
    // get key events
    n = read(fd, &ev, sizeof ev);
    int temp = -1;

    if (ev.type == EV_KEY && ev.value >= 0 && ev.value <= 2)
    {
      if ((int)ev.code == 1)
      {
        endwin();									// close window
        exit(0);
      }
      // mvprintw(12,0, "        ");
      // mvprintw(12,0, "%d", (int)ev.code);
      // // if a key is pressed
      if (ev.value == 1)
      {
        temp = kb_convert[(int)ev.code];
        if (temp != -1)
        {
          keyboard = keyboard | temp;
        }
      }
      // if the key is released
      else if (ev.value == 0)
      {
        temp = kb_convert[(int)ev.code];
        temp = ~temp;
        if (temp != -1)
        {
          keyboard = keyboard & temp;
        }
      }
      // mvprintw(1,0, "%d", keyboard);
      // refresh();
    }
  }

  return 0;
}

int main(int argc, char** argv) {
  init();
  srand(time(NULL));  // needs a seed for random

  //displayReg();
  if (argc != 2) {
    fprintf(stderr, "error: requires a rom\n");
    exit(0);
  }
  // printf("%X, %u\n", 0xF000, 0x1000); // TODO REMOVE
  char* fileName = argv[1];
  //dumpRom(fileName);
  //dumpInstructions(fileName);



  // setup ncurses
  initscr();								// initialize screen
  raw();										// raw mode
  curs_set(0);              // hide cursor

  // create a thread for reading keyboard input
  pthread_t id;
  pthread_create(&id, NULL, kb_input, NULL);

  while (1) {

  }
  // mvaddch(0, 0, ' ' | A_REVERSE); // OR of space and reverse
  // mvaddch(0, 63, ' ' | A_REVERSE);
  // mvaddch(31, 0, ' ' | A_REVERSE);
  // mvaddch(31, 63, ' ' | A_REVERSE);
  // getch();

  // I_00E0(3);

  /*
  getch();
  V[1] = 0;
  V[2] = 0;
  I_Dxyn(0xD125);
  */
  // int i = 0;
  // int spaceX = 5;
  // int spaceY = 7;
  // V[1] = 0;
  // V[2] = 0;
  // for (i = 0; i < 0x10; i++) {
  //   I_Dxyn(0xD125);
  //   V[1] += spaceX;
  //   if (V[1] > 63) {
  //     V[1] = 0;
  //     V[2] += spaceY;
  //   }
  //   I += 5;
  // }
  //
  // getch();




  // dumpInstr(fileName);
  // exit(0);
  // loadRom(fileName);
  // int currentInstr;
  // currentInstr = readInstr();
  // //printf("%X\n", currentInstr);
  // lookupInstr(currentInstr);

  endwin();									// close window

  return 0;
}

void init() {
  // memset everything to 0
  memset(memory, 0, 4096);
  setASCII();
  memset(V, 0, 16);
  I = 0;
  SP = 0;
  memset(stack, 0, 32);
}

void displayReg() {
  // displays the values of all the registers
  int i = 0;
  for (i = 0; i < 16; i++) {
    printf("V%d %X\n", i, V[I]);
  }
  printf("I %X\n", I);
  printf("PC %X\n", PC);
  printf("SP %d\n", SP);
  for (i = 0; i < 16; i++) {
    printf("stack %d %X\n", i, stack[I]);
  }
}
void lookupInstr(int instr) {

  // Annn 0xA000 to 0xAFFF
  if ((instr >= 0xA000) && (instr <= 0xAFFF)) {
    I_Annn(instr);
  }
}

void I_0nnn(int instr) {
  // TODO
}

void I_00E0(int instr) {
  // clear display
  clear();
}

void I_00EE(int instr) {
  // return from subroutine
  // set PC to stack address and decrement SP
  PC = stack[SP];
  SP--;
}

void I_1nnn(int instr) {
  // set PC to nnn
  PC = instr - 0x1000;
}

void I_2nnn(int instr) {
  // call subroutine at nnn
  // increment SP
  SP++;
  // put PC on top of stack
  stack[SP] = PC;
  // set PC to nnn
  PC = instr - 0x2000;
}

void I_3xkk(int instr) {
  // skip next instruction if Vx = kk

  // get x
  int x = (instr & 0x0F00) >> 8;
  // get kk
  int kk = (instr & 0x00FF);
  // skip next instruction if Vx = kk
  if (V[x] == kk) {
    PC += 2;
  }
}

void I_4xkk(int instr) {
  // skip next instruction if Vx != kk

  // get x
  int x = (instr & 0x0F00) >> 8;
  // get kk
  int kk = (instr & 0x00FF);
  // skip next instruction if Vx != kk
  if (V[x] != kk) {
    PC += 2;
  }
}

void I_5xy0(int instr) {
  // skip next instruction in Vx = Vy
  int x = (instr & 0x0F00) >> 8;
  int y = (instr & 0x00F0) >> 4;
  if (V[x] == V[y]) {
    PC += 2;
  }
}

void I_6xkk(int instr) {
  // set Vx = kk
  int x = (instr & 0x0F00) >> 8;
  int kk = instr & 0x00FF;
  V[x] = kk;
}

void I_7xkk(int instr) {
  // set Vx += kk
  int x = (instr & 0x0F00) >> 8;
  int kk = instr & 0x00FF;
  V[x] += kk;
}

void I_8xy1(int instr) {
  // Vx = Vx | Vy
  int x = (instr & 0x0F00) >> 8;
  int y = (instr & 0x00F0) >> 4;
  V[x] = V[x] | V[y];
}

void I_8xy2(int instr) {
  // Vx = Vx & Vy
  int x = (instr & 0x0F00) >> 8;
  int y = (instr & 0x00F0) >> 4;
  V[x] = V[x] & V[y];
}

void I_8xy3(int instr) {
  // Vx = Vx ^ Vy (XOR)
  int x = (instr & 0x0F00) >> 8;
  int y = (instr & 0x00F0) >> 4;
  V[x] = V[x] ^ V[y];
}

void I_8xy4(int instr) {
  // Vx += Vy
  // VF is 1 if carry
  int x = (instr & 0x0F00) >> 8;
  int y = (instr & 0x00F0) >> 4;
  int sum = V[x] + V[y];
  V[x] = sum;
  if (sum > 0xFF) {
    V[0xF] = 1;
  } else {
    V[0xF] = 0;
  }
}

void I_8xy5(int instr) {
  // Vx -= Vy
  // if Vx > Vy then VF = 0
  int x = (instr & 0x0F00) >> 8;
  int y = (instr & 0x00F0) >> 4;
  if (V[x] > V[y]) {
    V[0xF] = 1;
  } else {
    V[0xF] = 0;
  }
  V[x] -= V[y];
}

void I_8xy6(int instr) {
  // shift right
  // if the smallest bit in Vx is 1 then VF = 1
  // otherwise VF = 2
  // then shift Vx 1 right
  int x = (instr & 0x0F00) >> 8;
  int bit = V[x] & 0x01;
  if (bit) {
    V[0xf] = 1;
  } else {
    V[0xf] = 0;
  }
  V[x] = V[x] >> 1;
}

void I_8xy7(int instr) {
  // Vx = Vy - Vx
  // if Vy > Vx then Vf = 1, else Vf = 0
  int x = (instr & 0x0F00) >> 8;
  int y = (instr & 0x00F0) >> 4;
  if (V[y] > V[x]) {
    V[0xF] = 1;
  } else {
    V[0xF] = 0;
  }
  V[x] = V[y] - V[x];

}

void I_8xyE(int instr) {
  // shift Vx left 1
  // if left most bit is 1 then Vf = 1, else Vf = 0
  int x = (instr & 0x0F00) >> 8;
  int bit = V[x] & 0x80;
  if (bit) {
    V[0xf] = 1;
  } else {
    V[0xf] = 0;
  }
  V[x] = V[x] << 1;
}

void I_9xy0(int instr) {
  // skip next instruction if Vx != Vy
  int x = (instr & 0x0F00) >> 8;
  int y = (instr & 0x00F0) >> 4;
  if (V[x] != V[y]) {
    PC += 2;
  }
}

void I_Annn(int instr) {
  // set I to nnn
  instr -= 40960;

  if (testing) {
    printf("Instruction Annn: set I from %X to %X\n", I, instr);
  }

  I = instr;
}

void I_Bnnn(int instr) {
  // PC = V0 + nnn
  int nnn = instr & 0x0FFF;
  PC = V[0] + nnn;
}

void I_Cxkk(int instr) {
  // Vx = random byte & kk
  int random = rand() % 0xFF;
  int x = (instr & 0x0F00) >> 8;
  int kk = instr & 0x00FF;
  V[x] = random & kk;
}

void I_Dxyn(int instr) {
  // display n bytes, starting at memory[I]
  // start at coordinates Vx, Vy (reverse for ncurses)
  int x = (instr & 0x0F00) >> 8;
  int y = (instr & 0x00F0) >> 4;
  int n = (instr & 0x000F);
  x = V[x];
  y = V[y];

  // each sprite is 8 wide and up to 15 long
  // mvaddch(0, 63, ' ' | A_REVERSE);

  int i = 0;
  int j = 0;
  int byte = 0;
  int bit = 0;
  // display each column
  for (i = 0; i < n; i++) {
    // display each row
    byte = memory[I + i];
    for (j = 0; j < 8; j++) {
      // get current value of bit
      bit = byte & 0x80;
      if (bit) {
        // display bit
        mvaddch((y + i), (x + j), ' ' | A_REVERSE);
      } else {
        // display nothing
        mvaddch((y + i), (x + j), ' ');
      }


      byte = byte << 1;
    }
  }
}

void I_Ex9E(int instr) {
  // skip next instruction if key == Vx
}

void I_Fx0A(int instr) {
  // wait for a key press and store it in Vx
  int x = (instr & 0x0F00) >> 8;
  char key = getch();
  char num = processKey(key);
  V[x] = num;
}

void I_Fx1E(int instr) {
  // I += Vx
  int x = (instr & 0x0F00) >> 8;
  I += x;
}

void I_Fx33(int instr) {
  // store BCD value of Vx in
  // memory[I], memory[I + 1], memory[I + 2]
  int x = (instr & 0x0F00) >> 8;
  // ones
  memory[I] = V[x] % 10;
  // tens
  memory[I + 1] = V[x] % 100 - memory[I];
  // hundreds
  memory[I + 2] = V[x] % 1000 - memory[I] - memory[I + 1];
}

void I_Fx55(int instr) {
  // stores registers V0 to Vx in memory
  // starting at memory[I]
  int x = (instr & 0x0F00) >> 8;
  int i = 0;
  for (i = 0; i < x; i++) {
    memory[I + i] = V[i];
  }
}

void I_Fx65(int instr) {
  // reads registers V0 to Vx in memory
  // starting from location memory[I]
  int x = (instr & 0x0F00) >> 8;
  int i = 0;
  for (i = 0; i < x; i++) {
    V[i] = memory[I + i];
  }
}

int readInstr() {
  // read instruction from memory
  int instr = memory[PC];   // get first byte
  instr = instr << 8;       // rotate left 8 bits to get room for 2nd byte
  instr += memory[PC + 1];  // get second byte
  //printf("%x\n", instr);
  // TODO update PC here?
  return instr;
}

void loadRom(char* fileName) {
  int fd = open(fileName, O_RDONLY);
  //printf("%d\n", fd); // TODO remove

  char buffer[1];
  int current = 512;    // current memory address
  int status = 1;       // equals 0 at end of file

  // loads the program into memory
  while (current < 4096 && status != 0) {
    status = read(fd, buffer, 1);
    memory[current] = buffer[0];

    //printf("%c, %d\n", memory[current], current);
    //printf("%d ", current);
    //printHex(memory[current]);
    //printf("\t");
    //printBinary(memory[current]);
    //printf("\n");

    current++;
  }

  close(fd);
}

void dumpRom(char* fileName) {
  printf("%s\n", fileName); // TODO remove

  int fd = open(fileName, O_RDONLY);
  //printf("%d\n", fd); // TODO remove

  // set up memory
  // 4 kb, 4096 bytes
  char v_memory[4096];
  // load memory from 0x200 to 0xFFF with program
  // 512-4095
  char buffer[1];
  int current = 512;    // current memory address
  int status = 1;       // equals 0 at end of file

  // loads the program into memory
  while (current < 4096 && status != 0) {
    status = read(fd, buffer, 1);
    v_memory[current] = buffer[0];

    //printf("%c, %d\n", memory[current], current);
    printf("%d %X\t", current, current);
    printBinary(v_memory[current]);
    printf("\n");

    current++;
  }

  close(fd);
}

void dumpInstr(char* fileName) {
  printf("%s Instructions\n", fileName); // TODO remove

  int fd = open(fileName, O_RDONLY);
  //printf("%d\n", fd); // TODO remove

  // set up memory
  // 4 kb, 4096 bytes
  char v_memory[4096];
  // load memory from 0x200 to 0xFFF with program
  // 512-4095
  char buffer[1];
  int current = 512;    // current memory address
  int status = 1;       // equals 0 at end of file

  // loads the program into memory
  while (current < 4096 && status != 0) {
    status = read(fd, buffer, 1);
    v_memory[current] = buffer[0];
    printf("%d %X\t", current, current);
    printHex(v_memory[current]);
    printHex(v_memory[current+1]);
    printf("\n");
    current +=2;
  }

  close(fd);
}

void printHex(int num) {
  /*
   *  convets an int value into hex and prints it
   *  max value is 255
   */

   // get left nibble
   int temp = num & 240;
   temp = temp >> 4;
   //printf("%d\n", temp);
   if (temp < 10) {
     printf("%d", temp);
   } else {
      // A is 65, 10 is already 10 so 10 + 55 = 65 or 'A'
     printf("%c", temp + 55);
   }
   // get right nibble
   temp = num & 15;
   if (temp < 10) {
     printf("%d", temp);
   } else {
      // A is 65, 10 is already 10 so 10 + 55 = 65 or 'A'
     printf("%c", temp + 55);
   }
}

void printBinary(int num) {
  /*
   *  convets an int value into hex and prints it
   *  max value is 255
   */
   int i;
   int result;
   int prev = num;
   for (i = 128; i > 0; i /= 2) {
     result = num % i;
     if (result == prev) {
       printf("0");
     } else {
       printf("1");
     }
     prev = result;
   }
}

unsigned int processKey(char key) {
  // 123C   1234
  // 456D   qwer
  // 789E   asdf
  // A0BF   zxcv
  switch (key) {
    case '1':
      return 0x01;
    case '2':
      return 0x02;
    case '3':
      return 0x03;
    case '4':
      return 0x0C;
    case 'q':
      return 0x04;
    case 'w':
      return 0x05;
    case 'e':
      return 0x06;
    case 'r':
      return 0x0D;
    case 'a':
      return 0x07;
    case 's':
      return 0x08;
    case 'd':
      return 0x09;
    case 'f':
      return 0x0E;
    case 'z':
      return 0x0A;
    case 'x':
      return 0x00;
    case 'c':
      return 0x0B;
    case 'v':
      return 0x0F;
    default:
      return 0x00;  // TODO error here?
  }
}

void setASCII() {
  // TODO finish
  // set the values from 0x00 to 0x1FF for fonts
  // of 0 to F

  // 0
  memory[0x00] = 0xF0;
  memory[0x01] = 0x90;
  memory[0x02] = 0x90;
  memory[0x03] = 0x90;
  memory[0x04] = 0xF0;

  // 1
  memory[0x05] = 0x20;
  memory[0x06] = 0x60;
  memory[0x07] = 0x20;
  memory[0x08] = 0x20;
  memory[0x09] = 0x70;

  // 2
  memory[0x0A] = 0xF0;
  memory[0x0B] = 0x10;
  memory[0x0C] = 0xF0;
  memory[0x0D] = 0x80;
  memory[0x0E] = 0xF0;

  // 3
  memory[0x0F] = 0xF0;
  memory[0x10] = 0x10;
  memory[0x11] = 0xF0;
  memory[0x12] = 0x10;
  memory[0x13] = 0xF0;

  // 4
  memory[0x14] = 0x90;
  memory[0x15] = 0x90;
  memory[0x16] = 0xF0;
  memory[0x17] = 0x10;
  memory[0x18] = 0x10;

  // 5
  memory[0x19] = 0xF0;
  memory[0x1A] = 0x80;
  memory[0x1B] = 0xF0;
  memory[0x1C] = 0x10;
  memory[0x1D] = 0xF0;

  // 6
  memory[0x1E] = 0xF0;
  memory[0x1F] = 0x80;
  memory[0x20] = 0xF0;
  memory[0x21] = 0x90;
  memory[0x22] = 0xF0;

  // 7
  memory[0x23] = 0xF0;
  memory[0x24] = 0x10;
  memory[0x25] = 0x20;
  memory[0x26] = 0x40;
  memory[0x27] = 0x40;

  // 8
  memory[0x28] = 0xF0;
  memory[0x29] = 0x90;
  memory[0x2A] = 0xF0;
  memory[0x2B] = 0x90;
  memory[0x2C] = 0xF0;

  // 9
  memory[0x2D] = 0xF0;
  memory[0x2E] = 0x90;
  memory[0x2F] = 0xF0;
  memory[0x30] = 0x10;
  memory[0x31] = 0xF0;

  // A
  memory[0x32] = 0xF0;
  memory[0x33] = 0x90;
  memory[0x34] = 0xF0;
  memory[0x35] = 0x90;
  memory[0x36] = 0x90;

  // B
  memory[0x37] = 0xE0;
  memory[0x38] = 0x90;
  memory[0x39] = 0xE0;
  memory[0x3A] = 0x90;
  memory[0x3B] = 0xE0;

  // C
  memory[0x3C] = 0xF0;
  memory[0x3D] = 0x80;
  memory[0x3E] = 0x80;
  memory[0x3F] = 0x80;
  memory[0x40] = 0xF0;

  // D
  memory[0x41] = 0xE0;
  memory[0x42] = 0x90;
  memory[0x44] = 0x90;
  memory[0x43] = 0x90;
  memory[0x45] = 0xE0;

  // E
  memory[0x46] = 0xF0;
  memory[0x47] = 0x80;
  memory[0x48] = 0xF0;
  memory[0x49] = 0x80;
  memory[0x4A] = 0xF0;

  // F
  memory[0x4B] = 0xF0;
  memory[0x4C] = 0x80;
  memory[0x4D] = 0xF0;
  memory[0x4E] = 0x80;
  memory[0x4F] = 0x80;
}
