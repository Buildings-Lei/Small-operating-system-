#include "keyboard.h"
#include "utility.h"
#include "queue.h"
#include "event.h"
#include "task.h"

#define KB_BUFF_SIZE   8

typedef struct
{
    uint head;
    uint tail;
    uint count;
    uint max;
    uint buff[KB_BUFF_SIZE];
} KeyCodeBuff;

typedef struct
{
    byte ascii1;   // no shift code
    byte ascii2;   // shift code
    byte scode;    // scan code
    byte kcode;    // key code
} KeyCode;

enum
{
    KeyRelease = 0,
    KeyPress = 0x1000000
};

// 数组下标就是这个扫描码
static const KeyCode gKeyMap[] = 
{
/* 0x00 - none      */ {  0,        0,        0,         0   },
/* 0x01 - ESC       */ {  0,        0,       0x01,      0x1B },
/* 0x02 - '1'       */ { '1',      '!',      0x02,      0x31 },
/* 0x03 - '2'       */ { '2',      '@',      0x03,      0x32 },
/* 0x04 - '3'       */ { '3',      '#',      0x04,      0x33 },
/* 0x05 - '4'       */ { '4',      '$',      0x05,      0x34 },
/* 0x06 - '5'       */ { '5',      '%',      0x06,      0x35 },
/* 0x07 - '6'       */ { '6',      '^',      0x07,      0x36 },
/* 0x08 - '7'       */ { '7',      '&',      0x08,      0x37 },
/* 0x09 - '8'       */ { '8',      '*',      0x09,      0x38 },
/* 0x0A - '9'       */ { '9',      '(',      0x0A,      0x39 },
/* 0x0B - '0'       */ { '0',      ')',      0x0B,      0x30 },
/* 0x0C - '-'       */ { '-',      '_',      0x0C,      0xBD },
/* 0x0D - '='       */ { '=',      '+',      0x0D,      0xBB },
/* 0x0E - BS        */ {  0,        0,       0x0E,      0x08 },
/* 0x0F - TAB       */ {  0,        0,       0x0F,      0x09 },
/* 0x10 - 'q'       */ { 'q',      'Q',      0x10,      0x51 },
/* 0x11 - 'w'       */ { 'w',      'W',      0x11,      0x57 },
/* 0x12 - 'e'       */ { 'e',      'E',      0x12,      0x45 },
/* 0x13 - 'r'       */ { 'r',      'R',      0x13,      0x52 },
/* 0x14 - 't'       */ { 't',      'T',      0x14,      0x54 },
/* 0x15 - 'y'       */ { 'y',      'Y',      0x15,      0x59 },
/* 0x16 - 'u'       */ { 'u',      'U',      0x16,      0x55 },
/* 0x17 - 'i'       */ { 'i',      'I',      0x17,      0x49 },
/* 0x18 - 'o'       */ { 'o',      'O',      0x18,      0x4F },
/* 0x19 - 'p'       */ { 'p',      'P',      0x19,      0x50 },
/* 0x1A - '['       */ { '[',      '{',      0x1A,      0xDB },
/* 0x1B - ']'       */ { ']',      '}',      0x1B,      0xDD },
/* 0x1C - CR/LF     */ {  0,        0,       0x1C,      0x0D },
/* 0x1D - l. Ctrl   */ {  0,        0,       0x1D,      0x11 },
/* 0x1E - 'a'       */ { 'a',      'A',      0x1E,      0x41 },
/* 0x1F - 's'       */ { 's',      'S',      0x1F,      0x53 },
/* 0x20 - 'd'       */ { 'd',      'D',      0x20,      0x44 },
/* 0x21 - 'f'       */ { 'f',      'F',      0x21,      0x46 },
/* 0x22 - 'g'       */ { 'g',      'G',      0x22,      0x47 },
/* 0x23 - 'h'       */ { 'h',      'H',      0x23,      0x48 },
/* 0x24 - 'j'       */ { 'j',      'J',      0x24,      0x4A },
/* 0x25 - 'k'       */ { 'k',      'K',      0x25,      0x4B },
/* 0x26 - 'l'       */ { 'l',      'L',      0x26,      0x4C },
/* 0x27 - ';'       */ { ';',      ':',      0x27,      0xBA },
/* 0x28 - '\''      */ { '\'',     '\"',     0x28,      0xDE },
/* 0x29 - '`'       */ { '`',      '~',      0x29,      0xC0 },
/* 0x2A - l. SHIFT  */ {  0,        0,       0x2A,      0x10 },
/* 0x2B - '\'       */ { '\\',     '|',      0x2B,      0xDC },
/* 0x2C - 'z'       */ { 'z',      'Z',      0x2C,      0x5A },
/* 0x2D - 'x'       */ { 'x',      'X',      0x2D,      0x58 },
/* 0x2E - 'c'       */ { 'c',      'C',      0x2E,      0x43 },
/* 0x2F - 'v'       */ { 'v',      'V',      0x2F,      0x56 },
/* 0x30 - 'b'       */ { 'b',      'B',      0x30,      0x42 },
/* 0x31 - 'n'       */ { 'n',      'N',      0x31,      0x4E },
/* 0x32 - 'm'       */ { 'm',      'M',      0x32,      0x4D },
/* 0x33 - ','       */ { ',',      '<',      0x33,      0xBC },
/* 0x34 - '.'       */ { '.',      '>',      0x34,      0xBE },
/* 0x35 - '/'       */ { '/',      '?',      0x35,      0xBF },
/* 0x36 - r. SHIFT  */ {  0,        0,       0x36,      0x10 },
/* 0x37 - '*'       */ { '*',      '*',      0x37,      0x6A },
/* 0x38 - ALT       */ {  0,        0,       0x38,      0x12 },
/* 0x39 - ' '       */ { ' ',      ' ',      0x39,      0x20 },
/* 0x3A - CapsLock  */ {  0,        0,       0x3A,      0x14 },
/* 0x3B - F1        */ {  0,        0,       0x3B,      0x70 },
/* 0x3C - F2        */ {  0,        0,       0x3C,      0x71 },
/* 0x3D - F3        */ {  0,        0,       0x3D,      0x72 },
/* 0x3E - F4        */ {  0,        0,       0x3E,      0x73 },
/* 0x3F - F5        */ {  0,        0,       0x3F,      0x74 },
/* 0x40 - F6        */ {  0,        0,       0x40,      0x75 },
/* 0x41 - F7        */ {  0,        0,       0x41,      0x76 },
/* 0x42 - F8        */ {  0,        0,       0x42,      0x77 },
/* 0x43 - F9        */ {  0,        0,       0x43,      0x78 },
/* 0x44 - F10       */ {  0,        0,       0x44,      0x79 },
/* 0x45 - NumLock   */ {  0,        0,       0x45,      0x90 },
/* 0x46 - ScrLock   */ {  0,        0,       0x46,      0x91 },
/* 0x47 - Home      */ {  0,        0,       0x47,      0x24 },
/* 0x48 - Up        */ {  0,        0,       0x48,      0x26 },
/* 0x49 - PgUp      */ {  0,        0,       0x49,      0x21 },
/* 0x4A - '-'       */ {  0,        0,       0x4A,      0x6D },
/* 0x4B - Left      */ {  0,        0,       0x4B,      0x25 },
/* 0x4C - MID       */ {  0,        0,       0x4C,      0x0C },
/* 0x4D - Right     */ {  0,        0,       0x4D,      0x27 },
/* 0x4E - '+'       */ {  0,        0,       0x4E,      0x6B },
/* 0x4F - End       */ {  0,        0,       0x4F,      0x23 },
/* 0x50 - Down      */ {  0,        0,       0x50,      0x28 },
/* 0x51 - PgDown    */ {  0,        0,       0x51,      0x22 },
/* 0x52 - Insert    */ {  0,        0,       0x52,      0x2D },
/* 0x53 - Del       */ {  0,        0,       0x53,      0x2E },
/* 0x54 - Enter     */ {  0,        0,       0x54,      0x0D },
/* 0x55 - ???       */ {  0,        0,        0,         0   },
/* 0x56 - ???       */ {  0,        0,        0,         0   },
/* 0x57 - F11       */ {  0,        0,       0x57,      0x7A },  
/* 0x58 - F12       */ {  0,        0,       0x58,      0x7B },
/* 0x59 - ???       */ {  0,        0,        0,         0   },
/* 0x5A - ???       */ {  0,        0,        0,         0   },
/* 0x5B - Left Win  */ {  0,        0,       0x5B,      0x5B },	
/* 0x5C - Right Win */ {  0,        0,       0x5C,      0x5C },
/* 0x5D - Apps      */ {  0,        0,       0x5D,      0x5D },
/* 0x5E - Pause     */ {  0,        0,       0x5E,        }
};

