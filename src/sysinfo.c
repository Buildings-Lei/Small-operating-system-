
#include "sysinfo.h"

uint gMemSize = 0;

void SysInfoCallHandler(uint cmd, uint param1, uint param2)
{
    if( cmd == 0 )
    {
        uint* pRet = (uint*)param1;
        
        *pRet = gMemSize;
    }
}
