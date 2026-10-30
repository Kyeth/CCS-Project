#ifndef __PARAMSAVE_H
#define __PARAMSAVE_H

#include "RingBuff.h"


#ifdef __cplusplus
extern "C" {
#endif


#define PARAM_ROOT  0x55AAAA55
#define PARAM_ROOT_BYTE0 ( (PARAM_ROOT >> 0) & 0xFF )
#define PARAM_ROOT_BYTE1 ( (PARAM_ROOT >> 8) & 0xFF )
#define PARAM_ROOT_BYTE2 ( (PARAM_ROOT >> 16) & 0xFF )
#define PARAM_ROOT_BYTE3 ( (PARAM_ROOT >> 24) & 0xFF )

#define PARAM_MANAGER_ADDR  0

#define PARAM_START_ADDR  (PARAM_MANAGER_ADDR+8)

#define PARAM_BANK_SIZE 4   // 对于EEPROM来说，每个参数占用4个字节

#define CMD_SAVE_SING_PARAM     0x01
#define CMD_SAVE_ALL_PARAM      0x02
#define CMD_LOAD_SING_PARAM     0x04
#define CMD_LOAR_ALL_PARAM      0x08


#define CMD_BUFF_SIZE 32


// 寄存器对应的值的类型
typedef enum
{
    PARAM_TYPE_INT8,
    PARAM_TYPE_UINT8,
    PARAM_TYPE_INT16,
    PARAM_TYPE_UINT16,
    PARAM_TYPE_INT32,
    PARAM_TYPE_UINT32,
    PARAM_TYPE_FLOAT
}E_PARAM_TYPE;




//  联合体
typedef union
{
    unsigned char   u8Val;
    char            i8Val;
    Uint16  u16Val;
    int16   i16Val;
    Uint32  u32Val;
    int32   i32Val;
    float   fVal;
}U_PARAM_VAL;

typedef struct
{
    int (*ParamWrite)(Uint32 u32Addr, Uint16 *u16Buff, Uint16 u16Size);
    int (*ParamRead)(Uint32 u32Addr, Uint16 *u16Buff, Uint16 u16Size);
}ST_PARAM_WR_INTF;


typedef struct
{
    Uint16 u16Index;
    Uint16 bIsSaveFlag      :1;
    Uint16 bParamType       :4;
    Uint16 Rsv              :11;
    U_PARAM_VAL *puParamVal;
}ST_PARAM_ITEM;

typedef struct
{
    ST_PARAM_ITEM *pstParamItem;
    Uint16 u16ItemNum;
}ST_PARAM_CFG;


typedef struct
{
    Uint16 u16Cmd;
    Uint16 u16Index;
}ST_PARAM_CMD;

typedef struct
{
    ST_PARAM_CFG *pstParamCfg;
    ST_PARAM_CMD stParamCmdBuff[CMD_BUFF_SIZE];
    ST_RING_BUFF stParamCMD;
    ST_PARAM_WR_INTF stParamWRIntf;
    Uint16 bCmdParam :8;
    Uint16 Rsv       :8;
}ST_PARAM_SAVE_HANDLE;


extern int16 ParamSaveInit(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle, ST_PARAM_CFG *pstParamCfg, ST_PARAM_WR_INTF *pstParamWrIntf);

extern int16 SaveAllParam(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle);

extern int16 SaveSingleParam(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle, Uint16 u16Index);

extern int16 SaveAllParamProc(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle);

extern int16 LoadAllParam(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle);

extern int16 LoadAllParamProc(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle);

extern int16 LoadSingleParam(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle, Uint16 u16Index);

extern int16 LoadSingleParamProc(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle,  Uint16 u16Index);

extern int16 SaveSingleParamProc(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle, Uint16 u16Index);

extern int16 ParamSaveHandle(ST_PARAM_SAVE_HANDLE *pstParamSaveHandle);

#ifdef __cplusplus
}
#endif

#endif
