; 动态的将软盘初始化成FAT12的格式
jmp short _start
nop

header:
    BS_OEMName     db "B.L.Soft"
    BPB_BytsPerSec dw 512
    BPB_SecPerClus db 1
    BPB_RsvdSecCnt dw 1
    BPB_NumFATs    db 2
    BPB_RootEntCnt dw 224
    BPB_TotSec16   dw 2880
    BPB_Media      db 0xF0
    BPB_FATSz16    dw 9
    BPB_SecPerTrk  dw 18
    BPB_NumHeads   dw 2
    BPB_HiddSec    dd 0
    BPB_TotSec32   dd 0
    BS_DrvNum      db 0
    BS_Reserved1   db 0
    BS_BootSig     db 0x29
    BS_VolID       dd 0
    BS_VolLab      db "B.L.OS-0.01"
    BS_FileSysType db "FAT12   "
    
const:
    RootEntryOffset  equ 19     ;根目录的扇区的偏移位置
    RootEntryLength  equ 14     ;根目录的扇区的长度为14
    SPInitValue      equ BaseOfStack - EntryItemLength
    EntryItem        equ SPInitValue        ;更目录放置的起始偏移地址，这里结尾刚刚好到0x7c00的地址
    EntryItemLength  equ 32     ;一条目录文件的大小为32字节
    FatEntryOffset   equ 1      ;Fat表的偏移地址
    FatEntryLength   equ 9      ;Fat表的长度。
    
_start:
    jmp BLMain
;调用该函数的时候，将参数放入栈中，参数一共五个，每个2字节。
;note: 从软盘中复制目录信息到设置的栈空间中进行比较，这样节省了空间
;buffer 是临时缓冲区。 
; unshort LoadTarget( char*   Target,      notice ==> sizeof(char*) == 2
;                     unshort TarLen,
;                     unshort BaseOfTarget,
;                     unshort BOT_Div_0x10,
;                     char*   Buffer );
; return:
;     dx --> (dx != 0) ? success : failure
LoadTarget:
    mov bp, sp
    
    mov ax, RootEntryOffset
    mov cx, RootEntryLength
    mov bx, [bp + 10] ; mov bx, Buffer，这里要加10，是因为前十个字节为参数。加10是因为栈是从高地址到低地址的。 
    
    call ReadSector     ;先存储在缓冲区
    
    mov si, [bp + 2] ; mov si, Target
    mov cx, [bp + 4] ; mov cx, TarLen
    mov dx, 0
    
    call FindEntry
    
    cmp dx, 0
    jz finish
    
    mov si, bx  ;bx 中存的是找到的文件项的起始地址。
    mov di, EntryItem
    mov cx, EntryItemLength 
    
    call MemCpy ; 将找到的文件项拷贝到设定好的位置，栈顶开始的32字节的位置，还在栈中。
    
    mov bp, sp
    mov ax, FatEntryLength
    mov cx, [BPB_BytsPerSec]
    mul cx
    mov bx, [bp + 6] ; mov bx, BaseOfTarget
    sub bx, ax  ; 将读取的根目录文件项放入 0x9000以下的地址（介于 0x7c00 - 0x9000之间）
    
    mov ax, FatEntryOffset
    mov cx, FatEntryLength
    
    call ReadSector  ; 将读取的内容放入（再读取一次根目录）。
    
    mov dx, [EntryItem + 0x1A] ; 0x1A为文件索引的偏移地址。
    mov es, [bp + 8] ; mov si, BaseOfTarget / 0x10
                     ; mov es, si
    xor si, si 
    
loading:
    mov ax, dx
    add ax, 31
    mov cx, 1
    push dx
    push bx
    mov bx, si
    call ReadSector
    pop bx
    pop cx
    call FatVec
    cmp dx, 0xFF7
    jnb finish
    add si, 512
    cmp si, 0       ;这里一部分代码就是扩大地址访问空间的作用，
    jnz continue    ;原本是es：si，si最大为FFFF访问空间有限，所以对es动手脚。
    mov si, es      ;
    add si, 0x1000
    mov es, si
    mov si, 0
continue:
    jmp loading
 
finish:   
    ret

