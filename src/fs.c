#include "hdraw.h"
#include "fs.h"
#include "utility.h"
#include "list.h"

#ifdef DTFSER
#include <malloc.h>
#define Malloc malloc
#define Free free
#else
#include "memory.h"
#endif

#define FS_MAGIC       "BuildingsFS-v1.0"
#define ROOT_MAGIC     "ROOT"
#define HEADER_SCT_IDX 0
#define ROOT_SCT_IDX   1
#define FIXED_SCT_SIZE 2
#define SCT_END_FLAG   ((uint)-1) // 标记为非法扇区
#define FE_BYTES       sizeof(FileEntry)
#define FD_BYTES       sizeof(FileDesc)
#define FE_ITEM_CNT    (SECT_SIZE / FE_BYTES) 
#define MAP_ITEM_CNT   (SECT_SIZE / sizeof(uint)) // 一个扇区要包括多少管理单元

typedef struct
{
    byte forJmp[4];     // 预留给jump指令使用的。
    char magic[32];     // 名字
    uint sctNum;        // 硬盘还有多少扇区可以使用
    uint mapSize;       // 扇区分配表的大小
    uint freeNum;       // 空闲空间的大小
    uint freeBegin;     // 空闲空间的起始位置，绝对地址。
} FSHeader;             // 存储在0号扇区，记录文件系统概要信息

typedef struct
{
    char magic[32];     // 存储根目录的名字
    uint sctBegin;      // 记录root 这个文件从哪开始的
    uint sctNum;        // 记录root 有多少个扇区 
    uint lastBytes;     // 根目录这个文件,有可能最后一个扇区没有用完，所以这个记录用了多少
} FSRoot;               // 存储1号扇区，记录根目录相关信息。

typedef struct
{
    char name[32];      // 文件名
    uint sctBegin;      // 文件起始扇区
    uint sctNum;        // 文件扇区的数量
    uint lastBytes;     // 文件最后一个扇区的所占有的空间
    uint type;          // 文件的类型是文件还是目录
    uint inSctIdx;      // 用来定位文件描述结构FileEntry在根目录链表中的扇区号
    uint inSctOff;      // 根目录链表中的扇区号中的偏移地址和上面一起定位FileEntry
    uint reserved[2];   // 预留
} FileEntry; // 在磁盘操作时，对文件进行描述         

typedef struct
{
    ListNode head; 
    FileEntry fe;
    uint objIdx;             // 指向文件链表中缓冲区所对应的位置，链表中序号。
    uint offset;             // 指向缓冲区的可写入的位置。
    uint changed;
    byte cache[SECT_SIZE];   // 文件数据缓冲区，减少读取的次数
} FileDesc; // 当打开文件时，用户使用层面对文件进行描述---文件描述符

typedef struct
{   
    uint* pSct;              // 记录扇区分配表中的地址
    uint sctIdx;             // 记录目标扇区中的偏移量
    uint sctOff;             // 记录扇区分配表中的偏移量
    uint idxOff;             // 记录对应扇区中的偏移量
} MapPos; // 记录分配扇区和分配单元之间的关系

static List gFDList = {0};

/**********************************
Function: FSModInit
Description: 文件系统的初始化
Calls: HDRawModInit List_Init
Input: NULL
Output: void
Note:
**********************************/
void FSModInit()
{
    // 硬件初始化
    HDRawModInit();

    List_Init(&gFDList);
}

/**********************************
Function: ReadSector
Description: 读取一个扇区的内容。
Calls: HDRawRead
Input: 所要读取的磁盘的逻辑号
Output: 读取的内容的地址（函数内申请的空间大小为512字节一扇区）
Note:
**********************************/
static void* ReadSector(uint si)
{
    void* ret = NULL;

    if( si != SCT_END_FLAG )
    {
        ret = Malloc(SECT_SIZE);

        if( !(ret && HDRawRead(si, (byte*)ret)) )
        {
            Free(ret);
            ret = NULL;
        }
    }

    return ret;
}

/**********************************
Function: FindInMap
Description: 将逻辑扇区号封装成MapPos结构体信息，方便后面进行空间申请和释放
Calls: ReadSector,Free
Input: 逻辑扇区号
Output: MapPos结构体
Note: 
**********************************/
static MapPos FindInMap(uint si)
{
    MapPos ret = {0};
    FSHeader* header = (si != SCT_END_FLAG) ? ReadSector(HEADER_SCT_IDX) : NULL;

    if( header )
    {
        uint offset = si - header->mapSize - FIXED_SCT_SIZE;
        uint sctOff = offset / MAP_ITEM_CNT;
        uint idxOff = offset % MAP_ITEM_CNT;
        uint* ps = ReadSector(sctOff + FIXED_SCT_SIZE);

        if( ps )
        {
            ret.pSct = ps;
            ret.sctIdx = si;
            ret.sctOff = sctOff;
            ret.idxOff = idxOff;
        }
    }

    Free(header);

    return ret;
}

