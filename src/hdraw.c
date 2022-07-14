#include "hdraw.h"
#include "memory.h"

#define ATA_IDENTIFY    0xEC
#define ATA_READ        0x20
#define ATA_WRITE       0x30

#define REG_DEV_CTRL  0x3F6
#define REG_DATA      0x1F0
#define REG_FEATURES  0x1F1
#define REG_ERROR     0x1F1
#define REG_NSECTOR   0x1F2
#define REG_LBA_LOW   0x1F3
#define REG_LBA_MID   0x1F4
#define REG_LBA_HIGH  0x1F5
#define REG_DEVICE    0x1F6
#define REG_STATUS    0x1F7
#define REG_COMMAND   0x1F7

#define	STATUS_BSY  0x80
#define	STATUS_DRDY 0x40
#define	STATUS_DFSE 0x20
#define	STATUS_DSC  0x10
#define	STATUS_DRQ  0x08
#define	STATUS_CORR 0x04
#define	STATUS_IDX  0x02
#define	STATUS_ERR  0x01

extern byte ReadPort(ushort port);
extern void WritePort(ushort port, byte value);
extern void ReadPortW(ushort port, ushort* buf, uint n);
extern void WritePortW(ushort port, ushort* buf, uint n);

typedef struct
{
    byte lbaLow;
    byte lbaMid;
    byte lbaHigh;
    byte device;
    byte command;
} HDRegValue;

static uint IsBusy()
{
    uint ret = 0;
    uint i = 0;
    
    while( (i < 500) && (ret = (ReadPort(REG_STATUS) & STATUS_BSY)) )
    {
        i++;
    }
    
    return ret;
}

static uint IsDevReady()
{
    return !(ReadPort(REG_STATUS) & STATUS_DRDY);
}

static uint IsDataReady()
{
    return ReadPort(REG_STATUS) & STATUS_DRQ;
}

// 设备寄存器的的值的获取。
static uint MakeDevRegVal(uint si)
{
    return 0xE0 | ((si >> 24) & 0x0F);
}

static HDRegValue MakeRegVals(uint si, uint action)
{
    HDRegValue ret = {0};
    
    ret.lbaLow = si & 0xFF;
    ret.lbaMid = (si >> 8) & 0xFF;
    ret.lbaHigh = (si >> 16) & 0xFF;
    ret.device = MakeDevRegVal(si);
    ret.command = action;
    
    return ret;
}

// 将相应HDRegValue的值写入寄存器当中。
static void WritePorts(HDRegValue hdrv)
{
    WritePort(REG_FEATURES, 0);
    WritePort(REG_NSECTOR, 1);
    WritePort(REG_LBA_LOW, hdrv.lbaLow);
    WritePort(REG_LBA_MID, hdrv.lbaMid);
    WritePort(REG_LBA_HIGH, hdrv.lbaHigh);
    WritePort(REG_DEVICE, hdrv.device);
    WritePort(REG_COMMAND, hdrv.command);
        
    WritePort(REG_DEV_CTRL, 0);
}

void HDRawModInit()
{

}

// 获取硬盘的信息，只用获取一次便可，所以用局部变量ret保存，获取扇区数量
uint HDRawSectors()
{
    static uint ret = -1;
    
    if( (ret == -1) && IsDevReady() )
    {
        HDRegValue hdrv = MakeRegVals(0, ATA_IDENTIFY);
        byte* buf = Malloc(SECT_SIZE);
        
        WritePorts(hdrv);
        
        if( !IsBusy() && IsDataReady() && buf )
        {
            ushort* data = (ushort*)buf;
            
            ReadPortW(REG_DATA, data, SECT_SIZE >> 1);
            
            ret = (data[61] << 16) | (data[60]);
        }
        
        Free(buf);
    }
    
    return ret;
}

/**********************************
Function: HDRawWrite
Description: 对硬盘写数据，调用汇编的函数WritePortW，一次写入一个扇区
Calls: HDRawSectors IsBusy MakeRegVals WritePorts IsDataReady WritePortW
Input: si: 逻辑扇区地址 ,  buf: 从扇区读取的数据缓冲区。
Output: 成功则为1.
Note: 需要用到 kentry.asm 中的 WritePortW函数
**********************************/
uint HDRawWrite(uint si, byte* buf)
{
    uint ret = 0;
    
    if( (si < HDRawSectors()) && buf && !IsBusy() )
    {
        HDRegValue hdrv = MakeRegVals(si, ATA_WRITE);
        
        WritePorts(hdrv);
        
        if( ret = (!IsBusy() && IsDataReady()) )
        {
            ushort* data = (ushort*)buf;
            
            WritePortW(REG_DATA, data, SECT_SIZE >> 1);
        }
    }
    
    return ret;
}
// 从硬盘中读取数据，返回值，读是否成功
uint HDRawRead(uint si, byte* buf)
{
    uint ret = 0;
    
    if( (si < HDRawSectors()) && buf && !IsBusy() )
    {
        HDRegValue hdrv = MakeRegVals(si, ATA_READ);
        
        WritePorts(hdrv);
        
        if( ret = (!IsBusy() && IsDataReady()) )
        {
            ushort* data = (ushort*)buf;
            
            ReadPortW(REG_DATA, data, SECT_SIZE >> 1);
        }
    }
    
    return ret;
}
