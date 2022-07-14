
#ifndef KERNEL_H
#define KERNEL_H

#include "type.h"
#include "const.h"
//描述符表中的成员
typedef struct {
    ushort limit1;
    ushort base1;
    byte   base2;
    byte   attr1;
    byte   attr2_limit2;
    byte   base3;
} Descriptor;
//全局描述符表
typedef struct {
    Descriptor * const entry; // 从共享内存中拿到这个值
    const int          size; // 大小
} GdtInfo;
// 门描述符表中的成员中的内存分布情况
typedef struct {
    ushort offset1;
    ushort selector;
    byte   dcount;
    byte   attr;
    ushort offset2;
} Gate;

//中断向量表
typedef struct {
    Gate * const entry; // 入口地址
    const int    size; // 大小
} IdtInfo;


// 全局描述符表的入口地址
extern GdtInfo gGdtInfo;
// 中断描述符表的入口地址
extern IdtInfo gIdtInfo;

// 描述子结构进行填充，基址，界限，属性填充到 pDesc中。uint base, uint limit, ushort attr 有多余位的。
int SetDescValue(Descriptor* pDesc, uint base, uint limit, ushort attr);
// 将描述子结构中的值 转换成，基址，界限，属性三个值。
int GetDescValue(Descriptor* pDesc, uint* pBase, uint* pLimit, ushort* pAttr);
// 配置目录页表
void ConfigPageTable();

#endif