/**********************************
Function: AllocSector
Description: 获取一个空闲的扇区，大小为512字节
Calls: ReadSector FindInMap HDRawWrite
Input: void
Output: 空闲空间扇区的逻辑号
Note:
**********************************/
static uint AllocSector()
{
    uint ret = SCT_END_FLAG;
    FSHeader* header = ReadSector(HEADER_SCT_IDX);// 0扇区只存了 header。

    if( header && (header->freeBegin != SCT_END_FLAG) )
    {
        MapPos mp = FindInMap(header->freeBegin);

        if( mp.pSct )
        {
            uint* pInt = AddrOff(mp.pSct, mp.idxOff);
            uint next = *pInt;
            uint flag = 1;

            ret = header->freeBegin;

            header->freeBegin = next + FIXED_SCT_SIZE + header->mapSize;
            header->freeNum--;

            *pInt = SCT_END_FLAG;
            // 磁盘的信息改变了，header 也就改变了，对应的扇区分配表也变了，所以也要重写上去。
            flag = flag && HDRawWrite(HEADER_SCT_IDX, (byte*)header);
            flag = flag && HDRawWrite(mp.sctOff + FIXED_SCT_SIZE, (byte*)mp.pSct);

            if( !flag )
            {
                ret = SCT_END_FLAG;
            }
        }

        Free(mp.pSct);
    }

    Free(header);

    return ret;
}

/**********************************
Function: FreeSector
Description: 释放一个扇区的空间
Calls: ReadSector FindInMap HDRawWrite
Input: 磁盘的逻辑扇区号
Output: 成功返回1；
Note:
**********************************/
static uint FreeSector(uint si)
{
    FSHeader* header = (si != SCT_END_FLAG) ? ReadSector(HEADER_SCT_IDX) : NULL;
    uint ret = 0;

    if( header )
    {
        MapPos mp = FindInMap(si);

        if( mp.pSct )
        {
            uint* pInt = AddrOff(mp.pSct, mp.idxOff);
            // 将扇区插入到空闲扇区链表的头部（相对地址，在扇区分配表之后开始算的）
            *pInt = header->freeBegin - FIXED_SCT_SIZE - header->mapSize;

            header->freeBegin = si;
            header->freeNum++;

            ret = HDRawWrite(HEADER_SCT_IDX, (byte*)header) &&
                  HDRawWrite(mp.sctOff + FIXED_SCT_SIZE, (byte*)mp.pSct);
        }

        Free(mp.pSct);
    }

    Free(header);

    return ret;
}
 
/**********************************
Function: NextSector
Description: 获得下一个扇区号（链式的管理的形式，每个单元扇区表中存储的是下一个扇区号）
Calls: ReadSector FindInMap
Input: 当前逻辑扇区号
Output: 下一个逻辑扇区号
Note:
**********************************/
static uint NextSector(uint si)
{
    FSHeader* header = (si != SCT_END_FLAG) ? ReadSector(HEADER_SCT_IDX) : NULL;
    uint ret = SCT_END_FLAG;

    if( header )
    {
        MapPos mp = FindInMap(si);

        if( mp.pSct )
        {
            uint* pInt = AddrOff(mp.pSct, mp.idxOff);

            if( *pInt != SCT_END_FLAG )
            {
                ret = *pInt + header->mapSize + FIXED_SCT_SIZE;
            }
        }

        Free(mp.pSct);
    }

    Free(header);

    return ret;
}
 
/**********************************
Function: FindLast
Description: 找到链表中的最后一个扇区号
Calls: NextSector
Input: 起始逻辑扇区号
Output: 链表最后一个逻辑扇区号
Note: 文件的保存形式是以链表的形式保存的。
**********************************/
static uint FindLast(uint sctBegin)
{
    uint ret = SCT_END_FLAG;
    uint next = sctBegin;

    while( next != SCT_END_FLAG )
    {
        ret = next;
        next = NextSector(next);
    }

    return ret;
}

/**********************************
Function: FindPrev
Description: 获得输入的逻辑号的上一个扇区逻辑号（链式的管理的形式，每个单元扇区表中存储的是下一个扇区号）
Calls:  FindInMap
Input:  文件起始的扇区逻辑号, 当前逻辑扇区号
Output: si的上一个扇区逻辑号
Note: 采用遍历方式，是考虑到查找这个并不频繁，不在结构体中加上指向上一个逻辑号的变量。
**********************************/
static uint FindPrev(uint sctBegin, uint si)
{
    uint ret = SCT_END_FLAG;
    uint next = sctBegin;

    while( (next != SCT_END_FLAG) && (next != si) )
    {
        ret = next;
        next = NextSector(next);
    }

    if( next == SCT_END_FLAG )
    {
        ret = SCT_END_FLAG;
    }

    return ret;
}

/**********************************
Function: FindIndex
Description: 查找链表当中的第idx扇区。
Calls: NextSector
Input: 链表的起始扇区号, 所要查找的第idx个扇区
Output: 如果找到了则返回第idx个扇区的逻辑号，反之返回错误状态
Note: idx 从 0 开始算，idx  = 0时，返回第一个扇区。
**********************************/
static uint FindIndex(uint sctBegin, uint idx)
{
    uint ret = sctBegin;
    uint i = 0;

    while( (i < idx) && (ret != SCT_END_FLAG) )
    {
        ret = NextSector(ret);

        i++;
    }

    return ret;
}

/**********************************
Function: MarkSector
Description: 标记目标扇区不可用
Calls: FindInMap HDRawWrite
Input: 目标扇区逻辑号
Output: 成功则为 1 
Note:
**********************************/
static uint MarkSector(uint si)
{
    uint ret = (si == SCT_END_FLAG) ? 1 : 0;
    MapPos mp = FindInMap(si);

    if( mp.pSct )
    {
        uint *pInt = AddrOff(mp.pSct, mp.idxOff);

        *pInt = SCT_END_FLAG;

        ret = HDRawWrite(mp.sctOff + FIXED_SCT_SIZE, (byte*)mp.pSct);
    }

    Free(mp.pSct);

    return ret;
}

