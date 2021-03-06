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
#include <errno.h>
#include <signal.h>

#include "chip8.h"
#define ENTER 28
#define SPACE 57

int show_reg = 0;
int step_mode = 0;
int log_mode = 0;
int instr_count = 0;
long instr_delay = 1250000;              // default value of delay, in nanoseconds

FILE *fp;                      // file pointer for log file

unsigned char memory[4096];    // 4096 bytes of memory
unsigned char V[16];           // 16 8 bit registers
unsigned short I;              // 16 bit register
unsigned short PC = 0x200;     // program counter is 16 bit
char SP;                       // SP is 8 bit
char stack_pointer;
unsigned short stack[16];      // stack in 16 16 bit values
int DT = 0;                    // delay timer
int ST = 0;                    // sound timer

int keyboard = 0;              // contains flags for all keys from 0-F
int errno;
int end_program = 0;          // set to 1 by the signal handler

int main(int argc, char** argv) {
  signal(SIGINT, signal_handler);

  int i = 0;
  // process command line arguments
  if (argc > 2) {
    for (i = 2; i < argc; i++) {
      if (strlen(argv[i]) > 6) {
        // compare the first 5 chars and check for delay
        char pattern[] = "delay=";
        char *match;

        match = strstr(argv[i], pattern);

        if (match) {
            // get parts of string after delay=
            int len = strlen(argv[i]) - 5; // +1 to include \0
            char delay_str[len];
            delay_str[len - 1] = '\0';
            int j;

            for (j=6; j < strlen(argv[i]); j++) {
              delay_str[j-6] = argv[i][j];
            }
            instr_delay = atoi(delay_str);
        }
      } else if (!strcmp(argv[i], "reg")) {
        show_reg = 1;
      } else if (!strcmp(argv[i], "step")) {
        step_mode = 1;
      } else if (!strcmp(argv[i], "log")) {
        log_mode = 1;
      }
    }
  }

  //create log file
  if (log_mode) {
    printf("log mode on\n");
    fp = fopen("log.txt", "w");
    fprintf(fp, "start of log file\n");

  }

  memset(memory, 0, 4096);
  setASCII();
  memset(V, 0, 16);
  I = 0;
  memset(stack, 0, 32);
  srand(time(NULL));  // needs a seed for random

  if (argc < 2) {
    fprintf(stderr, "error: requires a rom\n");
    exit(0);
  }

  char* fileName = argv[1];

  // setup ncurses
  initscr();								// initialize screen
  raw();										// raw mode
  noecho();                 // key presses aren't put on screen
  curs_set(0);              // hide cursor

  // load the rom
  loadRom(fileName);

  // create a thread for reading keyboard input
  pthread_t kb_id;
  pthread_create(&kb_id, NULL, kb_input, NULL);

  // create a thread for timers
  pthread_t timer_id;
  pthread_create(&timer_id, NULL, timer_thread, NULL);

  int currentInstr;

  while (!end_program) {

    // get next instruction
    currentInstr = (memory[PC] << 8) | memory[PC+1];

    // show register information
    if (show_reg == 1) {
      mvprintw(35,0, "                                                                               ");
      mvprintw(36,0, "                                                         ");
      mvprintw(37,0, "                              ");
      mvprintw(38,0, "                              ");
      mvprintw(39,0, "                              ");

      mvprintw(36,0, "Instr=%X", currentInstr);
      if (step_mode) {
        mvprintw(36,16, "step mode on ");
      } else {
        mvprintw(36,16, "step mode off");
      }
      mvprintw(36,32, "instr:%d", instr_count);

      drawScreen(keyboard, 38, 20);

      mvprintw(37,0, "PC %X", PC);
      mvprintw(37,10, "SP %X", stack_pointer);
      mvprintw(38,0, "I %X", I);
      mvprintw(38,10, "     ");
      mvprintw(38,10, "DT %X", DT);
      mvprintw(39,10, "     ");
      mvprintw(39,10, "ST %X", ST);
      int col = 39;
      for (i = 0; i < 16; i++) {
        mvprintw(39+i,0, "      ");
        mvprintw(39+i,0, "V%X %X", i, V[i]);
      }
      refresh();

    }
    processInstr(currentInstr);

    // TODO figure out if log file should be kept
    if (log_mode) {
      // output screen info to log here
      char displayedChar;
      char string[64];
      int i;
      // TODO use memset here
      for (i = 0; i < 64; i++) {
        string[i] = ' ';
      }
      string[63] = '\0';

      for (i = 0; i < 63; i++) {
        displayedChar = mvinch(35, i);
        if (displayedChar > 31 && displayedChar < 126) {
          string[i] = displayedChar;
        } else {
          string[i] = ' ';
        }
      }
      fprintf(fp, "%d: %s\n", instr_count, string);
      instr_count++;
    }

    // TODO check to see how long instructions are supposed to last

    // sleep a little after the instruction is finished
    // instr_delay is measured in nanoseconds
    struct timespec ts = {0, instr_delay};
    nanosleep(&ts, NULL);

    if (step_mode) {
      // if in step mode don't continue until there is a key press
      getch();
    }
  }

  endwin();
  if (log_mode) {
    // close log file
    fclose(fp);
    // change file permissions for log file
    chmod("log.txt", 0777);
  }
  pthread_join(kb_id, NULL);
  pthread_join(timer_id, NULL);
  // TODO clear input from terminal
  return 0;
}

