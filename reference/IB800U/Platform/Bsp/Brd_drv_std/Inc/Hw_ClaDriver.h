#ifndef __HW_CLADRIVER_H
#define __HW_CLADRIVER_H

#include "F28003x_device.h"
#include "cla.h"


#ifdef __cplusplus
    extern "C" {
#endif


#define Cla1ForceTask1()       asm("  IACK  #0x0001")
#define Cla1ForceTask2()       asm("  IACK  #0x0002")
#define Cla1ForceTask3()       asm("  IACK  #0x0004")
#define Cla1ForceTask4()       asm("  IACK  #0x0008")
#define Cla1ForceTask5()       asm("  IACK  #0x0010")
#define Cla1ForceTask6()       asm("  IACK  #0x0020")
#define Cla1ForceTask7()       asm("  IACK  #0x0040")
#define Cla1ForceTask8()       asm("  IACK  #0x0080")

enum
{
    TASK_NO_1 = 1,
    TASK_NO_2 = 2,
    TASK_NO_3 = 3,
    TASK_NO_4 = 4,
    TASK_NO_5 = 5,
    TASK_NO_6 = 6,
    TASK_NO_7 = 7,
    TASK_NO_8 = 8,
};




typedef struct
{
    Uint16 u16TaskNo;   // 任务号从1到任务8
    Uint16 u16IntVectorNum;
    Uint16 u16TrigerSrc;
    void (*ClaTask)(void);
    void (*ClaIsr)(void);
}ST_CLA_CFG_ITEM;

typedef struct
{
    ST_CLA_CFG_ITEM *pstClaCfgItem;
    Uint16 u16ItemNum;
}ST_CLA_CFG;


extern void Hw_ClaInit(ST_CLA_CFG *pstClaCfg);



#ifdef __cplusplus
    }
#endif

#endif