/**********************************
Function: AddToLast
Description: 将扇区添加到链表的末尾
Calls: FindLast FindInMap HDRawWrite
Input: sctBegin : 链表的头 , si : 需要添加的扇区
Output: void 
Note: 当 si == sctBegin 时也不影响啊。
**********************************/
static void AddToLast(uint sctBegin, uint si)
{
    uint last = FindLast(sctBegin);

    if( last != SCT_END_FLAG )
    {
        MapPos lmp = FindInMap(last);
        MapPos smp = FindInMap(si);

        if( lmp.pSct && smp.pSct )
        {
            if( lmp.sctOff == smp.sctOff )
            {
                uint* pInt = AddrOff(lmp.pSct, lmp.idxOff);

                *pInt = lmp.sctOff * MAP_ITEM_CNT + smp.idxOff;

                pInt = AddrOff(lmp.pSct, smp.idxOff);

                *pInt = SCT_END_FLAG;

                HDRawWrite(lmp.sctOff + FIXED_SCT_SIZE, (byte*)lmp.pSct);
            }
            else
            {
                uint* pInt = AddrOff(lmp.pSct, lmp.idxOff);

                *pInt = smp.sctOff * MAP_ITEM_CNT + smp.idxOff;

                pInt = AddrOff(smp.pSct, smp.idxOff);

                *pInt = SCT_END_FLAG;

                HDRawWrite(lmp.sctOff + FIXED_SCT_SIZE, (byte*)lmp.pSct);
                HDRawWrite(smp.sctOff + FIXED_SCT_SIZE, (byte*)smp.pSct);
            }
        }

        Free(lmp.pSct);
        Free(smp.pSct);
    }
}

/**********************************
Function: CheckStorage
Description: 检查链表（代表一个文件）的空间是否够用
Calls: AddToLast
Input: FSRoot* fe: 链表的头节点（保存所有的文件的信息 FileEntry或者是FSRoot）
Output: 成功则为 1。
Note: FileEntry 代表一个文件。是链式结构的。（根目录也是一个文件）
**********************************/
static uint CheckStorage(FSRoot* fe)
{
    uint ret = 0;
    // 一个扇区是字节，是FSRoot和FileEntry的大小的整数倍，不会出现有最后一个空间不够的情况
    // lastBytes = 512 时说明这个扇区的空间已经用完了
    if( fe->lastBytes == SECT_SIZE )
    {
        uint si = AllocSector();

        if( si != SCT_END_FLAG )
        {
            if( fe->sctBegin == SCT_END_FLAG )
            {
                fe->sctBegin = si;
            }
            else
            {
                AddToLast(fe->sctBegin, si);
            }

            fe->sctNum++;
            fe->lastBytes = 0;

            ret = 1;
        }
    }

    return ret;
}

/**********************************
Function: CreateFileEntry
Description: 创建一个文件表示体FileEntry，
Calls: FindLast ReadSector StrCpy HDRawWrite
Input: name: 文件名, sctBegin: 根目录文件链表的起始扇区（头节点） lastBytes: 链表最后一个扇区用了多少空间
Output: 成功则为 1。
Note: FileEntry 代表一个文件。其存储结构是链式结构的。
**********************************/
static uint CreateFileEntry(const char* name, uint sctBegin, uint lastBytes)
{
    uint ret = 0;
    uint last = FindLast(sctBegin);
    FileEntry* feBase = NULL;

    if( (last != SCT_END_FLAG) && (feBase = (FileEntry*)ReadSector(last)) )
    {
        uint offset = lastBytes / FE_BYTES;
        FileEntry* fe = AddrOff(feBase, offset);

        StrCpy(fe->name, name, sizeof(fe->name) - 1);
        // Note: 直接在内存中操作，不用申请一个数据结构,只需要找到地址fe。
        fe->type = 0;
        fe->sctBegin = SCT_END_FLAG; // 非法值，表示新创建的文件一个扇区也没有。
        fe->sctNum = 0;
        fe->inSctIdx = last;
        fe->inSctOff = offset;
        fe->lastBytes = SECT_SIZE;

        ret = HDRawWrite(last, (byte*)feBase);
    }

    Free(feBase);

    return ret;
}

/**********************************
Function: CreateInRoot
Description: 在根目录文件链表中增加一个文件
Calls: ReadSector CheckStorage CreateFileEntry HDRawWrite
Input: name: 文件名
Output: 成功则为1。
Note:
**********************************/
static uint CreateInRoot(const char* name)
{
    FSRoot* root = (FSRoot*)ReadSector(ROOT_SCT_IDX);
    uint ret = 0;

    if( root )
    {
        // 如果 root->lastBytes = 512 的话，则重新申请一个扇区。并用lastBytes记录此扇区还有多少空间
        CheckStorage(root); 

        if( CreateFileEntry(name, root->sctBegin, root->lastBytes) )
        {
            root->lastBytes += FE_BYTES; 

            ret = HDRawWrite(ROOT_SCT_IDX, (byte*)root);
        }
    }

    Free(root);

    return ret;
}

