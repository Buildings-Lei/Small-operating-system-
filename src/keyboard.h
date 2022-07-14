
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "type.h"

void KeyboardModInit();
void PutScanCode(byte sc);
void NotifyKeyCode();
void KeyCallHandler(uint cmd, uint param1, uint param2);

#endif
