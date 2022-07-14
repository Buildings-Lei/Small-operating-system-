; 主要完成的功能：加载loader文件，放在软盘，硬盘等媒介中的第一个扇区，起到一个操作系统和桥梁的作用
%include "blfunc.asm"
%include "common.asm"

org BaseOfBoot


BaseOfStack    equ    BaseOfBoot

Loader db  "LOADER     "
LdLen  equ ($-Loader)

BLMain:
    mov ax, cs
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov sp, SPInitValue

    ;加载loader文件，并放到BaseOfLoader的位置处
    push word Buffer
    push word BaseOfLoader / 0x10 ; 左移四位，是为了扩大内存的访问空间。
    push word BaseOfLoader
    push word LdLen
    push word Loader
    
    call LoadTarget
    
    cmp dx, 0
    jz output
    
    jmp BaseOfLoader
    
output:    
    mov ax, cs
    mov es, ax
    mov bp, ErrStr
    mov cx, ErrLen
    xor dx, dx
    mov ax, 0x1301
    mov bx, 0x0007
    int 0x10
    
    jmp $    

ErrStr db  "NOLD"    
ErrLen equ ($-ErrStr)

Buffer:
    times 510-($-$$) db 0x00
    db 0x55, 0xaa