/**********************************
Function: FindInSector
Description: 在一个扇区中根据文件名来查找FileEntry结构体。
Calls:  StrCmp
Input: name:文件名  feBase:读入一个扇区的起始地址，将其进行强转为了FileEntry  cnt: 需要查找的数量
Output: 查到到的文件结构体 FileEntry 没有找到返回NULL
Note: 这里feBase进行强转是为了方便以AddrOff的形式来遍历
**********************************/
static FileEntry* FindInSector(const char* name, FileEntry* feBase, uint cnt)
{
    FileEntry* ret = NULL;
    uint i = 0;

    for(i=0; i<cnt; i++)
    {
        FileEntry* fe = AddrOff(feBase, i);

        if( StrCmp(fe->name, name, -1) )
        {
            ret = (FileEntry*)Malloc(FE_BYTES);

            if( ret )
            {
                *ret = *fe;
            }

            break;
        }
    }

    return ret;
}

/**********************************
Function: FindFileEntry
Description: 在根目录文件链表中查找是否存在某个文件
Calls: ReadSector FindInSector NextSector
Input: name:要查找的文件名  sctBegin: 起始扇区号  sctNum:需要查找的扇区数  lastBytes:最后一个扇区用来多少空间
Output: 查到到的文件结构体 FileEntry 没有找到返回NULL
Note: 分成两次查 一次是查所有完整的扇区是否有该文件名，第二次是查最后一个扇区是否有该文件名
**********************************/
static FileEntry* FindFileEntry(const char* name, uint sctBegin, uint sctNum, uint lastBytes)
{
    FileEntry* ret = NULL;
    uint next = sctBegin;
    uint i = 0;
    // 最后一个扇区留到后面查找
    for(i=0; i<(sctNum-1); i++)
    {
        FileEntry* feBase = (FileEntry*)ReadSector(next);

        if( feBase )
        {
            ret = FindInSector(name, feBase, FE_ITEM_CNT);
        }

        Free(feBase);

        if( !ret )
        {
            next = NextSector(next);
        }
        else
        {
            break;
        }
    }
    //如果上面没有找到查找最后一个扇区，最后一个扇区不一定为满的，
    if( !ret )
    {
        uint cnt = lastBytes / FE_BYTES;
        FileEntry* feBase = (FileEntry*)ReadSector(next);

        if( feBase )
        {
            ret = FindInSector(name, feBase, cnt);
        }

        Free(feBase);
    }

    return ret;
}

/**********************************
Function: FindInRoot
Description: 查找根目录中是否有某个文件
Calls: ReadSector FindFileEntry
Input: name: 需要查找文件的文件名
Output: 查到到的文件结构体 FileEntry 没有找到返回NULL
Note:
**********************************/
static FileEntry* FindInRoot(const char* name)
{
    FileEntry* ret = NULL;
    FSRoot* root = (FSRoot*)ReadSector(ROOT_SCT_IDX);

    if( root && root->sctNum )
    {
        ret = FindFileEntry(name, root->sctBegin, root->sctNum, root->lastBytes);
    }

    Free(root);

    return ret;
}

/**********************************
Function: FCreate
Description: 创建文件
Calls: FExisted CreateInRoot
Input:  fn 文件名
Output: FS_SUCCEED : 创建成功 , FS_FAILED : 创建失败
Note: 此函数为对外接口
**********************************/
uint FCreate(const char* fn)
{
    uint ret = FExisted(fn);

    if( ret == FS_NONEXISTED )
    {
        ret = CreateInRoot(fn) ? FS_SUCCEED : FS_FAILED;
    }

    return ret;
}

/**********************************
Function: FExisted
Description: 判断文件是否存在
Calls: FindInRoot 
Input:  fn 文件名
Output: FS_EXISTED : 文件存在 , FS_NONEXISTED : 文件不存在
Note: 此函数为对外接口
**********************************/
uint FExisted(const char* fn)
{
    uint ret = FS_FAILED;

    if( fn )
    {
        FileEntry* fe = FindInRoot(fn);

        ret = fe ? FS_EXISTED : FS_NONEXISTED;

        Free(fe);
    }

    return ret;
}

static uint IsOpened(const char* name)
{
    uint ret = 0;
    ListNode* pos = NULL;

    List_ForEach(&gFDList, pos)
    {
        FileDesc* fd = (FileDesc*)pos;

        if( StrCmp(fd->fe.name, name, -1) )
        {
            ret = 1;
            break;
        }
    }

    return ret;
}

/**********************************
Function: FreeFile
Description: 释放文件所占的所有扇区，归还到空闲链表中
Calls: NextSector FreeSector
Input: 起始扇区号
Output: 释放的扇区数
Note:
**********************************/
static uint FreeFile(uint sctBegin)
{
    uint slider = sctBegin;
    uint ret = 0;

    while( slider != SCT_END_FLAG )
    {
        uint next = NextSector(slider);

        ret += FreeSector(slider);

        slider = next;
    }

    return ret;
}

static void MoveFileEntry(FileEntry* dst, FileEntry* src)
{
    uint inSctIdx = dst->inSctIdx;
    uint inSctOff = dst->inSctOff;

    *dst = *src;
    // inSctIdx  inSctOff 表示的是dst在根目录链表中的位置。
    dst->inSctIdx = inSctIdx;
    dst->inSctOff = inSctOff;
}