; 根据FAT12软盘的文件组织形式，存在里面的大文件是以一个扇区一个扇区分散式的存储的，之间的链接是以
; 链表的形式，进行连接的。这个函数的功能主要是找到文件下一个存储的扇区的扇区号。
; cx --> index
; bx --> fat table address
;
; return:
;     dx --> fat[index]
FatVec:
    push cx
    
    mov ax, cx
    shr ax, 1
    
    mov cx, 3
    mul cx
    mov cx, ax
    
    pop ax
    
    and ax, 1
    jz even
    jmp odd

even:    ; FatVec[j] = ( (Fat[i+1] & 0x0F) << 8 ) | Fat[i];
    mov dx, cx
    add dx, 1
    add dx, bx
    mov bp, dx
    mov dl, byte [bp]
    and dl, 0x0F
    shl dx, 8
    add cx, bx
    mov bp, cx
    or  dl, byte [bp]
    jmp return
    
odd:     ; FatVec[j+1] = (Fat[i+2] << 4) | ( (Fat[i+1] >> 4) & 0x0F );
    mov dx, cx
    add dx, 2
    add dx, bx
    mov bp, dx
    mov dl, byte [bp]
    mov dh, 0
    shl dx, 4
    add cx, 1
    add cx, bx
    mov bp, cx
    mov cl, byte [bp]
    shr cl, 4
    and cl, 0x0F
    mov ch, 0
    or  dx, cx

return: 
    ret
; 进行复制拷贝，从源地址处的内容拷贝到目标地址处的内容。这里已经考虑了si 和di之间的大小问题。
; ds:si --> source
; es:di --> destination
; cx    --> length
MemCpy:
    
    cmp si, di
    
    ja btoe
    
    add si, cx
    add di, cx
    dec si
    dec di
    
    jmp etob
    
btoe:
    cmp cx, 0
    jz done
    mov al, [si]
    mov byte [di], al
    inc si
    inc di
    dec cx
    jmp btoe
    
etob: 
    cmp cx, 0
    jz done
    mov al, [si]
    mov byte [di], al
    dec si
    dec di
    dec cx
    jmp etob

done:   
    ret

; 在软盘中找到目录，对于FAT12 文件系统来说，根目录文件项（里面包含了所有的文件名等信息）在19扇区号中，长度为14字节，都是固定的。
; 这个函数的功能是为了后面加载loder文件和内核文件中使用的。如果找到了bx中最后返回时存的是找到的文件项的起始地址。
; es:bx --> root entry offset address
; ds:si --> target string
; cx    --> target length
;
; return:
;     (dx !=0 ) ? exist : noexist
;        exist --> bx is the target entry
FindEntry:
    push cx
    
    mov dx, [BPB_RootEntCnt]
    mov bp, sp
    
find:
    cmp dx, 0
    jz noexist
    mov di, bx
    mov cx, [bp]
    push si
    call MemCmp
    pop si
    cmp cx, 0
    jz exist
    add bx, 32 ;一项文件目录项的固定大小为32字节。
    dec dx
    jmp find

exist:
noexist: 
    pop cx
       
    ret

; 内存比较，比较两个字符串是否相等。用于找到load文件和内核文件。
; ds:si --> source
; es:di --> destination
; cx    --> length
;
; return:
;        (cx == 0) ? equal : noequal
MemCmp:

compare:
    cmp cx, 0
    jz equal
    mov al, [si]
    cmp al, byte [di]
    jz goon
    jmp noequal
goon:
    inc si
    inc di
    dec cx
    jmp compare
    
equal: 
noequal:   

    ret

; 读取 软盘中的扇区的信息，输入扇区号，数量，拷贝软盘内容到 es:bx的地址处。
; ax    --> logic sector number
; cx    --> number of sector
; es:bx --> target address
ReadSector:
    
    mov ah, 0x00
    mov dl, [BS_DrvNum]
    int 0x13
    
    push bx
    push cx
    
    mov bl, [BPB_SecPerTrk]
    div bl
    mov cl, ah
    add cl, 1
    mov ch, al
    shr ch, 1
    mov dh, al
    and dh, 1
    mov dl, [BS_DrvNum]
    
    pop ax
    pop bx
    
    mov ah, 0x02

read:    
    int 0x13
    jc read
    
    ret
