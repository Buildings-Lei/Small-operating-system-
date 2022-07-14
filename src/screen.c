
#include "const.h"
#include "screen.h"

static byte gPosW = 0;
static byte gPosH = 0;
static byte gColor = SCREEN_WHITE;

byte GetPrintPosH()
{
    return gPosH;
}

byte GetPrintPosW()
{
    return gPosW;
}

void ClearScreen()
{
    int h = 0;
    int w = 0;
    
    SetPrintPos(0, 0);
    
    for(h=0; h<SCREEN_HEIGHT; h++)
    {
        for(w=0; w<SCREEN_WIDTH; w++)
        {
            PrintChar(' ');
        }
    }
    
    SetPrintPos(0, 0);
}

int SetPrintPos(byte w, byte h)
{
    int ret = 0;
    
    if( ret = ((w < SCREEN_WIDTH) && (h < SCREEN_HEIGHT)) )
    {
        ushort bx = SCREEN_WIDTH * h + w;
        
        gPosW = w;
        gPosH = h;
        
        asm volatile(
            "movw %0,      %%bx\n"
            "movw $0x03D4, %%dx\n"
            "movb $0x0E,   %%al\n"
            "outb %%al,    %%dx\n"
            "movw $0x03D5, %%dx\n"
            "movb %%bh,    %%al\n"
            "outb %%al,    %%dx\n"
            "movw $0x03D4, %%dx\n"
            "movb $0x0F,   %%al\n"
            "outb %%al,    %%dx\n"
            "movw $0x03D5, %%dx\n"
            "movb %%bl,    %%al\n"
            "outb %%al,    %%dx\n"
            :
            : "r"(bx)
            : "ax", "bx", "dx"
        );
    }
    
    return ret;
}

void SetPrintColor(PrintColor c)
{
    gColor = c;
}

int PrintChar(char c)
{
    int ret = 0;
    
    if( (c == '\n') || (c == '\r') )
    {
        ret = SetPrintPos(0, gPosH + 1);
    } 
    else
    {
        byte pw = gPosW;
        byte ph = gPosH;
        
        if( (pw < SCREEN_WIDTH) && (ph < SCREEN_HEIGHT) )
        {
            uint edi = (SCREEN_WIDTH * ph + pw) * 2;
            byte ah = gColor;
            char al = c;
            
            asm volatile(
               "movl %0,   %%edi\n"
               "movb %1,   %%ah\n"
               "movb %2,   %%al\n"
               "movw %%ax, %%gs:(%%edi)"
               "\n"
               :
               : "r"(edi), "r"(ah), "r"(al)
               : "ax", "edi"
            );
            
            pw++;
            
            if( pw == SCREEN_WIDTH )
            {
                pw = 0;
                ph = ph + 1;
            }
            
            ret = 1;
        }
        
        SetPrintPos(pw, ph);
    }
    
    return ret;
}

int PrintString(const char* s)
{
    int ret = 0;
    
    if( s != NULL )
    {
        while( *s )
        {
            ret += PrintChar(*s++);
        }
    }
    else
    {
        ret = -1;
    }
    
    return ret;
}

int PrintIntHex(uint n)
{
    int i = 0;
    int ret = 0;
    
    ret += PrintChar('0');
    ret += PrintChar('x');
    //32位的整形值，FFFFFFFF既可以表示下来了。
    for(i=28; i>=0; i-=4)
    {
        int p = (n >> i) & 0xF;
        
        if( p < 10 )
        {
            ret += PrintChar('0' + p);
        }
        else
        {
            ret += PrintChar('A' + p - 10);
        }
    }
    
    return ret;
}

int PrintIntDec(int n)
{
    int ret = 0;
    
    if( n < 0 )
    {
        ret += PrintChar('-');
        
        n = -n;
        
        ret += PrintIntDec(n);
    }
    else
    {
        if( n < 10 )
        {
            ret += PrintChar('0' + n);
        }
        else
        {
            ret += PrintIntDec(n/10);
            ret += PrintIntDec(n%10);
        }
    } 
    
    return ret;
}




