#include "kernel.h"

GdtInfo gGdtInfo = {0};
IdtInfo gIdtInfo = {0};

// 将base limit 和 attr(属性)，放入到描述符 pDesc。 base : 基址，limit : 极限地址。
int SetDescValue(Descriptor* pDesc, uint base, uint limit, ushort attr)
{
    int ret = 0;
    
    if( ret = (pDesc != NULL) )
    {
        pDesc->limit1        = limit & 0xFFFF;
        pDesc->base1         = base & 0xFFFF;
        pDesc->base2         = (base >> 16) & 0xFF;
        pDesc->attr1         = attr & 0xFF;
        pDesc->attr2_limit2  = ((attr >> 8) & 0xF0) | ((limit >> 16) & 0xF);
        pDesc->base3         = (base >> 24) & 0xFF;
    }
    
    return ret;
}

// 从描述符 pDesc 获得 base ，limit 和 attr(属性)等信息。
int GetDescValue(Descriptor* pDesc, uint* pBase, uint* pLimit, ushort* pAttr)
{
    int ret = 0;
    
    if( ret = (pDesc && pBase && pLimit && pAttr) )
    {
        *pBase  = (pDesc->base3 << 24) | (pDesc->base2 << 16) | pDesc->base1;
        *pLimit = ((pDesc->attr2_limit2 & 0xF) << 16) | pDesc->limit1;
        *pAttr  = ((pDesc->attr2_limit2 & 0xF0) << 8) | pDesc->attr1;
    } 
    
    return ret;
}
// 修改页表属性，让内核态的代码禁止用户态的程序进行修改 
void ConfigPageTable()
{
    uint* TblBase = (void*)PageTblBase;
    // BaseOfApp / 0x1000 - 1; 只修改内核态的页表的属性。
    uint  index = BaseOfApp / 0x1000 - 1;
    uint  i = 0;
    
    for(i=0; i<=index; i++)
    {
        uint* addr = TblBase + i;
        uint  value = *addr;
        
        value = value & 0xFFFFFFFD;
        
        *addr = value;
    }
}