void signal_handler (int sig) {
  end_program = 1;
}

void* timer_thread() {
  // Decrement DT and ST 60 times a second if they don't equal 0
  long sixty_hz = 1000000000 / 60;
  struct timespec ts = {0, sixty_hz};

  while (!end_program) {
    nanosleep(&ts, NULL);   // sleep for 1/60 of a second
    if (DT != 0) {
      DT--;
    }
    if (ST != 0) {
      printf("\a");
      ST--;
    }
  }
  return NULL;
}

int processInstr(int instr) {
  // convert the instruction

  int nnn = 0;
  int n = 0;
  int kk = 0;

  int first  = instr >> 12;           // first nibble
  int fourth = instr & 0x000F;        // second nibble
  int threeFour = instr & 0x00FF;     // third and fourth nibble

  switch (first) {
    case 0:
      if (threeFour == 0x00E0) {
        I_00E0(instr);
      } else if (threeFour == 0x00EE) {
        I_00EE(instr);
      }

      // TODO SYS addr
      break;

    case 1:
      I_1nnn(instr);
      break;

    case 2:
      I_2nnn(instr);
      break;

    case 3:
      I_3xkk(instr);
      break;

    case 4:
      I_4xkk(instr);
      break;

    case 5:
      I_5xy0(instr);
      break;

    case 6:
      I_6xkk(instr);
      break;

    case 7:
      I_7xkk(instr);
      break;

    case 8:
      if (fourth == 0x0000) {
        I_8xy0(instr);
      } else if (fourth == 0x0001) {
        I_8xy1(instr);
      } else if (fourth == 0x0002) {
        I_8xy2(instr);
      } else if (fourth == 0x0003) {
        I_8xy3(instr);
      } else if (fourth == 0x0004) {
        I_8xy4(instr);
      } else if (fourth == 0x0005) {
        I_8xy5(instr);
      } else if (fourth == 0x0006) {
        I_8xy6(instr);
      } else if (fourth == 0x0007) {
        I_8xy7(instr);
      } else if (fourth == 0x000E) {
        I_8xyE(instr);
      }
      break;

    case 9:
      I_9xy0(instr);
      break;

    case 10:
      I_Annn(instr);
      break;

    case 11:
      I_Bnnn(instr);
      break;

    case 12:
      I_Cxkk(instr);
      break;

    case 13:
      I_Dxyn(instr);
      break;

    case 14:
      if (fourth == 0x000E) {
        I_Ex9E(instr);
      } else {
        I_ExA1(instr);
      }
      break;

    case 15:
      if (threeFour == 0x0007) {
        I_Fx07(instr);
      } else if (threeFour == 0x000A) {
        I_Fx0A(instr);
      } else if (threeFour == 0x0015) {
        I_Fx15(instr);
      } else if (threeFour == 0x0018) {
        I_Fx18(instr);
      } else if (threeFour == 0x001E) {
        I_Fx1E(instr);
      } else if (threeFour == 0x0029) {
        I_Fx29(instr);
      } else if (threeFour == 0x0033) {
        I_Fx33(instr);
      } else if (threeFour == 0x0055) {
        I_Fx55(instr);
      } else if (threeFour == 0x0065) {
        I_Fx65(instr);
      }
      break;

    default:
      break;
  }
}

