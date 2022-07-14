
#ifndef CONST_H
#define CONST_H

#define NULL  ((void*)0)

#define HeapBase       0x70000
#define HeapSize       0x20000
#define KernelHeapBase HeapBase
#define AppHeapBase    (HeapBase - HeapSize)
#define PageDirBase    (HeapBase + HeapSize)
#define PageTblBase    (PageDirBase + 0x1000)

#define AppStackSize    512

#define BaseOfKernel    0xB000
#define BaseOfApp       0x12000

#define BaseOfSharedMemory 0xA000
#define AppMainEntry       (BaseOfSharedMemory + 36)

#define    DA_DPL0            0x00
#define    DA_DPL1            0x20
#define    DA_DPL2            0x40
#define    DA_DPL3            0x60

#define    SA_RPL_MASK    0xFFFC

#define    SA_RPL0        0
#define    SA_RPL1        1
#define    SA_RPL2        2
#define    SA_RPL3        3

#define    SA_TI_MASK    0xFFFB

#define    SA_TIG        0
#define    SA_TIL        4


#define    DA_32            0x4000
#define    DA_LIMIT_4K        0x8000

#define    DA_DR            0x90
#define    DA_DRW            0x92
#define    DA_DRWA            0x93
#define    DA_C            0x98
#define    DA_CR            0x9A
#define    DA_CCO            0x9C
#define    DA_CCOR            0x9E

#define    DA_LDT            0x82
#define    DA_TaskGate        0x85
#define    DA_386TSS        0x89
#define    DA_386CGate        0x8C
#define    DA_386IGate        0x8E
#define    DA_386TGate        0x8F

#define    GDT_DUMMY_INDEX         0    
#define    GDT_CODE32_INDEX        1
#define    GDT_VIDEO_INDEX         2
#define    GDT_CODE32_FLAT_INDEX   3
#define    GDT_DATA32_FLAT_INDEX   4
#define    GDT_TASK_LDT_INDEX      5
#define    GDT_TASK_TSS_INDEX      6

#define    GDT_DUMMY_SELECTOR         ((GDT_DUMMY_INDEX << 3) + SA_TIG + SA_RPL0)
#define    GDT_CODE32_SELECTOR        ((GDT_CODE32_INDEX << 3) + SA_TIG + SA_RPL0)    
#define    GDT_VIDEO_SELECTOR         ((GDT_VIDEO_INDEX << 3) + SA_TIG + SA_RPL0)    
#define    GDT_CODE32_FLAT_SELECTOR   ((GDT_CODE32_FLAT_INDEX << 3) + SA_TIG + SA_RPL0)
#define    GDT_DATA32_FLAT_SELECTOR   ((GDT_DATA32_FLAT_INDEX << 3) + SA_TIG + SA_RPL0)
#define    GDT_TASK_LDT_SELECTOR      ((GDT_TASK_LDT_INDEX << 3) + SA_TIG + SA_RPL0)
#define    GDT_TASK_TSS_SELECTOR      ((GDT_TASK_TSS_INDEX << 3) + SA_TIG + SA_RPL0)        

#define    LDT_VIDEO_INDEX         0
#define    LDT_CODE32_INDEX        1    
#define    LDT_DATA32_INDEX        2

#define    LDT_VIDEO_SELECTOR     ((LDT_VIDEO_INDEX << 3) + SA_TIL + SA_RPL3)    
#define    LDT_CODE32_SELECTOR    ((LDT_CODE32_INDEX << 3) + SA_TIL + SA_RPL3)    
#define    LDT_DATA32_SELECTOR    ((LDT_DATA32_INDEX << 3) + SA_TIL + SA_RPL3)    

#define    MASTER_EOI_PORT     0x20
#define    SLAVE_EOI_PORT      0xA0

#endif
