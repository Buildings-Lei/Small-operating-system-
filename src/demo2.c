
#include "demo2.h"
#include "syscall.h"
#include "utility.h"
#include "screen.h"

static const NUM = 50;
static uint g_mutex_write = 0;
static uint g_mutex_read = 0;
static char g_data = 'A';
static int g_count = 0;
static int g_rnext = TASK_START_W;
static int g_rcol = TASK_START_H + 2;

static void Reader()
{
    int run = 1;
    
    while( run )
    {
        EnterCritical(g_mutex_read);
        
        if( g_count == 0 )
            EnterCritical(g_mutex_write); 
        
        g_count++;
        
        ExitCritical(g_mutex_read);
        
        run = !!g_data;
        
        if( run )
        {
            SetPrintPos(g_rnext, g_rcol);
            PrintChar(g_data);
        }
        
        g_rnext++;
        
        if( g_rnext == 50 )
        {
            g_rnext = 0;
            g_rcol++;
        }
        
        EnterCritical(g_mutex_read);
        
        g_count--;
        
        if( g_count == 0 )
            ExitCritical(g_mutex_write);
        
        ExitCritical(g_mutex_read);
        
        Delay(1);
    }
}

static void Writer()
{
    int next = 0;
    
    SetPrintPos(TASK_START_W, TASK_START_H);
    
    PrintString(__FUNCTION__);
    
    while( next < NUM )
    {
        EnterCritical(g_mutex_write);
        
        g_data++;
        
        SetPrintPos(TASK_START_W + 12 + next, TASK_START_H);
        PrintChar(g_data);
        
        ExitCritical(g_mutex_write);
        
        next++;
        
        Delay(1);
    }
    
    EnterCritical(g_mutex_write);
    
    g_data = 0;
    
    ExitCritical(g_mutex_write);
}

static void Initialize()
{
    g_mutex_write = CreateMutex(Normal);
    g_mutex_read = CreateMutex(Strict);
    g_rnext = TASK_START_W;
    g_rcol = TASK_START_H + 2;
    g_data = 'A';
    g_count = 0;
}

static void Deinit()
{
    Wait("Writer");
    Wait("ReaderA");
    Wait("ReaderB");
    Wait("ReaderC");
    
    SetPrintPos(TASK_START_W, TASK_START_H + 8);
    PrintString(__FUNCTION__);
    
    DestroyMutex(g_mutex_write);
    DestroyMutex(g_mutex_read);
}

void RunDemo2()
{
    Initialize();
    
    RegApp("Writer",  Writer, 255);
    RegApp("ReaderA", Reader, 255);
    RegApp("ReaderB", Reader, 255);
    RegApp("ReaderC", Reader, 255);
    
    RegApp("Deinit", Deinit, 255);
}