void *kb_input(void *ptr) {
  // setup keyboard stuff
  int kb_convert[256];  // array used to convert ev.code into correct values
  memset(kb_convert, -1, 256);

  kb_convert[2] = 0x0002;     // 1
  kb_convert[3] = 0x0004;     // 2
  kb_convert[4] = 0x0008;     // 3
  kb_convert[5] = 0x1000;     // D

  kb_convert[16] = 0x0010;    // 4
  kb_convert[17] = 0x0020;    // 5
  kb_convert[18] = 0x0040;    // 6
  kb_convert[19] = 0x2000;    // D

  kb_convert[30] = 0x0080;    // 7
  kb_convert[31] = 0x0100;    // 8
  kb_convert[32] = 0x0200;    // 9
  kb_convert[33] = 0x4000;    // A

  kb_convert[44] = 0x0400;    // B
  kb_convert[45] = 0x0001;    // 0
  kb_convert[46] = 0x0800;    // C
  kb_convert[47] = 0x8000;    // F

  char *dev = "/dev/input/by-path/platform-i8042-serio-0-event-kbd";
  struct input_event ev;
  ssize_t n;
  int fd;
  fd = open(dev, O_RDONLY);

  while (!end_program) {
    // get key events
    n = read(fd, &ev, sizeof ev);
    int temp = -1;

    if (ev.type == EV_KEY && ev.value >= 0 && ev.value <= 2) {
      // if Esc is pressed
      if ((int)ev.code == 1) {
        raise(SIGINT);
      }

      // // if a key is pressed
      if (ev.value == 1) {
        if (ev.code == ENTER) {
          // TODO change
          if (step_mode) {
            step_mode = 0;
          } else {
            step_mode = 1;
          }
        }
        temp = kb_convert[(int)ev.code];
        if (temp != -1) {
          keyboard = keyboard | temp;
        }
      }
      // if the key is released
      else if (ev.value == 0) {
        temp = kb_convert[(int)ev.code];
        temp = ~temp;
        if (temp != -1) {
          keyboard = keyboard & temp;
        }
      }
    }
  }

  return NULL;
}

