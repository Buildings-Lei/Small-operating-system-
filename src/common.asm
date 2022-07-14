; Base Address
HeapBase       equ    0x70000
HeapSize       equ    0x20000
KernelHeapBase equ    HeapBase
AppHeapBase    equ    HeapBase - HeapSize
PageDirBase    equ    HeapBase + HeapSize
PageTblBase    equ    PageDirBase + 0x1000

; Base Definition
BaseOfBoot    equ    0x7C00
BaseOfLoader  equ    0x9000
BaseOfKernel  equ    0xB000
BaseOfApp     equ    0x12000

BaseOfSharedMemory   equ    0xA000


; Shared Value Address
GdtEntry             equ       BaseOfSharedMemory + 0   ;全局描述符表入口地址
GdtSize              equ       BaseOfSharedMemory + 4   ;全局描述符表的大小
IdtEntry             equ       BaseOfSharedMemory + 8   ;局部描述符表的入口地址
IdtSize              equ       BaseOfSharedMemory + 12  ;局部描述符表的大小
RunTaskEntry         equ       BaseOfSharedMemory + 16  ;运行任务的函数的入口地址
InitInterruptEntry   equ       BaseOfSharedMemory + 20  ;初始化中断函数的入口地址
MemSize              equ       BaseOfSharedMemory + 24  ;获得物理内存大小的入口地址
SendEOIEntry         equ       BaseOfSharedMemory + 28  ;在外部中断使用时，发送EOI信息的函数入口地址
LoadTaskEntry        equ       BaseOfSharedMemory + 32  ;加载任务的入口地址
AppMainEntry         equ       BaseOfSharedMemory + 36  ;app 的main函数的入口地址

; PIC-8259A Ports 8259A的端口号
MASTER_ICW1_PORT                        equ     0x20
MASTER_ICW2_PORT                        equ     0x21
MASTER_ICW3_PORT                        equ     0x21
MASTER_ICW4_PORT                        equ     0x21
MASTER_OCW1_PORT                        equ     0x21
MASTER_OCW2_PORT                        equ     0x20
MASTER_OCW3_PORT                        equ     0x20

SLAVE_ICW1_PORT                         equ     0xA0
SLAVE_ICW2_PORT                         equ     0xA1
SLAVE_ICW3_PORT                         equ     0xA1
SLAVE_ICW4_PORT                         equ     0xA1
SLAVE_OCW1_PORT                         equ     0xA1
SLAVE_OCW2_PORT                         equ     0xA0
SLAVE_OCW3_PORT                         equ     0xA0

MASTER_EOI_PORT                         equ     0x20
MASTER_IMR_PORT                         equ     0x21
MASTER_IRR_PORT                         equ     0x20
MASTER_ISR_PORT                         equ     0x20

SLAVE_EOI_PORT                          equ     0xA0
SLAVE_IMR_PORT                          equ     0xA1
SLAVE_IRR_PORT                          equ     0xA0
SLAVE_ISR_PORT                          equ     0xA0

; Segment Attribute 保护模式中的对段进行设置的时候，需要设置的段的属性
DA_32       equ    0x4000
DA_LIMIT_4K    EQU       0x8000
DA_DR       equ    0x90
DA_DRW      equ    0x92
DA_DRWA     equ    0x93
DA_C        equ    0x98
DA_CR       equ    0x9A
DA_CCO      equ    0x9C
DA_CCOR     equ    0x9E

; Segment Privilege 段的优先级的设置
DA_DPL0        equ      0x00    ; DPL = 0
DA_DPL1        equ      0x20    ; DPL = 1
DA_DPL2        equ      0x40    ; DPL = 2
DA_DPL3        equ      0x60    ; DPL = 3

; Special Attribute 
DA_LDT       equ    0x82
DA_TaskGate  equ    0x85    ; 任务门类型值
DA_386TSS    equ    0x89    ; 可用 386 任务状态段类型值
DA_386CGate  equ    0x8C    ; 386 调用门类型值
DA_386IGate  equ    0x8E    ; 386 中断门类型值
DA_386TGate  equ    0x8F    ; 386 陷阱门类型值

; Selector Attribute 选择子的属性设置
SA_RPL0    equ    0
SA_RPL1    equ    1
SA_RPL2    equ    2
SA_RPL3    equ    3

SA_TIG    equ    0
SA_TIL    equ    4

PG_P    equ    1    ; 页存在属性位
PG_RWR  equ    0    ; R/W 属性位值, 读/执行
PG_RWW  equ    2    ; R/W 属性位值, 读/写/执行
PG_USS  equ    0    ; U/S 属性位值, 系统级
PG_USU  equ    4    ; U/S 属性位值, 用户级

; 描述符
; usage: Descriptor Base, Limit, Attr
;        Base:  dd
;        Limit: dd (low 20 bits available)
;        Attr:  dw (lower 4 bits of higher byte are always 0)
%macro Descriptor 3                              ; 段基址， 段界限， 段属性
    dw    %2 & 0xFFFF                         ; 段界限1
    dw    %1 & 0xFFFF                         ; 段基址1
    db    (%1 >> 16) & 0xFF                   ; 段基址2
    dw    ((%2 >> 8) & 0xF00) | (%3 & 0xF0FF) ; 属性1 + 段界限2 + 属性2
    db    (%1 >> 24) & 0xFF                   ; 段基址3
%endmacro                                     ; 共 8 字节

; 门
; usage: Gate Selector, Offset, DCount, Attr
;        Selector:  dw
;        Offset:    dd
;        DCount:    db
;        Attr:      db
%macro Gate 4
    dw    (%2 & 0xFFFF)                      ; 偏移地址1
    dw    %1                                 ; 选择子
    dw    (%3 & 0x1F) | ((%4 << 8) & 0xFF00) ; 属性
    dw    ((%2 >> 16) & 0xFFFF)              ; 偏移地址2
%endmacro 