static KeyCodeBuff gKCBuff = {0};

// 等待键盘输入的任务。
static Queue gKeyWait = {0};

static uint FetchKeyCode()
{
    uint ret = 0;
    
    if( gKCBuff.count > 0 )
    {
        uint* p = AddrOff(gKCBuff.buff, gKCBuff.head);
        
        ret = *p;
        
        gKCBuff.head = (gKCBuff.head + 1) % gKCBuff.max;
        gKCBuff.count--;
    }
    
    return ret;
}

static void StoreKeyCode(uint kc)
{
    uint* p = NULL;
    
    if( gKCBuff.count < gKCBuff.max )
    {
        p = AddrOff(gKCBuff.buff, gKCBuff.tail);
        
        *p = kc;
        
        gKCBuff.tail = (gKCBuff.tail + 1) % gKCBuff.max;
        gKCBuff.count++;
    }
    else if( gKCBuff.count > 0 )
    {
        FetchKeyCode();
        StoreKeyCode(kc);
    }
}
// 按键是按下了还是释放了
static uint KeyType(byte sc)
{
    uint ret = KeyPress;
    
    if( sc & 0x80 )
    {
        ret = KeyRelease;
    }
    
    return ret;
}

// 按下的是否为Shift键
static uint IsShift(byte sc)
{
    return (sc == 0x2A) || (sc == 0x36);
}
// 按下的是否为大小写的字母键
static uint IsCapsLock(byte sc)
{
    return (sc == 0x3A);
}
// 按下的是否为数字键
static uint IsNumLock(byte sc)
{
    return (sc == 0x45);
}
// 是否按下了暂停键了
static uint PauseHandler(byte sc)
{
    static int cPause = 0;
    uint ret = ( (sc == 0xE1) || cPause );
    
    if( ret )
    {
        static byte cPauseCode[] = {0xE1, 0x1D, 0x45, 0xE1, 0x9D, 0xC5};
        byte* pcc = AddrOff(cPauseCode, cPause);
        
        if( sc == *pcc )
        {
            cPause++; // cPause 是static类型的，下次进来还是使用旧值。
        }
        else
        {
            cPause = 0;
            ret = 0;
        }
        // 确定是按下并且释放了暂停键了。
        if( cPause == Dim(cPauseCode) )
        {
            cPause = 0; 
            //重新映射扫描码，到主键处理，KeyHandler(sc)
            PutScanCode(0x5E);// 按下暂停键的新的编码， 0x1D + 0x45 = 0x5D
            PutScanCode(0xDE);// 释放暂停键的新的编码， 0x5D +  0x80 = 0xDE
        }
    }
    
    return ret;
}
// 是否为数字小键盘
static uint IsNumPadKey(byte sc, int E0)
{
    static const byte cNumScanCode[]            = {0x52, 0x53, 0x4F, 0x50, 0x51, 0x4B, 0x4C, 0x4D, 0x47, 0x48, 0x49, 0x35, 0x37, 0x4A, 0x4E, 0x1C};  
    static const byte cNumE0[Dim(cNumScanCode)] = {0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,    0,    0,    0,    1   };

    uint ret = 0;
    int i = 0;
    
    for(i=0; i<Dim(cNumScanCode); i++)
    {
        byte* pc = AddrOff(cNumScanCode, i);
        byte* pe = AddrOff(cNumE0, i);
        
        if( (sc == *pc) && (E0 == *pe) )
        {
            ret = 1;
            break;
        }
    }
    
    return ret;
}
// 主键盘上的编码方式
static uint MakeNormalCode(KeyCode* pkc, int shift, int caps)
{
    uint ret = 0;
    
    if( !caps )
    {
        if( !shift )
            ret = (pkc->scode << 16) | (pkc->kcode << 8) | pkc->ascii1;
        else
            ret = (pkc->scode << 16) | (pkc->kcode << 8) | pkc->ascii2;
    }
    else
    {
        if( ('a' <= pkc->ascii1) && (pkc->ascii1 <= 'z') )
        {
            if( !shift )
                ret = (pkc->scode << 16) | (pkc->kcode << 8) | pkc->ascii2;
            else
                ret = (pkc->scode << 16) | (pkc->kcode << 8) | pkc->ascii1;
        }
        else
        {
            if( !shift )
                ret = (pkc->scode << 16) | (pkc->kcode << 8) | pkc->ascii1;
            else
                ret = (pkc->scode << 16) | (pkc->kcode << 8) | pkc->ascii2;
        }
    }
    
    return ret;
}

