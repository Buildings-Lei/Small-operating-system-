%include "common.asm"

global _start ;从此处开始执行，也就是_start处
;这里的中断函数声明主要是用来给外面文件来注册到中断描述符表中的。
global TimerHandlerEntry
global KeyboardHandlerEntry
global SysCallHandlerEntry
global PageFaultHandlerEntry
global SegmentFaultHandlerEntry

global ReadPort
global WritePort
global ReadPortW
global WritePortW

extern TimerHandler
extern KeyboardHandler
extern SysCallHandler
extern PageFaultHandler
extern SegmentFaultHandler

extern gMemSize
extern gCTaskAddr
extern gGdtInfo
extern gIdtInfo
extern InitInterrupt
extern EnableTimer
extern SendEOI
extern RunTask
extern LoadTask
extern KMain
extern ClearScreen

%macro BeginFSR 0
    cli 
    
    pushad
    
    push ds
    push es
    push fs
    push gs
    
    mov si, ss
    mov ds, si
    mov es, si
    
    mov esp, BaseOfLoader
%endmacro

%macro BeginISR 0
    cli 

    sub esp, 4
    
    pushad
    
    push ds
    push es
    push fs
    push gs
    
    mov si, ss
    mov ds, si
    mov es, si
    
    mov esp, BaseOfLoader
%endmacro

%macro EndISR 0
    mov esp, [gCTaskAddr]
    
    pop gs
    pop fs
    pop es
    pop ds
    
    popad
    
    add esp, 4
    
    iret
%endmacro

[section .text]
[bits 32]
_start:
    mov ebp, 0
    
    call InitGlobal
    call ClearScreen
    call KMain
    
    jmp $
    
;
;    
InitGlobal:
    push ebp
    mov ebp, esp ; 遵守调用约定，gcc编译的文件和nasm编译的文件要链接调用，就要加上这两句。
    
    mov eax, dword [GdtEntry]
    mov [gGdtInfo], eax
    mov eax, dword [GdtSize]
    mov [gGdtInfo + 4], eax
    
    mov eax, dword [IdtEntry]
    mov [gIdtInfo], eax
    mov eax, dword [IdtSize]
    mov [gIdtInfo + 4], eax
    
    mov eax, dword [RunTaskEntry]
    mov dword [RunTask], eax
    
    mov eax, dword [InitInterruptEntry]
    mov dword [InitInterrupt], eax
    
    mov eax, dword [SendEOIEntry]
    mov dword [SendEOI], eax
    
    mov eax, dword [LoadTaskEntry]
    mov dword [LoadTask], eax
    
    mov eax, dword [MemSize]
    mov dword [gMemSize], eax
    
    leave
    
    ret

;
; byte ReadPort(ushort port)
; 返回值为ax中，端口为传入参数。
ReadPort:
    push ebp
    mov  ebp, esp
    
    xor eax, eax
    
    mov dx, [ebp + 8]
    in  al, dx
    
    nop
    nop
    nop
    
    leave
    
    ret

;
; void WritePort(ushort port, byte value)
;
WritePort:
    push ebp
    mov  ebp, esp
    
    xor eax, eax
    
    mov dx, [ebp + 8]
    mov al, [ebp + 12]
    out dx, al
    
    nop
    nop
    nop
    
    leave
    
    ret

;
; void ReadPortW(ushort port, ushort* buf, uint n)
; 向端口写数据。
ReadPortW:
    push ebp
    mov  ebp, esp
    
    mov edx, [ebp + 8]   ; port
    mov edi, [ebp + 12]  ; buf
    mov ecx, [ebp + 16]  ; n
    
    cld
    rep insw
    
    nop
    nop
    nop
    
    leave
    
    ret

;
; void WritePortW(ushort port, ushort* buf, uint n)
; 用于读取端口数据，用于硬盘读写
WritePortW:
    push ebp
    mov  ebp, esp
    
    mov edx, [ebp + 8]   ; port
    mov esi, [ebp + 12]  ; buf
    mov ecx, [ebp + 16]  ; n
    
    cld
    rep outsw
    
    nop
    nop
    nop
    
    leave
    
    ret

;
;一个中断入口，在注册的时候用
TimerHandlerEntry:
BeginISR 
    call TimerHandler ; 实际的处理函数，在中断文件中实现。
EndISR

;
;
KeyboardHandlerEntry:
BeginISR
    call KeyboardHandler
EndISR

;
;
SysCallHandlerEntry:
BeginISR
; 这四个是传递的参数，调用号，子功能号，参数1，参数2.
    push edx ; 参数2
    push ecx ; 参数1
    push ebx ; 子功能号
    push eax ; 调用号
    call SysCallHandler
    pop eax
    pop ebx
    pop ecx
    pop edx
EndISR

;
;
PageFaultHandlerEntry:
BeginFSR
    call PageFaultHandler
EndISR

;
;
SegmentFaultHandlerEntry:
BeginFSR
    call SegmentFaultHandler
EndISR