/**********************************
Function: AdjustStorage
Description: 根据链表最后一个扇区的容量来调整链表的扇区。
Calls: FindLast FindPrev FreeSector MarkSector
Input: fe: 链表头
Output: 成功则为 1; 
Note:
**********************************/
static uint AdjustStorage(FSRoot* fe)
{
    uint ret = 0;

    if( !fe->lastBytes )
    {
        uint last = FindLast(fe->sctBegin);
        uint prev = FindPrev(fe->sctBegin, last);

        if( FreeSector(last) && MarkSector(prev) )
        {
            fe->sctNum--;
            fe->lastBytes = SECT_SIZE;

            if( !fe->sctNum )
            {
                fe->sctBegin = SCT_END_FLAG;
            }

            ret = 1;
        }
    }

    return ret;
}

/**********************************
Function: EraseLast
Description: 在根目录链表中从后面删除bytes大小的字节容量
Calls:  AdjustStorage
Input:  fe: 根目录链表的头, bytes: 要删除的数据量
Output: 删除的数据量的大小
Note:
**********************************/
static uint EraseLast(FSRoot* fe, uint bytes) 
{
    uint ret = 0;

    while( fe->sctNum && bytes )
    {
        if( bytes < fe->lastBytes )
        {
            fe->lastBytes -= bytes;

            ret += bytes;

            bytes = 0;
        }
        else
        {
            bytes -= fe->lastBytes;

            ret += fe->lastBytes;

            fe->lastBytes = 0;

            AdjustStorage(fe);
        }
    }

    return ret;
}

/**********************************
Function: DeleteInRoot
Description: 在根目录链表中删除文件
Calls: ReadSector  FindInRoot FindLast ReadSector FreeFile MoveFileEntry EraseLast HDRawWrite
Input: name: 要删除的文件的名字
Output: 成功则为 1.
Note:
**********************************/
static uint DeleteInRoot(const char* name)
{
    FSRoot* root = (FSRoot*)ReadSector(ROOT_SCT_IDX);
    FileEntry* fe = FindInRoot(name);
    uint ret = 0;

    if( root && fe )
    {
        uint last = FindLast(root->sctBegin);
        FileEntry* feTarget = ReadSector(fe->inSctIdx);
        FileEntry* feLast = (last != SCT_END_FLAG) ? ReadSector(last) : NULL;

        if( feTarget && feLast )
        {
            uint lastOff = root->lastBytes / FE_BYTES - 1;
            FileEntry* lastItem = AddrOff(feLast, lastOff);
            FileEntry* targetItem = AddrOff(feTarget, fe->inSctOff);

            FreeFile(targetItem->sctBegin);

            MoveFileEntry(targetItem, lastItem);

            EraseLast(root, FE_BYTES);

            ret = HDRawWrite(ROOT_SCT_IDX, (byte*)root) &&
                    HDRawWrite(fe->inSctIdx, (byte*)feTarget);
        }

        Free(feTarget);
        Free(feLast);
    }

    Free(root);
    Free(fe);

    return ret;
}

/**********************************
Function: FOpen
Description: 打开一个文件
Calls:
Input: 文件名
Output: 文件描述符（其实就是FileDesc的地址）
Note: 此函数为对外接口
**********************************/
uint FOpen(const char *fn)
{
    FileDesc* ret = NULL;

    if( fn && !IsOpened(fn) )
    {
        FileEntry* fe = NULL;

        ret = (FileDesc*)Malloc(FD_BYTES);
        fe = ret ? FindInRoot(fn) : NULL;

        if( ret && fe )
        {
            ret->fe = *fe;
            ret->objIdx = SCT_END_FLAG;
            ret->offset = SECT_SIZE;
            ret->changed = 0;

            List_Add(&gFDList, (ListNode*)ret);
        }

        Free(fe);
    }

    return (uint)ret;
}

/**********************************
Function: IsFDValid 
Description: 判断文件描述符是否有效，文件描述符只对打开的文件有意义.
Calls:
Input: 文件描述符 FileDesc* fd
Output: 成功则返回 1。
Note: 对外接口函数
**********************************/
static uint IsFDValid(FileDesc* fd)
{
    uint ret = 0;
    ListNode* pos = NULL;

    List_ForEach(&gFDList, pos)
    {
        if( IsEqual(pos, fd) )
        {
            ret = 1;
            break;
        }
    }

    return ret;
}

/**********************************
Function: FlushCache
Description: 将文件描述符结构中的缓冲数据重新写入磁盘中
Calls: FindIndex  HDRawWrite
Input: 文件描述符FileDesc*
Output: 成功返回 1 .
Note: 
**********************************/
static uint FlushCache(FileDesc* fd)
{
    uint ret = 1;

    if( fd->changed )
    {
        uint sctIdx = FindIndex(fd->fe.sctBegin, fd->objIdx);

        ret = 0;

        if( (sctIdx != SCT_END_FLAG) && (ret = HDRawWrite(sctIdx, fd->cache)) )
        {
            fd->changed = 0;
        }
    }

    return ret;
}

/**********************************
Function: FlushFileEntry
Description: 将FileEntry内容重新写入磁盘
Calls: ReadSector  HDRawWrite
Input: FileEntry* fe
Output: 成功返回 1。
Note:
**********************************/
static uint FlushFileEntry(FileEntry* fe)
{
    uint ret = 0;
    FileEntry* feBase = ReadSector(fe->inSctIdx);
    FileEntry* feInSct = AddrOff(feBase, fe->inSctOff);

    *feInSct = *fe;

    ret = HDRawWrite(fe->inSctIdx, (byte*)feBase);

    Free(feBase);

    return ret;
}

