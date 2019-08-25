void signal_handler (int sig);
void* timer_thread();
int processInstr(int instr);
void *kb_input(void *ptr);
int drawScreen(int keyboard, int y, int x);


void loadRom(char* fileName);
int readInstr();
void lookupInstr(int currentInstr);
void setASCII();
unsigned int processKey(char key);

// Instructions
void I_0nnn(int instr);
void I_00E0(int instr);
void I_00EE(int instr);
void I_1nnn(int instr);
void I_2nnn(int instr);
void I_3xkk(int instr);
void I_4xkk(int instr);
void I_5xy0(int instr);
void I_6xkk(int instr);
void I_7xkk(int instr);
void I_8xy0(int instr);
void I_8xy1(int instr);
void I_8xy2(int instr);
void I_8xy3(int instr);
void I_8xy4(int instr);
void I_8xy5(int instr);
void I_8xy6(int instr);
void I_8xy7(int instr);
void I_8xyE(int instr);
void I_9xy0(int instr);
void I_Annn(int instr);
void I_Bnnn(int instr);
void I_Cxkk(int instr);
void I_Dxyn(int instr);
void I_Ex9E(int instr);
void I_ExA1(int instr);
void I_Fx07(int instr);
void I_Fx0A(int instr);
void I_Fx15(int instr);
void I_Fx18(int instr);
void I_Fx1E(int instr);
void I_Fx29(int instr);
void I_Fx33(int instr);
void I_Fx55(int instr);
void I_Fx65(int instr);
