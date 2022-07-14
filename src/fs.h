#ifndef FS_H
#define FS_H

#include "type.h"

enum
{
    FS_FAILED,
    FS_SUCCEED,
    FS_EXISTED,
    FS_NONEXISTED
};

void FSModInit();
uint FSFormat();
uint FSIsFormatted();

uint FCreate(const char* fn);
uint FExisted(const char* fn);
uint FDelete(const char* fn);
uint FRename(const char* ofn, const char* nfn);

uint FOpen(const char* fn);
uint FWrite(uint fd, byte* buf, uint len);
uint FRead(uint fd, byte* buf, uint len);
void FClose(uint fd);
uint FErase(uint fd, uint bytes);
uint FSeek(uint fd, uint pos);
uint FLength(uint fd);
uint FTell(uint fd);
uint FFlush(uint fd);


#endif
