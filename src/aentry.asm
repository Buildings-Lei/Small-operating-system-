%include "common.asm"

global _start  ;从此处开始执行，也就是_start处
global AppModInit

extern AppMain
extern MemModInit

[section .text]
[bits 32]
_start:
; 被 KMain 调用的函数
AppModInit: ; 0xF000
    push ebp
    mov ebp, esp
    
    mov dword [AppMainEntry], AppMain
    
    push HeapSize
    push AppHeapBase
    
    call MemModInit
    
    add esp, 8
    
    leave
    
    ret
    