/**********************************
Function: ToFlush
Description: 将文件描述符中的缓冲区的内容写到硬盘之中
Calls: FlushCache FlushFileEntry
Input: 文件描述符 fd
Output: 成功则为 1.
Note:
**********************************/
static uint ToFlush(FileDesc* fd)
{
    return FlushCache(fd) && FlushFileEntry(&fd->fe);
}

/**********************************
Function: FClose
Description: 关闭一个文件
Calls: IsFDValid ToFlush List_DelNode
Input: 文件描述符（其实就是FileDesc的地址）
Output: void
Note: 此函数为对外接口
**********************************/
void FClose(uint fd)
{
    FileDesc* pf = (FileDesc*)fd;

    if( IsFDValid(pf) )
    {
        ToFlush(pf);

        List_DelNode((ListNode*)pf);

        Free(pf);
    }
}

/**********************************
Function: ReadToCache
Description: 将链表序号为 idx的读到缓冲区中
Calls: FindIndex ToFlush HDRawRead
Input: fd: 文件描述符, idx: 文件链表的序号
Output: 成功返回 1
Note:
**********************************/
static uint ReadToCache(FileDesc* fd, uint idx)
{
    uint ret = 0;

    if( idx < fd->fe.sctNum )
    {
        uint sctIdx = FindIndex(fd->fe.sctBegin, idx);
        // 先把数据写进到磁盘中，这里的idx 和 fd中的objidx不一定相等。
        ToFlush(fd);

        if( (sctIdx != SCT_END_FLAG) && (ret = HDRawRead(sctIdx, fd->cache)) )
        {
            fd->objIdx = idx;
            fd->offset = 0;
            fd->changed = 0;
        }
    }

    return ret;
}

/**********************************
Function: PrepareCache
Description: 更新文件描述符中的缓冲空间。
Calls: CheckStorage ReadToCache
Input:
Output:
Note:
**********************************/
static uint PrepareCache(FileDesc* fd, uint objIdx)
{
    CheckStorage(&fd->fe);

    return ReadToCache(fd, objIdx);
}

/**********************************
Function: CopyToCache
Description: 拷贝到fd的缓冲区中
Calls: 
Input: fd 文件描述符 , buf : 将要拷贝的内容 , 拷贝的长度。
Output: 拷贝的数量
Note:
**********************************/
static uint CopyToCache(FileDesc* fd, byte* buf, uint len)
{
    uint ret = -1;

    if( fd->objIdx != SCT_END_FLAG )
    {
        uint n = SECT_SIZE - fd->offset;
        byte* p = AddrOff(fd->cache, fd->offset);

        n = (n < len) ? n : len;

        MemCpy(p, buf, n);

        fd->offset += n;
        fd->changed = 1;
        // 更新文件链表的最后一个扇区。
        if( ((fd->fe.sctNum - 1) == fd->objIdx) && (fd->fe.lastBytes < fd->offset) )
        {
            fd->fe.lastBytes = fd->offset;
        }

        ret = n;
    }

    return ret;
}

/**********************************
Function: ToWrite
Description: 将buf中长度为len的内容写入文件中
Calls: PrepareCache CopyToCache
Input:  fd 文件描述符 , buf 写入的内容 , len 要写入的长度。
Output: 返回写入了内容的长度 
Note:
**********************************/
static uint ToWrite(FileDesc* fd, byte* buf, uint len)
{
    uint ret = 1;
    uint i = 0;
    uint n = 0;

    while( (i < len) && ret )
    {
        byte* p = AddrOff(buf, i);

        if( fd->offset == SECT_SIZE )
        {
            ret = PrepareCache(fd, fd->objIdx + 1);
        }

        if( ret )
        {
            n = CopyToCache(fd, p, len - i);

            i += n;
        }
    }

    ret = i;

    return ret;
}

/**********************************
Function: FWrite
Description: 将buf中长度为len的内容写入文件中
Calls: PrepareCache CopyToCache
Input:  fd 文件描述符 , buf 写入的内容 , len 要写入的长度。
Output: 返回写入了内容的长度 
Note: 此函数为对外接口
**********************************/
uint FWrite(uint fd, byte* buf, uint len)
{
    uint ret = -1;

    if( IsFDValid((FileDesc*)fd) && buf )
    {
        ret = ToWrite((FileDesc*)fd, buf, len);
    }

    return ret;
}

/**********************************
Function: FDelete
Description: 删除文件
Calls: IsOpened DeleteInRoot
Input:  fn 文件名
Output: FS_SUCCEED : 删除成功 , FS_FAILED : 删除失败
Note: 此函数为对外接口
**********************************/
uint FDelete(const char* fn)
{
    return fn && !IsOpened(fn) && DeleteInRoot(fn) ? FS_SUCCEED : FS_FAILED;
}

