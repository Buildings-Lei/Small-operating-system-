#include "shell.h"
#include "syscall.h"
#include "screen.h"
#include "utility.h"
#include "list.h"
#include "demo1.h"
#include "demo2.h"

#define BUFF_SIZE     64
#define PROMPT        "L.Q.L.OS >> "
#define KEY_ENTER     0x0D
#define KEY_BACKSPACE 0x08


static char gKBuf[BUFF_SIZE] = {0}; // 字符缓冲，存储用户输入的字符，到时候组成命令调用
static int gKIndex = 0;
static List gCmdList = {0};

typedef struct
{
    ListNode header;
    const char* cmd;
    void (*run)();
} CmdRun;

// 第七位是否为1是按下还是释放的分界线。
static uint IsKeyDown(uint kc)
{
    return !!(kc & 0xFF000000);
}
// 获得的重新编码 低八位对应的ascall码，直接强转就可以了
static char GetChar(uint kc)
{
    return (char)kc;
}

static byte GetKeyCode(uint kc)
{
    return (byte)(kc >> 8);
}

static void Mem()
{
    uint ms = GetMemSize() >> 20;
    int w = 0;
    
    SetPrintPos(CMD_START_W, CMD_START_H + 1);
    
    for(w=CMD_START_W; w<SCREEN_WIDTH; w++)
    {
        PrintChar(' ');
    }
    
    SetPrintPos(CMD_START_W, CMD_START_H + 1);
    PrintString("Pysical Memory: ");
    PrintIntDec(ms);
    PrintString(" MB\n");
}

static void Clear()
{
    int h = 0;
    int w = 0;
    
    SetPrintPos(ERR_START_W, ERR_START_H);
    
    for(h=ERR_START_H; h<SCREEN_HEIGHT; h++)
    {
        for(w=ERR_START_W; w<SCREEN_WIDTH; w++)
        {
            PrintChar(' ');
        }
    }
    
    SetPrintPos(CMD_START_W, CMD_START_H);
    PrintString(PROMPT);
}

static void AddCmdEntry(const char* cmd, void(*run)())
{
    CmdRun* cr = (CmdRun*)Malloc(sizeof(CmdRun));
    
    if( cr && cmd && run )
    {
        cr->cmd = cmd;
        cr->run = run;
        
        List_Add(&gCmdList, (ListNode*)cr);
    }
    else
    {
        Free(cr);
    }
}

static int DoCmd(const char* cmd)
{
    int ret = 0;
    ListNode* pos = NULL;
    
    List_ForEach(&gCmdList, pos)
    {
        CmdRun* cr = (CmdRun*)pos;
        
        if( StrCmp(cmd, cr->cmd, -1) )
        {
            cr->run();
            ret = 1;
            break;
        }
    }
    
    return ret;
}

static void Unknown(const char* s)
{
    int w = 0;
    
    SetPrintPos(CMD_START_W, CMD_START_H + 1);
    
    for(w=CMD_START_W; w<SCREEN_WIDTH; w++)
    {
        PrintChar(' ');
    }
    
    SetPrintPos(CMD_START_W, CMD_START_H + 1);
    PrintString("Unknown Command: ");
    PrintString(s);
}

static void ResetCmdLine()
{
    int w = 0;
    
    SetPrintPos(CMD_START_W, CMD_START_H);
    
    for(w=CMD_START_W; w<SCREEN_WIDTH; w++)
    {
        PrintChar(' ');
    }
    
    SetPrintPos(CMD_START_W, CMD_START_H);
    PrintString(PROMPT);
}

static void EnterHandler()
{
    gKBuf[gKIndex++] = 0;
    
    if( (gKIndex > 1) && !DoCmd(gKBuf) )
    {
        Unknown(gKBuf);
    }
    
    gKIndex = 0;
    
    ResetCmdLine();
}

static void BSHandler()
{
    static int cPtLen = 0;
    int w = GetPrintPosW();
    
    if( !cPtLen )
    {
        cPtLen = StrLen(PROMPT);
    }
    
    if( w > cPtLen )
    {
        w--;
        
        SetPrintPos(w, CMD_START_H);
        PrintChar(' ');
        SetPrintPos(w, CMD_START_H);
        
        gKIndex--;
    }
}

static void Handle(char ch, byte vk)
{
    if( ch )
    {
        PrintChar(ch);
        gKBuf[gKIndex++] = ch;
    }
    else
    {
        switch(vk)
        {
            case KEY_ENTER:
                EnterHandler();
                break;
            case KEY_BACKSPACE:
                BSHandler();
                break;
            default:
                break;
        }
    }
}

static void DoWait()
{
    Delay(5);
    Wait("Deinit");
    ResetCmdLine();
}

static void Demo1()
{
    RunDemo1();
    DoWait();
}

static void Demo2()
{
    RunDemo2();
    DoWait();
}

void Shell()
{
    List_Init(&gCmdList);
    
    AddCmdEntry("mem", Mem);
    AddCmdEntry("clear", Clear);
    AddCmdEntry("demo1", Demo1);
    AddCmdEntry("demo2", Demo2);
    
    SetPrintPos(CMD_START_W, CMD_START_H);
    PrintString(PROMPT);
    
    while(1)
    {
        uint key = ReadKey();
        
        if( IsKeyDown(key) )
        {
            char ch = GetChar(key);
            uint vk = GetKeyCode(key);
            
            Handle(ch, vk);
        }
    }
}
