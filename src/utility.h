
#ifndef UTILITY_H
#define UTILITY_H

#include "type.h"

#define AddrOff(a, i)    ((void*)((uint)(a) + (i) * sizeof(*(a))))
#define AddrIndex(b, a)  (((uint)(b) - (uint)(a))/sizeof(*(b)))

#define IsEqual(a, b)           \
({                              \
    unsigned ta = (unsigned)(a);\
    unsigned tb = (unsigned)(b);\
    !(ta - tb);                 \
})
// 计算 member 在 type结构中的偏移地址
#define OffsetOf(type, member)  ((unsigned)&(((type*)0)->member))
// 通过member 成员计算 type结构体的地址，ptr是member的地址。
#define ContainerOf(ptr, type, member)                  \
({                                                      \
      const typeof(((type*)0)->member)* __mptr = (ptr); \
      (type*)((char*)__mptr - OffsetOf(type, member));  \
})

#define Min(a, b) ((a) < (b) ? (a) : (b))
#define Max(a, b) ((a) > (b) ? (a) : (b))

#define Dim(a)  (sizeof(a)/sizeof(*(a)))

void Delay(int n);
byte* MemCpy(byte* dst, const byte* src, uint n);
byte* MemSet(byte* dst, uint n, byte val);
char* StrCpy(char* dst, const char* src, uint n);
int StrLen(const char* s);
int StrCmp(const char* left, const char* right, uint n);
#endif