//数字小键盘上的键要进行重新映射，和主键盘上的不一样。
static uint MakeNumCode(KeyCode* pkc, int shift, int num)
{
    static const KeyCode cNumKeyMap[] = 
    {
        { '0',       0,       0x52,     0x2D },
        { '.',       0,       0x53,     0x2E },
        { '1',       0,       0x4F,     0x23 },
        { '2',       0,       0x50,     0x28 },
        { '3',       0,       0x51,     0x22 },
        { '4',       0,       0x4B,     0x25 },
        { '5',       0,       0x4C,     0x0C },
        { '6',       0,       0x4D,     0x27 },
        { '7',       0,       0x47,     0x24 },
        { '8',       0,       0x48,     0x26 },
        { '9',       0,       0x49,     0x21 },
        { '/',      '/',      0x35,     0x6F },
        { '*',      '*',      0x37,     0x6A },
        { '-',      '-',      0x4A,     0x6D },
        { '+',      '+',      0x4E,     0x6B },
        {  0,        0,       0x1C,     0x0D },
        {  0,        0,        0,        0   }
    };
    
    uint ret = 0;
    int i = 0;
    KeyCode* nkc = AddrOff(cNumKeyMap, i);
    
    while( nkc->scode )
    {
        if( nkc->scode == pkc->scode )
        {
            pkc = nkc;
            break;
        }
        
        i++;
        
        nkc = AddrOff(cNumKeyMap, i);
    }
    
    if( IsEqual(pkc, nkc) )
    {
        if( !num )
        {
            ret = (pkc->scode << 16) | (pkc->kcode << 8) | pkc->ascii2;
        }
        else
        {
            if( !shift )
                ret = (pkc->scode << 16) | (pkc->kcode << 8) | pkc->ascii1;
            else 
                ret = (pkc->scode << 16) | (pkc->kcode << 8) | pkc->ascii2;
        }
    }
    
    return ret;
}
// 进行编码的功能
static uint MakeCode(KeyCode* pkc, int shift, int caps, int num, int E0)
{
    uint ret = 0;
    
    if( IsNumPadKey(pkc->scode, E0) )
    {
        ret = MakeNumCode(pkc, shift, num);
    }
    else
    {
        ret = MakeNormalCode(pkc, shift, caps);
    }
    
    return ret;
}

