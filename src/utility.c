
#include "utility.h"

void Delay(int n)
{
    while( n > 0 )
    {
        int i = 0;
        int j = 0;
        
        for(i=0; i<1000; i++)
        {
            for(j=0; j<1000; j++)
            {
                asm volatile ("nop\n");
            }
        }
        
        n--;
    }
}

byte* MemCpy(byte* dst, const byte* src, uint n)
{
    byte* ret = dst;
    uint dAddr = (uint)dst;
    uint sAddr = (uint)src;
    int i = 0;

    if( dAddr < sAddr )
    {
        for(i=0; i<n; i++)
        {
            dst[i] = src[i];
        }
    }

    if( dAddr > sAddr )
    {
        for(i=n-1; i>=0; i--)
        {
            dst[i] = src[i];
        }
    }

    return ret;
}

byte* MemSet(byte* dst, uint n, byte val)
{
    byte* ret = dst;
    int i = 0;

    while( i < n )
    {
        dst[i++] = val;
    }

    return ret;
}

char* StrCpy(char* dst, const char* src, uint n)
{
    char* ret = dst;
    uint dAddr = (uint)dst;
    uint sAddr = (uint)src;
    int i = 0;

    if( dAddr < sAddr )
    {
        for(i=0; src[i] && (i<n); i++)
        {
            dst[i] = src[i];
        }

        dst[i] = 0;
    }

    if( dAddr > sAddr )
    {
        uint len = StrLen(src);

        len = (len < n) ? len : n;

        dst[len] = 0;

        for(i=len-1; i>=0; i--)
        {
            dst[i] = src[i];
        }
    }
    
    return ret;
}

int StrLen(const char* s)
{
    int ret = 0;
    
    while( s && s[ret] )
    {
        ret++;
    }
    
    return ret;
}

int StrCmp(const char* left, const char* right, uint n)
{
    int ret = 1;
    
    if( !IsEqual(left, right) )
    {
        int lLen = StrLen(left);
        int rLen = StrLen(right);
        int m = Min(lLen, rLen);
        int i = 0;
        
        n = Min(m, n);
        ret = IsEqual(lLen, rLen);
        
        for(i=0; (i<n) && ret; i++)
        {
            ret = IsEqual(left[i], right[i]);
            
            if( !ret )
            {
                break;
            }
        }
    }
    
    return ret;
}