/**********************************
Function: FSFormat
Description: 对 0 ，1 扇区的内容以及扇区分配表进行初始化
Calls: StrCpy HDRawSectors HDRawWrite 
Input: void
Output: 成功则为1.
Note:
**********************************/
uint FSFormat()
{
    FSHeader* header = (FSHeader*)Malloc(SECT_SIZE);
    FSRoot* root = (FSRoot*)Malloc(SECT_SIZE);
    uint* p = (uint*)Malloc(MAP_ITEM_CNT * sizeof(uint));
    uint ret = 0;

    if( header && root && p )
    {
        uint i = 0;
        uint j = 0;
        uint current = 0;

        StrCpy(header->magic, FS_MAGIC, sizeof(header->magic)-1);

        header->sctNum = HDRawSectors();
        // 取余再取整表上向上取整。
        header->mapSize = (header->sctNum - FIXED_SCT_SIZE) / 129 + !!((header->sctNum - FIXED_SCT_SIZE) % 129);
        header->freeNum = header->sctNum - header->mapSize - FIXED_SCT_SIZE;
        header->freeBegin = FIXED_SCT_SIZE + header->mapSize;
        // 将根目录写回到磁盘中。
        ret = HDRawWrite(HEADER_SCT_IDX, (byte*)header);

        StrCpy(root->magic, ROOT_MAGIC, sizeof(root->magic)-1);

        root->sctNum = 0;
        root->sctBegin = SCT_END_FLAG;
        root->lastBytes = SECT_SIZE;

        ret = ret && HDRawWrite(ROOT_SCT_IDX, (byte*)root);
        // 构建空闲扇区链表（扇区分配表）
        for(i=0; ret && (i<header->mapSize) && (current<header->freeNum); i++)
        {
            for(j=0; j<MAP_ITEM_CNT; j++)
            {
                uint* pInt = AddrOff(p, j);

                if( current < header->freeNum )
                {
                    *pInt = current + 1;

                    if( current == (header->freeNum - 1) )
                    {
                        *pInt = SCT_END_FLAG;
                    }

                    current++;
                }
                else
                {
                    break;
                }
            }

            ret = ret && HDRawWrite(i + FIXED_SCT_SIZE, (byte*)p);
        }
    }

    Free(header);
    Free(root);
    Free(p);

    return ret;
}

/**********************************
Function: FSIsFormatted
Description: 判断是否已经对 0 1 扇区的内容进行初始化了。
Calls: ReadSector StrCmp HDRawSectors
Input: void
Output: 成功则为1.
Note: 通过对 0 ， 1 扇区命名和磁盘扇区数是否对来判断是否初始化了
**********************************/
uint FSIsFormatted()
{
    uint ret = 0;
    FSHeader* header = (FSHeader*)ReadSector(HEADER_SCT_IDX);
    FSRoot* root = (FSRoot*)ReadSector(ROOT_SCT_IDX);

    if( header && root )
    {
        ret = StrCmp(header->magic, FS_MAGIC, -1) &&
                (header->sctNum == HDRawSectors()) &&
                StrCmp(root->magic, ROOT_MAGIC, -1);
    }

    Free(header);
    Free(root);

    return ret;
}

/**********************************
Function: FRename
Description: 给文件重新命名
Calls: IsOpened  FindInRoot  StrCpy FlushFileEntry
Input: ofn: 老名字, nfn: 新名字
Output: 成功返回FS_SUCCEED 失败返回 FS_FAILED
Note: 对外接口函数
**********************************/
uint FRename(const char* ofn, const char* nfn)
{
    uint ret = FS_FAILED;

    if( ofn && !IsOpened(ofn) && nfn )
    {
        FileEntry* ofe = FindInRoot(ofn);
        FileEntry* nfe = FindInRoot(nfn);

        if( ofe && !nfe )
        {
            StrCpy(ofe->name, nfn, sizeof(ofe->name) - 1);

            if( FlushFileEntry(ofe) )
            {
                ret = FS_SUCCEED;
            }
        }

        Free(ofe);
        Free(nfe);
    }

    return ret;
}

/**********************************
Function: GetFileLen
Description: 获取文件的数据量（字节长度）
Calls:
Input: 文件描述符 fd
Output: 文件字节数，即文件大小
Note: 
**********************************/
static uint GetFileLen(FileDesc* fd)
{
    uint ret = 0;

    if( fd->fe.sctBegin != SCT_END_FLAG )
    {
        ret = (fd->fe.sctNum - 1) * SECT_SIZE + fd->fe.lastBytes;
    }

    return ret;
}

/**********************************
Function: GetFilePos
Description: 获取文件指针位置处所包含文件的大小
Calls:
Input: 文件描述符 fd
Output: 字节数大小
Note: 即文件从开始到文件指针处所拥有的字节数量
**********************************/
static uint GetFilePos(FileDesc* fd)
{
    uint ret = 0;

    if( fd->objIdx != SCT_END_FLAG )
    {
        ret = fd->objIdx * SECT_SIZE + fd->offset;
    }

    return ret;
}

/**********************************
Function: CopyFromCache
Description: 从文件描述符中的缓冲区中拷贝内容
Calls: MemCpy
Input: fd 文件描述符, buf 存放拷贝内容的缓冲区 , len 长度 
Output: 拷贝成功的字节数
Note: 拷贝的内容不一定是len的长度，有可能受限于文件的大小。
**********************************/
static uint CopyFromCache(FileDesc* fd, byte* buf, uint len)
{
    uint ret = (fd->objIdx != SCT_END_FLAG);

    if( ret )
    {
        uint n = SECT_SIZE - fd->offset;
        byte* p = AddrOff(fd->cache, fd->offset);

        n = (n < len) ? n : len;

        MemCpy(buf, p, n);

        fd->offset += n;

        ret = n;
    }

    return ret;
}