static uint KeyHandler(byte sc)
{
    // 这里用static 类型是为了可以使用上一次的状态
    static int cShift = 0;
    static int cCapsLock = 0;
    static int cNumLock = 0;
    static int E0 = 0;
    
    uint ret = 0;
    
    if( sc == 0xE0 )
    {
        E0 = 1;
        ret = 1;
    }
    else
    {
        uint pressed = KeyType(sc);
        KeyCode* pkc = NULL;
        
        if( !pressed )
        {
            sc = sc - 0x80;
        }
        
        pkc = AddrOff(gKeyMap, sc);
        
        if( ret = !!pkc->scode )
        {
            uint code = 0;
            
            if( IsShift(sc) )
            {
                cShift = pressed;
            }
            else if( IsCapsLock(sc) && pressed )
            {
                cCapsLock = !cCapsLock; // 表示有大小写键按下了。
            }
            else if( IsNumLock(sc) && pressed )
            {
                cNumLock = !cNumLock;
            }
            
            code = pressed | MakeCode(pkc, cShift, cCapsLock, cNumLock, E0);
            
            StoreKeyCode(code);
            
            E0 = 0;
        }
    }
    
    return ret;
}

void PutScanCode(byte sc)
{
    if( PauseHandler(sc) )
    {
        /* Pause Key */
    }
    else if( KeyHandler(sc) )
    {
        /* Normal Key */
    }
    else
    {
        /* Unknown Key */
    }
}

static void NotifyAll(uint kc)
{
    Event evt = {KeyEvent, (uint)&gKeyWait, kc, 0};
    
    EventSchedule(NOTIFY, &evt);
}

void KeyboardModInit()
{
    Queue_Init(&gKeyWait);
    
    gKCBuff.max = 2;
}

void NotifyKeyCode()
{
    uint kc = FetchKeyCode();
    
    if( kc )
    {
        NotifyAll(kc);
    }
}

void KeyCallHandler(uint cmd, uint param1, uint param2)
{
    if( param1 )
    {
        uint kc = FetchKeyCode();
        
        if( kc )
        {
            uint* ret = (uint*)param1;
            
            *ret = kc;
            
            NotifyAll(kc);
        }
        else
        {
            Event* evt = CreateEvent(KeyEvent, (uint)&gKeyWait, param1, 0);
            
            EventSchedule(WAIT, evt);
        }
    }
}
