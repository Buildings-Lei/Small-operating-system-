
#ifndef SCREEN_H
#define SCREEN_H

#include "type.h"

#define SCREEN_WIDTH    80
#define SCREEN_HEIGHT   25

#define ERR_START_W   0
#define ERR_START_H   6
#define CMD_START_W   0
#define CMD_START_H   9
#define TASK_START_W  0
#define TASK_START_H  12

typedef enum
{
    SCREEN_GRAY   = 0x07,
    SCREEN_BLUE   = 0x09,
    SCREEN_GREEN  = 0x0A,
    SCREEN_RED    = 0x0C,
    SCREEN_YELLOW = 0x0E,
    SCREEN_WHITE  = 0x0F
} PrintColor;

void ClearScreen();
int  SetPrintPos(byte w, byte h);
void SetPrintColor(PrintColor c);
int PrintChar(char c);
int PrintString(const char* s);
int PrintIntDec(int n);
int PrintIntHex(uint n);
byte GetPrintPosH();
byte GetPrintPosW();

#endif
