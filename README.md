# Chip-8 emulator
## Only tested in Ubuntu
Run makefile first, then run with sudo ./chip8 romName.ch8

View the registers with sudo ./chip8 romName.ch8 dev

Enable step mode with sudo ./chip8 romName.ch8 step (this won't work with programs that require key presses)

Both options can be combined

sudo is required because the file platform-i8042-serio-0-event-kbd from /dev/input/by-path/ is read to tell if a key is being held down

## Chip-8 assembler
Run with python assembler.py filename.txt
Will convert txt files to Chip-8 roms
