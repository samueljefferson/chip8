// needs to be run with sudo

#include <stdio.h>
#include <ncurses.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <linux/input.h>

int compare(int num, int check)
{
  return (num >> check-1) & 0x0001;
}

int drawScreen(int keyboard)
{
  int i = 0;
  for (i = 0; i < 16; i++)
  {
    // printw("%d 0x%04x\n", i, (keyboard>>i)&1);

    // printw("%d %d\n", i, (keyboard>>i)&1);


    switch(i)
    {
      case 0:
        (keyboard>>i)&1 == 1 ? mvprintw(3,1, "#") : mvprintw(3,1, "%d", i);
        break;
      case 1:
        (keyboard>>i)&1 == 1 ? mvprintw(0,0, "#") : mvprintw(0,0, "%d", i);
        break;
      case 2:
        (keyboard>>i)&1 == 1 ? mvprintw(0,1, "#") : mvprintw(0,1, "%d", i);
        break;
      case 3:
        (keyboard>>i)&1 == 1 ? mvprintw(0,2, "#") : mvprintw(0,2, "%d", i);
        break;
      case 4:
        (keyboard>>i)&1 == 1 ? mvprintw(1,0, "#") : mvprintw(1,0, "%d", i);
        break;
      case 5:
        (keyboard>>i)&1 == 1 ? mvprintw(1,1, "#") : mvprintw(1,1, "%d", i);
        break;
      case 6:
        (keyboard>>i)&1 == 1 ? mvprintw(1,2, "#") : mvprintw(1,2, "%d", i);
        break;
      case 7:
        (keyboard>>i)&1 == 1 ? mvprintw(2,0, "#") : mvprintw(2,0, "%d", i);
        break;
      case 8:
        (keyboard>>i)&1 == 1 ? mvprintw(2,1, "#") : mvprintw(2,1, "%d", i);
        break;
      case 9:
        (keyboard>>i)&1 == 1 ? mvprintw(2,2, "#") : mvprintw(2,2, "%d", i);
        break;
      case 10:
        (keyboard>>i)&1 == 1 ? mvprintw(3,0, "#") : mvprintw(3,0, "%X", i);
        break;
      case 11:
        (keyboard>>i)&1 == 1 ? mvprintw(3,2, "#") : mvprintw(3,2, "%X", i);
        break;
      case 12:
        (keyboard>>i)&1 == 1 ? mvprintw(0,3, "#") : mvprintw(0,3, "%X", i);
        break;
      case 13:
        (keyboard>>i)&1 == 1 ? mvprintw(1,3, "#") : mvprintw(1,3, "%X", i);
        break;
      case 14:
        (keyboard>>i)&1 == 1 ? mvprintw(2,3, "#") : mvprintw(2,3, "%X", i);
        break;
      case 15:
        (keyboard>>i)&1 == 1 ? mvprintw(3,3, "#") : mvprintw(3,3, "%X", i);
        break;
      default:
        break;
    }
  }
}

int main()
{
  // set up ncurses
  initscr();
	raw();
  curs_set(0);    // hide the cursor
  noecho(); // don't display getch to screen

  int keyboard = 0;
  drawScreen(keyboard);

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
  // mvprintw(10,0, "start");
  char *dev = "/dev/input/by-path/platform-i8042-serio-0-event-kbd";
  struct input_event ev;
  ssize_t n;
  int fd;
  fd = open(dev, O_RDONLY);
  // mvprintw(11,0, "trace");
  while (1)
  {
    refresh();
    // get key events
    n = read(fd, &ev, sizeof ev);
    int temp = -1;

    if (ev.type == EV_KEY && ev.value >= 0 && ev.value <= 2)
    {
      if ((int)ev.code == 1)
      {
        break;
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
      drawScreen(keyboard);
      // mvprintw(13,0, "                     ");
      // mvprintw(13,0, "%d          keyboard", keyboard);
      // mvprintw(14,0, "        ");
      // mvprintw(14,0, "%d", temp);
    }
    // 0 is released
    // 1 is pressed
    // drawScreen(keyboard);

  }

  getch();
  endwin();
  return 0;
}