/**********************************
Function: ToRead 
Description: 读文件
Calls: GetFileLen GetFilePos PrepareCache CopyFromCache
Input: fd 文件描述符, buf: 需要读取到的地方 , len 读取的数据的长度
Output: 读的长度，返回 -1 表示读错误
Note:
**********************************/
static uint ToRead(FileDesc* fd, byte* buf, uint len)
{
    uint ret = -1;
    // 计算文件中还可以读多少数据量
    uint n = GetFileLen(fd) - GetFilePos(fd);
    uint i = 0;
    len = (len < n) ? len : n;

    while( (i < len) && ret )
    {
        byte* p = AddrOff(buf, i);

        if( fd->offset == SECT_SIZE )
        {
            ret = PrepareCache(fd, fd->objIdx + 1);
        }

        if( ret )
        {
            n = CopyFromCache(fd, p, len - i);
        }

        i += n;
    }

    ret = i;

    return ret;
}

/**********************************
Function: FRead 
Description: 读文件，对外接口
Calls: GetFileLen GetFilePos PrepareCache CopyFromCache
Input: fd 文件描述符, buf: 需要读取到的地方 , len 读取的数据的长度
Output: 读的长度，返回 -1 表示读错误
Note: 此函数为对外接口
**********************************/
uint FRead(uint fd, byte* buf, uint len)
{
    uint ret = -1;

    if( IsFDValid((FileDesc*)fd) && buf )
    {
        ret = ToRead((FileDesc*)fd, buf, len);
    }

    return ret;
}

/**********************************
Function: ToLocate
Description: 文件读写指针移动函数
Calls: GetFileLen FindIndex ToFlush HDRawRead
Input: fd : 文件描述符 pos 文件指针需要移动的位置
Output: 成功则返回函数指针的位置 ， 失败返回 -1
Note: 
**********************************/
static uint ToLocate(FileDesc* fd, uint pos)
{
    uint ret = -1;
    uint len = GetFileLen(fd);

    pos = (pos < len) ? pos : len;

    {
        uint objIdx = pos / SECT_SIZE;
        uint offset = pos % SECT_SIZE;
        uint sctIdx = FindIndex(fd->fe.sctBegin, objIdx);

        ToFlush(fd);

        if( (sctIdx != SCT_END_FLAG) && HDRawRead(sctIdx, fd->cache) )
        {
            fd->objIdx = objIdx;
            fd->offset = offset;

            ret = pos;
        }
    }

    return ret;
}

/**********************************
Function: FErase
Description: 擦除文件中最后的bytes字节长度的数据
Calls: IsFDValid GetFilePos GetFileLen EraseLast ToLocate
Input: fd 文件描述符 bytes 擦除的数据长度
Output: 实际擦除的数据长度
Note: 对外接口函数
**********************************/
uint FErase(uint fd, uint bytes)
{
    uint ret = 0;
    FileDesc* pf = (FileDesc*)fd;

    if( IsFDValid(pf) )
    {
        uint pos = GetFilePos(pf);
        uint len = GetFileLen(pf);

        ret = EraseLast(&pf->fe, bytes);

        len -= ret;
        // 文件指针指向的是删除区域中，需要移动到删除后文件的的末尾
        if( ret && (pos > len) )
        {
            ToLocate(pf, len);
        }
    }

    return ret;
}

/**********************************
Function: FSeek 
Description: 重新放置文件指针的位置
Calls: IsFDValid ToLocate
Input: fd 文件描述符 pos 文件指针需要放置的位置
Output:  成功为 1 
Note: 对外接口函数
**********************************/
uint FSeek(uint fd, uint pos)
{
    uint ret = -1;
    FileDesc* pf = (FileDesc*)fd;

    if( IsFDValid(pf) )
    {
        ret = ToLocate(pf, pos);
    }

    return ret;
}

/**********************************
Function: FLength
Description: 获取文件大小函数
Calls: GetFileLen
Input: fd 文件描述符
Output: 文件大小（字节为单位）
Note: 对外接口函数
**********************************/
uint FLength(uint fd)
{
    uint ret = -1;
    FileDesc* pf = (FileDesc*)fd;

    if( IsFDValid(pf) )
    {
        ret = GetFileLen(pf);
    }

    return ret;
}

/**********************************
Function: FTell
Description: 获得当前文件指针的文件的位置
Calls:  GetFilePos
Input: fd 文件描述符
Output: 文件指针的位置
Note: 对外接口函数
**********************************/
uint FTell(uint fd)
{
    uint ret = -1;
    FileDesc* pf = (FileDesc*)fd;

    if( IsFDValid(pf) )
    {
        ret = GetFilePos(pf);
    }

    return ret;
}

/**********************************
Function: FFlush
Description: 将文件描述符中的缓冲区写入到磁盘中
Calls:  IsFDValid ToFlush
Input: fd 文件描述符
Output: 成功则返回 1 .
Note: 对外接口函数
**********************************/
uint FFlush(uint fd)
{
    uint ret = -1;
    FileDesc* pf = (FileDesc*)fd;

    if( IsFDValid(pf) )
    {
        ret = ToFlush(pf);
    }

    return ret;
}