int drawScreen(int keyboard, int y, int x)
{
  int i = 0;
  for (i = 0; i < 16; i++)
  {
    switch(i)
    {
      case 0:
        (keyboard>>i)&1 == 1 ? mvprintw(y+3,x+1, "#") : mvprintw(y+3,x+1, "%d", i);
        break;
      case 1:
        (keyboard>>i)&1 == 1 ? mvprintw(y+0,x+0, "#") : mvprintw(y+0,x+0, "%d", i);
        break;
      case 2:
        (keyboard>>i)&1 == 1 ? mvprintw(y+0,x+1, "#") : mvprintw(y+0,x+1, "%d", i);
        break;
      case 3:
        (keyboard>>i)&1 == 1 ? mvprintw(y+0,x+2, "#") : mvprintw(y+0,x+2, "%d", i);
        break;
      case 4:
        (keyboard>>i)&1 == 1 ? mvprintw(y+1,x+0, "#") : mvprintw(y+1,x+0, "%d", i);
        break;
      case 5:
        (keyboard>>i)&1 == 1 ? mvprintw(y+1,x+1, "#") : mvprintw(y+1,x+1, "%d", i);
        break;
      case 6:
        (keyboard>>i)&1 == 1 ? mvprintw(y+1,x+2, "#") : mvprintw(y+1,x+2, "%d", i);
        break;
      case 7:
        (keyboard>>i)&1 == 1 ? mvprintw(y+2,x+0, "#") : mvprintw(y+2,x+0, "%d", i);
        break;
      case 8:
        (keyboard>>i)&1 == 1 ? mvprintw(y+2,x+1, "#") : mvprintw(y+2,x+1, "%d", i);
        break;
      case 9:
        (keyboard>>i)&1 == 1 ? mvprintw(y+2,x+2, "#") : mvprintw(y+2,x+2, "%d", i);
        break;
      case 10:
        (keyboard>>i)&1 == 1 ? mvprintw(y+3,x+0, "#") : mvprintw(y+3,x+0, "%X", i);
        break;
      case 11:
        (keyboard>>i)&1 == 1 ? mvprintw(y+3,x+2, "#") : mvprintw(y+3,x+2, "%X", i);
        break;
      case 12:
        (keyboard>>i)&1 == 1 ? mvprintw(y+0,x+3, "#") : mvprintw(y+0,x+3, "%X", i);
        break;
      case 13:
        (keyboard>>i)&1 == 1 ? mvprintw(y+1,x+3, "#") : mvprintw(y+1,x+3, "%X", i);
        break;
      case 14:
        (keyboard>>i)&1 == 1 ? mvprintw(y+2,x+3, "#") : mvprintw(y+2,x+3, "%X", i);
        break;
      case 15:
        (keyboard>>i)&1 == 1 ? mvprintw(y+3,x+3, "#") : mvprintw(y+3,x+3, "%X", i);
        break;
      default:
        break;
    }
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
  if (show_reg) {
    mvprintw(35,0, "ICDP1802_0nnn: not implemented yet");
    refresh();
  }
  if (step_mode) {
    getch();
  }
  PC += 2;
}

void I_00E0(int instr) {
  // clear display 64x32
  clear();

  if (show_reg) {
    mvprintw(35,0, "I_00E0: clear display");
    refresh();
  }
  PC += 2;
}

void I_00EE(int instr) {
  // return from subroutine
  // set PC to stack address and decrement SP
  if (show_reg) {
    mvprintw(35,0, "I_00EE: RET, set PC to %X and decrement SP", stack[SP]);
    refresh();
  }

  PC = stack[stack_pointer];
  stack_pointer--;

  // TODO CHECK
  PC += 2;
}

void I_1nnn(int instr) {
  // set PC to nnn
  int nnn = instr & 0x0FFF;
  if (show_reg) {
    mvprintw(35,0, "I_1nnn: JMP, set PC to %X", nnn);
    refresh();
  }
  PC = nnn;
}

void I_2nnn(int instr) {
  // call subroutine at nnn
  int nnn = instr & 0x0FFF;

  // increment stack_pointer
  stack_pointer++;
  // put PC on top of stack
  stack[stack_pointer] = PC;
  // set PC to nnn
  PC = nnn;

  if (show_reg) {
    mvprintw(35,0, "I_2nnn: call subroutine at %X, SP++, stack[%X] = PC(%X)", nnn, stack_pointer, stack[stack_pointer-1]);
    refresh();
  }
}

void I_3xkk(int instr) {
  // skip next instruction if Vx = kk

  // get x
  int x = (instr & 0x0F00) >> 8;
  // get kk
  int kk = (instr & 0x00FF);
  // skip next instruction if Vx = kk
  if (V[x] == kk) {
    PC += 4;
  } else {
    PC += 2;
  }

  if (show_reg) {
    mvprintw(35,0, "I_3xkk: skip next instruction if V[%X](%X) = %X", x, V[x], kk);
    refresh();
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
    PC += 4;
  } else {
    PC += 2;
  }

  if (show_reg) {
    mvprintw(35,0, "I_4xkk: skip next instruction if V%X(%X) != %X", x, V[x], kk);
    refresh();
  }
}

void I_5xy0(int instr) {
  // skip next instruction in Vx = Vy
  int x = (instr & 0x0F00) >> 8;
  int y = (instr & 0x00F0) >> 4;
  if (V[x] == V[y]) {
    PC += 4;
  } else {
    PC += 2;
  }
  if (show_reg) {
    mvprintw(35,0, "I_5xy0: skip next instruction if V%x = V%x (%X = %X)", x, y, V[x], V[y]);
    refresh();
  }
}

void I_6xkk(int instr) {
  // set Vx = kk
  int x = (instr & 0x0F00) >> 8;
  int kk = instr & 0x00FF;

  if (show_reg) {
    mvprintw(35,0, "I_6xkk: set V[%X] from %X to %X\n", x, V[x], kk);
    refresh();
  }

  V[x] = kk;
  PC += 2;
}

void I_7xkk(int instr) {
  // set Vx += kk
  int x = (instr & 0x0F00) >> 8;
  int kk = instr & 0x00FF;
  if (show_reg) {
    mvprintw(35,0, "I_7xkk: set V[%X] = %X + %X = %X", x, V[x], kk, (V[x]+kk)%255);
    refresh();
  }
  V[x] += kk;

  // TODO check instruction, if V[x] > 255 then mod it with 255
  // V[x] = V[x] & 0x00FF;
  V[x] = V[x] % 0x00FF;
  PC += 2;
}

void I_8xy0(int instr) {
  // Vx = Vy
  int x = (instr & 0x0F00) >> 8;
  int y = (instr & 0x00F0) >> 4;
  V[x] = V[y];
  PC += 2;
}

void I_8xy1(int instr) {
  // Vx = Vx | Vy
  int x = (instr & 0x0F00) >> 8;
  int y = (instr & 0x00F0) >> 4;
  V[x] = V[x] | V[y];

  if (show_reg) {
    mvprintw(35,0, "I_8xy1: set V%X = V%X | V%X", x, x, y);
    refresh();
  }
  PC += 2;
}

void I_8xy2(int instr) {
  // Vx = Vx & Vy
  int x = (instr & 0x0F00) >> 8;
  int y = (instr & 0x00F0) >> 4;
  V[x] = V[x] & V[y];

  if (show_reg) {
    mvprintw(35,0, "I_8xy2: V%x = V%x & V%x", x, x, y);
    refresh();
  }
  PC += 2;
}

void I_8xy3(int instr) {
  // Vx = Vx ^ Vy (XOR)
  int x = (instr & 0x0F00) >> 8;
  int y = (instr & 0x00F0) >> 4;
  V[x] = V[x] ^ V[y];
  if (show_reg) {
    mvprintw(35,0, "I_8xy3: V%X = V%X ^ V%X", x, x, y);
    refresh();
  }
  PC += 2;
}

void I_8xy4(int instr) {
  // Vx += Vy
  // VF is 1 if carry
  int x = (instr & 0x0F00) >> 8;
  int y = (instr & 0x00F0) >> 4;
  int sum = V[x] + V[y];
  if (show_reg) {
    mvprintw(35,0, "I_8xy4: add %X to %X, if %X is greater than 255 set Vf to 1", x, y, sum);
    refresh();
  }
  V[x] = sum & 0x00FF;  // TODO CHECK
  // if (sum > 0xFF) {
  //   V[15] = 1;
  // } else {
  //   V[15] = 0;
  // }
  V[15] = (sum > 0xFF) ? 1 : 0;
  PC += 2;
}

void I_8xy5(int instr) {
  // Vx -= Vy
  // if Vx > Vy then VF = 0
  int x = (instr & 0x0F00) >> 8;
  int y = (instr & 0x00F0) >> 4;
  // if (V[x] > V[y]) {
  //   V[15] = 1;
  // } else {
  //   V[15] = 0;
  // }
  V[15] = (V[x] > V[y]) ? 1 : 0;
  V[x] -= V[y];
  V[x] = V[x] & 0x00FF;   // TODO CHECK

  if (show_reg) {
    mvprintw(35,0, "I_8xy5: V%X -= V%X, if V%x > V%X then VF=0", x, y, x, y);
    refresh();
  }
  PC += 2;
}

void I_8xy6(int instr) {
  // shift right
  // if the smallest bit in Vx is 1 then VF = 1
  // otherwise VF = 0
  // then shift Vx 1 right
  int x = (instr & 0x0F00) >> 8;
  int bit = V[x] & 0x01;
  if (bit) {
    V[15] = 1;
  } else {
    V[15] = 0;
  }
  V[x] = V[x] >> 1;
  V[x] = V[x] & 0x00FF;   // TODO CHECK

  if (show_reg) {
    mvprintw(35,0, "I_8xy6: V%X >> 1, if first digit is 1 set VF=1 else VF=0", x);
    refresh();
  }
  PC += 2;
}

void I_8xy7(int instr) {
  // Vx = Vy - Vx
  // if Vy > Vx then Vf = 1, else Vf = 0
  int x = (instr & 0x0F00) >> 8;
  int y = (instr & 0x00F0) >> 4;
  if (V[y] > V[x]) {
    V[15] = 1;
  } else {
    V[15] = 0;
  }
  V[x] = V[y] - V[x];
  V[x] = V[x] & 0x00FF;   // TODO CHECK

  if (show_reg) {
    mvprintw(35,0, "I_8xy7: V%X = V%X - V%X, if V%X>V%X then VF=1, else VF=0", x, y, x, y, x);
    refresh();
  }
  PC += 2;
}

void I_8xyE(int instr) {
  // shift Vx left 1
  // if left most bit is 1 then VF = 1, else VF = 0
  int x = (instr & 0x0F00) >> 8;
  int bit = V[x] & 0x80;
  if (bit) {
    V[15] = 1;
  } else {
    V[15] = 0;
  }
  V[x] = V[x] << 1;
  V[x] = V[x] & 0x00FF;   // TODO CHECK

  if (show_reg) {
    mvprintw(35,0, "I_8xyE: V%X << 1, if left most bit is 1 then VF = 1, else VF = 0 ", x);
    refresh();
  }
  PC += 2;
}

void I_9xy0(int instr) {
  // skip next instruction if Vx != Vy
  int x = (instr & 0x0F00) >> 8;
  int y = (instr & 0x00F0) >> 4;

  if (show_reg) {
    mvprintw(35,0, "I_9xy0: skip next instruction if V%x(%X) != V%x(%X)", x, V[x], y, V[y]);
    refresh();
  }

  if (V[x] != V[y]) {
    PC += 4;
  } else {
    PC += 2;
  }
}

void I_Annn(int instr) {
  // set I to nnn
  // instr -= 40960;
  int nnn = instr & 0x0FFF;

  if (show_reg) {
    mvprintw(35,0, "I_Annn: set I from %X to %X\n", I, nnn);
    refresh();
  }

  I = nnn;
  PC += 2;
}

void I_Bnnn(int instr) {
  // PC = V0 + nnn
  int nnn = instr & 0x0FFF;
  PC = V[0] + nnn;

  if (show_reg) {
    mvprintw(35,0, "I_Bnnn: PC = V0 + %X", nnn);
    refresh();
  }
}

void I_Cxkk(int instr) {
  // Vx = random byte & kk
  int random = rand() % 0xFF;
  int x = (instr & 0x0F00) >> 8;
  int kk = instr & 0x00FF;

  if (show_reg) {
    mvprintw(35,0, "I_Cxkk: V%x = %X & %X (%X)", x, random, kk, random & kk);
    refresh();
  }
  V[x] = random & kk;
  PC += 2;
}

void I_Dxyn(int instr) {
  // display n bytes, starting at memory[I]
  // start at coordinates Vx, Vy (reverse for ncurses)
  // each sprite is 8 wide and up to 15 long
  int x = (instr & 0x0F00) >> 8;
  int y = (instr & 0x00F0) >> 4;
  int n = (instr & 0x000F);
  int i = 0;
  int j = 0;
  int byte = 0;
  int bit = 0;
  char displayedChar = ' ';
  int displayedBit = 0;    // is the pixel currently being displayed?
  chtype filled = ' ' | A_REVERSE;  // filled in character
  chtype blank = ' ';               // empty character

  x = V[x];
  y = V[y];

  if (show_reg == 1) {
    mvprintw(35,0, "I_Dxyn: display %X-byte sprite starting at (V%X, V%X)", n, x, y);
    refresh();
  }

  // set VF to 0
  V[15] = 0;
  // display each row
  if (show_reg == 1) {
    mvprintw(38,30, "byte");
    for (i = 0; i < 8; i++) {
      mvprintw(39+i,40, "                          ");
    }
    refresh();
  }
  for (i = 0; i < n; i++) {
    byte = memory[I + i];
    if (show_reg == 1) {
      // display the byte
      mvprintw(39+i,30, "             ");
      mvprintw(39+i,30, "%.4x", byte);
    }
    // display each column

    for (j = 0; j < 8; j++) {

      // find out if the pixel currently has anything displayed
      // y,x
      int row = y+i;
      int col = x+j;

      if (mvinch(row, col) == filled) {
        displayedBit = 1;
      } else {
        displayedBit = 0;
      }

      // mask for correct bit of j
      // take 1 and rotate it 7-j bits left to get the mask
      // so for j=0 do 1<<7=8 so you have a mask for the first bit
      // then and it with the byte
      // then rotate this 7-j bits right
      // for first bit (if its 1) 0x80>>7=1
      bit = ((0x01 << (7-j)) & byte) >> (7-j);

      // XOR bit with displayedBit
      bit = bit ^ displayedBit;

      if (bit) {
        // display bit
        mvaddch(row, col, filled);
      } else {
        // if bit was displayed before and is being erased set VF to 1
        if (displayedBit == 1) {
          V[15] = 1;
        }
        // display nothing
        mvaddch(row, col, blank);
      }
      refresh();
      if (step_mode) {
        getch();
      }
    }
  }
  PC += 2;
}

void I_Ex9E(int instr) {
  // skip next instruction if key == Vx
  int x = (instr & 0x0F00) >> 8;      // get x from instruction
  int key = V[x];                     // get key to check from Vx
  int key_mask = 0x01 << key;         // get keyboard mask in correct position
  int check = keyboard & key_mask;    // check it see if the key was pressed

  if (check) {                        // key was pressed, skip next instr
    PC += 4;
  } else {
    PC += 2;                          // key wasn't pressed, don't skip next instr
  }

  if (show_reg) {
    if (check) {
      mvprintw(35,0, "I_ExA1: skip next instruction if key %X is being pressed, it WAS pressed", key);
    } else {
      mvprintw(35,0, "I_ExA1: skip next instruction if key %X is being pressed, it WASN't pressed", key);
    }
    refresh();
  }
}

void I_ExA1(int instr) {
  // skip next instruction if key != Vx

  int x = (instr & 0x0F00) >> 8;      // get x from instruction
  int key = V[x];                     // get key to check from Vx
  int key_mask = 0x01 << key;         // get keyboard mask in correct position
  int check = keyboard & key_mask;    // check it see if the key was pressed

  if (check) {                        // key was pressed, don't skip next instr
    PC += 2;
  } else {
    PC += 4;                          // key wasn't pressed, skip next instr
  }

  if (show_reg) {
    if (check) {
      mvprintw(35,0, "I_ExA1: skip next instruction if key %X isn't being pressed, it WAS pressed", key);
    } else {
      mvprintw(35,0, "I_ExA1: skip next instruction if key %X isn't being pressed, it WASN't pressed", key);
    }
    refresh();
  }
}

void I_Fx07(int instr) {
  // set Vx = delay timer value
  int x = (instr & 0x0F00) >> 8;
  if (show_reg) {
    mvprintw(35,0, "I_Fx07: V%x = DT(%X)", x, DT);
    refresh();
  }
  V[x] = DT;
  PC += 2;
}

void I_Fx0A(int instr) {
  // wait for a key press and store it in Vx
  int x = (instr & 0x0F00) >> 8;

  if (show_reg) {
    mvprintw(35,0, "I_Fx0A: Wait for a key press and store it in V[%x]", x);
    refresh();
  }

  char key = getch();
  char num = processKey(key);
  V[x] = num;
  if (show_reg) {
    int i = 0;
    for (i = 0; i < 16; i++) {
      mvprintw(39+i,0, "V%X %X", i, V[i]);
    }

    mvprintw(35,0, "I_Fx0A: Storing %X in V[%x], V[%x]=%X                 ", num, x, x, V[x]);
    refresh();
  }
  PC += 2;
}

void I_Fx15(int instr) {
  // set delay timer = Vx
  int x = (instr & 0x0F00) >> 8;
  DT = V[x];

  if (show_reg) {
    mvprintw(35,0, "I_Fx15: DT = V%x (%X)", x, V[x]);
    refresh();
  }
  // getch();
  PC += 2;
}

void I_Fx18(int instr) {
  // set sound timer = Vx
  int x = (instr & 0x0F00) >> 8;
  ST = V[x];

  if (show_reg) {
    mvprintw(35,0, "I_Fx18: ST = V%x (%X)", x, V[x]);
    refresh();
  }
  if (step_mode) {
    getch();
  }

  PC += 2;
}

void I_Fx1E(int instr) {
  // I += Vx
  int x = (instr & 0x0F00) >> 8;

  if (show_reg) {
    // mvprintw(35,0, "I_Fx1E: I = I + V%x: %X = %X + %X", x, I+x, I, x);
    // mvprintw(35,0, "I_Fx1E: I = I + V%x: %X = %X + %X", x, I+x, I, V[x]);
    mvprintw(35,0, "I_Fx1E: I = I + V[%x] is %X = %X + %X", x, I+V[x], I, V[x]);
    refresh();
  }

  I += V[x];

  // TODO check if this is correct
  // set VF=1 if I > 0x0FFF
  // if (I > 0x0FFF) {
  //   V[15] = 1;
  // } else {
  //   V[15] = 0;
  // }

  PC += 2;

}

void I_Fx29(int instr) {
  // set I = location of sprite for digit Vx
  // aka where sprites for 0-F are in memory
  int x = (instr & 0x0F00) >> 8;
  x = V[x];
  I = x*5;

  if (show_reg) {
    mvprintw(35,0, "I_Fx29: I = V[%x]=%X, %X*5 = %X", x, V[x], V[x], V[x]*5);
    refresh();
  }
  PC += 2;

}

void I_Fx33(int instr) {
  // store BCD value of Vx in
  // memory[I], memory[I + 1], memory[I + 2]
  int x = (instr & 0x0F00) >> 8;
  x = V[x];

  // ones
  memory[I+2] = x % 10;

  // tens
  memory[I+1] = (x / 10) % 10;

  // hundreds
  memory[I] = x / 100;

  if (show_reg) {
    mvprintw(35,0, "I_Fx33: I = BCD of V%x(%X) BCD=%X,%X,%X", x, V[x], memory[I], memory[I+1], memory[I+2]);
    refresh();
  }
  PC += 2;

}

void I_Fx55(int instr) {
  // stores registers V0 to Vx in memory
  // starting at memory[I]
  int x = (instr & 0x0F00) >> 8;
  int i = 0;
  // for (i = 0; i < x; i++) {
  for (i = 0; i <= x; i++) {
    memory[I + i] = V[i];

  }

  // TODO check instruction
  I = I + x + 1;

  if (show_reg) {
    mvprintw(35,0, "I_Fx55: store registers V0-V%x in memory, starting at %X", x, I);
    refresh();
  }
  // getch();
  PC += 2;
}

void I_Fx65(int instr) {
  // fills registers V0 to Vx from memory
  // starting from location memory[I]

  int x = (instr & 0x0F00) >> 8;
  int i = 0;
  for (i = 0; i <= x; i++) {
    V[i] = memory[I + i];
  }
  int old_I = I;

  I = I + x + 1;

  if (show_reg) {
    mvprintw(35,0, "I_Fx65: fill registers V0-V%x from memory, starting at %X", x, old_I);
    refresh();
  }
  // getch();

  PC += 2;
}

int readInstr() {
  // read instruction from memory
  int instr = memory[PC];   // get first byte
  instr = instr << 8;       // rotate left 8 bits to get room for 2nd byte
  // instr += memory[PC + 1];  // get second byte
  instr = instr | memory[PC+1]; // get second byte
  //printf("%x\n", instr);
  char n1 = (memory[PC] & 0x00F0) >> 4;
  char n2 = memory[PC] & 0x000F;
  char n3 = (memory[PC+1] & 0x00F0) >> 4;
  char n4 = memory[PC+1] & 0x000F;
  int instr2 = n1 << 12;
  instr2 = instr | (n2 << 8);
  instr2 = instr | (n3 << 4);
  instr2 = instr | (n4);
  if (show_reg) {
    mvprintw(15,0, "              ");
    mvprintw(16,0, "              ");
    mvprintw(17,0, "              ");
    mvprintw(18,0, "              ");
    mvprintw(19,0, "              ");
    mvprintw(20,0, "              ");
    mvprintw(15,0, "n1=%X", n1);
    mvprintw(16,0, "n2=%X", n2);
    mvprintw(17,0, "n3=%X", n3);
    mvprintw(18,0, "n4=%X", n4);
    mvprintw(19,0, "comb=%X", instr2);
    mvprintw(20,0, "comb=%d", instr2);
    refresh();
  }

  PC += 2;
  return instr2;
}

void loadRom(char* fileName) {
  // TODO make sure rom is loaded before new threads are created

  // make sure filename ends in .ch8
  char *ext = fileName + strlen(fileName) - 4;
  if (strcmp(ext, ".ch8")) {
    endwin();
    printf("error: file doesn't have .ch8 extension\n");
    exit(0);
  }

  int fd = open(fileName, O_RDONLY);
  if (fd == -1) {
    endwin();
    printf("error opening %s: %s\n", fileName, strerror(errno));
    exit(0);
  }


  char buffer[1];
  int current = 512;    // current memory address
  int status = 1;       // equals 0 at end of file

  // loads the program into memory
  while (current < 4096 && status != 0) {
    status = read(fd, buffer, 1);
    memory[current] = buffer[0];
    current++;
  }

  close(fd);
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
